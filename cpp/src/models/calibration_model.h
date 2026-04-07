#pragma once
// CalibrationModel — mirrors src/models/calibration_model.py

#include "models/enums.h"
#include "utils/math_helpers.h"

#include <opencv2/core.hpp>
#include <array>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace sag {

struct CalibrationMarkerLayout {
    std::array<int, 4> marker_ids = {0, 1, 2, 3};
};

using Quad = std::array<Point, 4>; // tl, tr, br, bl

struct CalibrationModel {
    CalibrationState state = CalibrationState::NotCalibrated;
    CalibrationMarkerLayout marker_layout;
    std::unordered_map<int, cv::Mat> detected_marker_corners;

    cv::Mat homography;           // 3x3 pixel→floor
    cv::Mat inverse_homography;   // 3x3 floor→pixel

    std::optional<Quad> outer_bounds;
    std::optional<Quad> playable_bounds;

    std::string validation_message = "Not calibrated";

    bool is_valid() const {
        return state == CalibrationState::Valid && !homography.empty();
    }

    std::vector<int> detected_marker_ids() const {
        std::vector<int> ids;
        ids.reserve(detected_marker_corners.size());
        for (auto& [id, _] : detected_marker_corners)
            ids.push_back(id);
        std::sort(ids.begin(), ids.end());
        return ids;
    }

    int detected_marker_count() const {
        return static_cast<int>(detected_marker_corners.size());
    }

    std::vector<int> missing_marker_ids() const {
        std::vector<int> missing;
        for (int id : marker_layout.marker_ids) {
            if (detected_marker_corners.find(id) == detected_marker_corners.end())
                missing.push_back(id);
        }
        return missing;
    }
};

} // namespace sag
