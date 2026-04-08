#include "services/calibration_service.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace sag {

static const std::unordered_map<std::string, int> ARUCO_DICT_MAP = {
    {"DICT_4X4_50",   cv::aruco::DICT_4X4_50},
    {"DICT_4X4_100",  cv::aruco::DICT_4X4_100},
    {"DICT_4X4_250",  cv::aruco::DICT_4X4_250},
    {"DICT_4X4_1000", cv::aruco::DICT_4X4_1000},
    {"DICT_5X5_50",   cv::aruco::DICT_5X5_50},
    {"DICT_5X5_100",  cv::aruco::DICT_5X5_100},
    {"DICT_5X5_250",  cv::aruco::DICT_5X5_250},
    {"DICT_5X5_1000", cv::aruco::DICT_5X5_1000},
    {"DICT_6X6_50",   cv::aruco::DICT_6X6_50},
    {"DICT_6X6_100",  cv::aruco::DICT_6X6_100},
    {"DICT_6X6_250",  cv::aruco::DICT_6X6_250},
    {"DICT_6X6_1000", cv::aruco::DICT_6X6_1000},
    {"DICT_7X7_50",   cv::aruco::DICT_7X7_50},
    {"DICT_7X7_100",  cv::aruco::DICT_7X7_100},
    {"DICT_7X7_250",  cv::aruco::DICT_7X7_250},
    {"DICT_7X7_1000", cv::aruco::DICT_7X7_1000},
};

cv::aruco::Dictionary CalibrationService::get_dictionary(const std::string& name) {
    auto it = ARUCO_DICT_MAP.find(name);
    if (it != ARUCO_DICT_MAP.end())
        return cv::aruco::getPredefinedDictionary(it->second);
    return cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_1000);
}

CalibrationModel CalibrationService::calibrate(
    const cv::Mat& frame,
    const CalibrationMarkerLayout& layout,
    float playable_width,
    float playable_height,
    float playable_inset)
{
    CalibrationModel model;
    model.state = CalibrationState::Detecting;
    model.marker_layout = layout;

    if (frame.empty()) {
        model.state = CalibrationState::Invalid;
        model.validation_message = "No frame available";
        return model;
    }

    // Convert to grayscale for detection
    cv::Mat gray;
    if (frame.channels() == 3)
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    else
        gray = frame;

    // Detect ArUco markers
    auto dictionary = get_dictionary(dictionary_name_);
    cv::aruco::DetectorParameters parameters;
    std::vector<std::vector<cv::Point2f>> corners;
    std::vector<int> ids;
    cv::aruco::ArucoDetector detector(dictionary, parameters);
    detector.detectMarkers(gray, corners, ids);

    // Store detected marker corners
    for (size_t i = 0; i < ids.size(); ++i) {
        cv::Mat cornerMat(corners[i]);
        model.detected_marker_corners[ids[i]] = cornerMat.clone();
    }

    // Check all 4 markers found
    for (int required_id : layout.marker_ids) {
        if (model.detected_marker_corners.find(required_id) == model.detected_marker_corners.end()) {
            model.state = CalibrationState::Invalid;
            model.validation_message = "Missing marker ID " + std::to_string(required_id);
            return model;
        }
    }

    // Compute marker centers (pixel coordinates)
    std::vector<cv::Point2f> src_points;
    for (int mid : layout.marker_ids) {
        auto& corner_mat = model.detected_marker_corners[mid];
        auto pts = corner_mat.reshape(2);
        cv::Point2f center(0, 0);
        for (int r = 0; r < pts.rows; ++r) {
            auto pt = pts.at<cv::Point2f>(r);
            center += pt;
        }
        center *= (1.0f / static_cast<float>(pts.rows));
        src_points.push_back(center);
    }

    // World points: markers at corners of playable area
    // Layout: 0=TL, 1=TR, 2=BR, 3=BL
    std::vector<cv::Point2f> dst_points = {
        {0.0f, 0.0f},
        {playable_width, 0.0f},
        {playable_width, playable_height},
        {0.0f, playable_height},
    };

    // Compute homography
    model.homography = cv::findHomography(src_points, dst_points);
    if (model.homography.empty()) {
        model.state = CalibrationState::Invalid;
        model.validation_message = "Failed to compute homography";
        return model;
    }

    // Validate reprojection error
    std::vector<cv::Point2f> reprojected;
    cv::perspectiveTransform(src_points, reprojected, model.homography);
    float max_error = 0.0f;
    for (size_t i = 0; i < dst_points.size(); ++i) {
        float err = cv::norm(reprojected[i] - dst_points[i]);
        max_error = std::max(max_error, err);
    }

    if (max_error > constants::REPROJECTION_ERROR_THRESHOLD) {
        model.state = CalibrationState::Invalid;
        model.validation_message = "Reprojection error too high: " + std::to_string(max_error);
        return model;
    }

    // Compute inverse homography
    model.inverse_homography = model.homography.inv();

    // Set bounds
    model.outer_bounds = Quad{
        Point{0.0f, 0.0f},
        Point{playable_width, 0.0f},
        Point{playable_width, playable_height},
        Point{0.0f, playable_height},
    };

    model.playable_bounds = Quad{
        Point{playable_inset, playable_inset},
        Point{playable_width - playable_inset, playable_inset},
        Point{playable_width - playable_inset, playable_height - playable_inset},
        Point{playable_inset, playable_height - playable_inset},
    };

    model.state = CalibrationState::Valid;
    model.validation_message = "Calibrated";
    return model;
}

std::vector<std::string> CalibrationService::supported_dictionary_names() {
    std::vector<std::string> names;
    names.reserve(ARUCO_DICT_MAP.size());
    for (const auto& [name, _] : ARUCO_DICT_MAP)
        names.push_back(name);
    std::sort(names.begin(), names.end());
    return names;
}

} // namespace sag
