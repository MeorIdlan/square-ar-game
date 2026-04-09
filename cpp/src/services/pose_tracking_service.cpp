#include "services/pose_tracking_service.h"
#include "utils/math_helpers.h"
#include "utils/logger.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <chrono>
#include <vector>

namespace sag
{

    // YOLOv8-pose raw output shape: [1, 56, 8400]
    // Each column is one anchor: [x_c, y_c, w, h, obj_conf, kp0_x, kp0_y, kp0_v, ..., kp16_x, kp16_y, kp16_v]
    static constexpr int YOLO_KP_COUNT = 17;                                                 // COCO keypoints
    static constexpr int YOLO_KP_VALUES = 3;                                                 // x, y, visibility
    static constexpr int YOLO_BOX_FIELDS = 5;                                                // x_c, y_c, w, h, obj_conf
    static constexpr int YOLO_ROW_STRIDE = YOLO_BOX_FIELDS + YOLO_KP_COUNT * YOLO_KP_VALUES; // 56

    bool PoseTrackingService::initialize(const std::string &model_path, int num_poses)
    {
        close();
        num_poses_ = num_poses;

        net_ = cv::dnn::readNetFromONNX(model_path);
        if (net_.empty())
        {
            Logger::error(std::format("PoseTrackingService: failed to load model from {}", model_path));
            return false;
        }

        // Use OpenCL (GPU) if available and requested
        net_.setPreferableBackend(cv::dnn::DNN_BACKEND_DEFAULT);
        net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

        initialized_ = true;
        Logger::info(std::format("PoseTrackingService initialized from: {}", model_path));
        return true;
    }

    PoseResult PoseTrackingService::process_frame(const FramePacket &packet)
    {
        PoseResult result;
        result.frame_id = packet.frame_id;

        auto now = std::chrono::steady_clock::now();
        result.timestamp = std::chrono::duration<double>(now.time_since_epoch()).count();

        if (!initialized_ || packet.frame.empty())
        {
            result.status_text = "Pose tracker not initialized or empty frame";
            return result;
        }

        const int frame_w = packet.frame.cols;
        const int frame_h = packet.frame.rows;

        // Letterbox-resize to 640x640, keep aspect ratio
        float scale = std::min(
            static_cast<float>(input_size_) / static_cast<float>(frame_w),
            static_cast<float>(input_size_) / static_cast<float>(frame_h));
        int scaled_w = static_cast<int>(std::round(frame_w * scale));
        int scaled_h = static_cast<int>(std::round(frame_h * scale));
        int pad_left = (input_size_ - scaled_w) / 2;
        int pad_top = (input_size_ - scaled_h) / 2;

        cv::Mat resized;
        cv::resize(packet.frame, resized, cv::Size(scaled_w, scaled_h));
        cv::Mat padded(input_size_, input_size_, CV_8UC3, cv::Scalar(114, 114, 114));
        resized.copyTo(padded(cv::Rect(pad_left, pad_top, scaled_w, scaled_h)));

        // BGR → RGB blob, normalise to [0,1], NCHW
        cv::Mat blob = cv::dnn::blobFromImage(
            padded, 1.0 / 255.0, cv::Size(input_size_, input_size_),
            cv::Scalar(0, 0, 0), /*swapRB=*/true, /*crop=*/false);

        net_.setInput(blob);

        // Forward pass — output: [1, 56, 8400]
        cv::Mat raw = net_.forward();

        // raw is [1, 56, 8400] → reshape to [56, 8400] then transpose to [8400, 56]
        cv::Mat output = raw.reshape(1, YOLO_ROW_STRIDE); // [56 x 8400]
        cv::Mat detections;
        cv::transpose(output, detections); // [8400 x 56]

        // Collect boxes that pass confidence threshold
        std::vector<cv::Rect2f> boxes;
        std::vector<float> scores;
        std::vector<int> indices_keep;

        const int rows = detections.rows;
        for (int i = 0; i < rows; ++i)
        {
            const float *row = detections.ptr<float>(i);
            float conf = row[4];
            if (conf < conf_threshold_)
                continue;

            // Box in padded-image coords — convert to xyxy
            float cx = row[0], cy = row[1], bw = row[2], bh = row[3];
            float x1 = cx - bw * 0.5f;
            float y1 = cy - bh * 0.5f;
            boxes.emplace_back(x1, y1, bw, bh);
            scores.push_back(conf);
            indices_keep.push_back(i);
        }

        // NMS
        std::vector<int> nms_result;
        cv::dnn::NMSBoxes(boxes, scores, conf_threshold_, nms_threshold_, nms_result);

        // Sort by confidence descending, keep up to num_poses_
        std::sort(nms_result.begin(), nms_result.end(),
                  [&](int a, int b)
                  { return scores[a] > scores[b]; });
        if (static_cast<int>(nms_result.size()) > num_poses_)
            nms_result.resize(static_cast<size_t>(num_poses_));

        result.raw_pose_count = static_cast<int>(nms_result.size());

        // Inverse-letterbox scale factor: go from padded coords → original frame coords
        auto unpad = [&](float px, float py) -> Point
        {
            float x = (px - static_cast<float>(pad_left)) / scale;
            float y = (py - static_cast<float>(pad_top)) / scale;
            x = std::clamp(x, 0.0f, static_cast<float>(frame_w - 1));
            y = std::clamp(y, 0.0f, static_cast<float>(frame_h - 1));
            return {x, y};
        };

        for (int ni : nms_result)
        {
            const float *row = detections.ptr<float>(indices_keep[ni]);

            // Extract keypoints — layout: [5 + kp_idx*3 + {0=x, 1=y, 2=vis}]
            auto extract_kp = [&](int kp_idx) -> std::optional<Point>
            {
                int base = YOLO_BOX_FIELDS + kp_idx * YOLO_KP_VALUES;
                float vis = row[base + 2];
                if (vis < min_landmark_visibility_)
                    return std::nullopt;
                return unpad(row[base], row[base + 1]);
            };

            PoseFootState foot;

            // COCO 15 = left ankle, 16 = right ankle (from constants.h)
            for (int idx : constants::FOOT_LANDMARK_LEFT)
                foot.left_foot = extract_kp(idx);
            for (int idx : constants::FOOT_LANDMARK_RIGHT)
                foot.right_foot = extract_kp(idx);

            foot.resolved_point = resolve_feet_midpoint(foot.left_foot, foot.right_foot);

            // Confidence: mean visibility of the two ankle keypoints
            float total_vis = 0.0f;
            int vis_count = 0;
            for (int kp_idx : {15, 16})
            {
                int base = YOLO_BOX_FIELDS + kp_idx * YOLO_KP_VALUES;
                float vis = row[base + 2];
                total_vis += vis;
                ++vis_count;
            }
            foot.confidence = vis_count > 0 ? total_vis / static_cast<float>(vis_count) : 0.0f;

            if (foot.resolved_point)
                result.detections.push_back(std::move(foot));
        }

        result.status_text = "Detected " + std::to_string(result.detections.size()) + " pose(s)";
        return result;
    }

    void PoseTrackingService::close()
    {
        net_ = cv::dnn::Net{};
        initialized_ = false;
    }

} // namespace sag
