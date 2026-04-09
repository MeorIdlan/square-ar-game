#pragma once
// PoseTrackingService — OpenCV DNN pose detection (YOLOv8-pose ONNX)

#include "services/interfaces.h"
#include "utils/constants.h"

#include <opencv2/dnn.hpp>

#include <string>

namespace sag
{

    class PoseTrackingService : public IPoseTracker
    {
    public:
        PoseTrackingService() = default;
        ~PoseTrackingService() override { close(); }

        bool initialize(const std::string &model_path, int num_poses) override;
        PoseResult process_frame(const FramePacket &packet) override;
        void close() override;

    private:
        cv::dnn::Net net_;

        float conf_threshold_ = 0.35f;
        float nms_threshold_ = 0.45f;
        float min_landmark_visibility_ = 0.2f;
        int num_poses_ = 4;
        int input_size_ = 640; // YOLOv8-pose expects 640x640
        bool initialized_ = false;
    };

} // namespace sag
