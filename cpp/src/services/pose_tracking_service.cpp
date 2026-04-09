#include "services/pose_tracking_service.h"
#include "utils/math_helpers.h"
#include "utils/logger.h"

#include <opencv2/imgproc.hpp>

// ONNX Runtime headers
#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <chrono>
#include <numeric>
#include <vector>

namespace sag
{

    struct PoseTrackingService::Impl
    {
        Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "PoseTracker"};
        std::unique_ptr<Ort::Session> session;
        Ort::SessionOptions session_options;

        // Model I/O metadata (populated on init)
        std::vector<const char *> input_names;
        std::vector<const char *> output_names;
        std::vector<int64_t> input_shape; // e.g. {1, 256, 256, 3}
    };

    PoseTrackingService::PoseTrackingService()
        : impl_(std::make_unique<Impl>()) {}

    PoseTrackingService::~PoseTrackingService() { close(); }

    bool PoseTrackingService::initialize(const std::string &model_path, int num_poses)
    {
        num_poses_ = num_poses;

        try
        {
            impl_->session_options.SetIntraOpNumThreads(2);
            impl_->session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

            // Try GPU (DirectML) first, fall back to CPU
#ifdef USE_DIRECTML
            try
            {
                Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_DML(impl_->session_options, 0));
            }
            catch (...)
            {
                // GPU not available, CPU fallback
            }
#endif

#ifdef _WIN32
            std::wstring wpath(model_path.begin(), model_path.end());
            impl_->session = std::make_unique<Ort::Session>(
                impl_->env, wpath.c_str(), impl_->session_options);
#else
            impl_->session = std::make_unique<Ort::Session>(
                impl_->env, model_path.c_str(), impl_->session_options);
#endif

            // Query input/output names and shapes
            auto allocator = Ort::AllocatorWithDefaultOptions();

            size_t num_inputs = impl_->session->GetInputCount();
            for (size_t i = 0; i < num_inputs; ++i)
            {
                auto name = impl_->session->GetInputNameAllocated(i, allocator);
                impl_->input_names.push_back(strdup(name.get()));
            }
            Logger::info(std::format("ONNX model loaded: {} input(s), {} output(s)",
                                     num_inputs, impl_->session->GetOutputCount()));

            size_t num_outputs = impl_->session->GetOutputCount();
            for (size_t i = 0; i < num_outputs; ++i)
            {
                auto name = impl_->session->GetOutputNameAllocated(i, allocator);
                impl_->output_names.push_back(strdup(name.get()));
            }

            // Get input shape
            auto type_info = impl_->session->GetInputTypeInfo(0);
            auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
            impl_->input_shape = tensor_info.GetShape();
            std::string shape_str;
            for (auto d : impl_->input_shape)
                shape_str += std::to_string(d) + " ";
            Logger::info(std::format("ONNX input shape: [{}]", shape_str));

            initialized_ = true;
            Logger::info(std::format("PoseTrackingService initialized from: {}", model_path));
            return true;
        }
        catch (const Ort::Exception &e)
        {
            Logger::error(std::format("ONNX init failed: {}", e.what()));
            initialized_ = false;
            return false;
        }
    }

    PoseResult PoseTrackingService::process_frame(const FramePacket &packet)
    {
        PoseResult result;
        result.frame_id = packet.frame_id;

        auto now = std::chrono::steady_clock::now();
        result.timestamp = std::chrono::duration<double>(now.time_since_epoch()).count();

        if (!initialized_ || !impl_->session || packet.frame.empty())
        {
            result.status_text = "Pose tracker not initialized or empty frame";
            return result;
        }

        // Determine model input size (typically 256x256 or 224x224)
        int input_h = (impl_->input_shape.size() >= 3) ? static_cast<int>(impl_->input_shape[1]) : 256;
        int input_w = (impl_->input_shape.size() >= 4) ? static_cast<int>(impl_->input_shape[2]) : 256;

        // Preprocess: resize to model input size, convert BGR→RGB, normalize to [0,1]
        cv::Mat rgb;
        cv::cvtColor(packet.frame, rgb, cv::COLOR_BGR2RGB);

        cv::Mat resized;
        cv::resize(rgb, resized, cv::Size(input_w, input_h));

        cv::Mat float_mat;
        resized.convertTo(float_mat, CV_32F, 1.0f / 255.0f);

        // Create ONNX tensor
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<int64_t> input_shape = {1, input_h, input_w, 3};
        size_t input_size = static_cast<size_t>(input_h * input_w * 3);

        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, float_mat.ptr<float>(), input_size,
            input_shape.data(), input_shape.size());

        // Run inference
        try
        {
            auto output_tensors = impl_->session->Run(
                Ort::RunOptions{nullptr},
                impl_->input_names.data(), &input_tensor, 1,
                impl_->output_names.data(), impl_->output_names.size());

            // Parse output: expected shape [1, num_poses, 33, 5] (x, y, z, visibility, presence)
            // or [1, 33, 5] for single-pose model
            auto &landmarks_tensor = output_tensors[0];
            auto shape = landmarks_tensor.GetTensorTypeAndShapeInfo().GetShape();
            const float *data = landmarks_tensor.GetTensorMutableData<float>();

            int num_detected = 1;
            int landmarks_per_pose = 33;
            int values_per_landmark = 5;

            if (shape.size() == 4)
            {
                num_detected = static_cast<int>(shape[1]);
                landmarks_per_pose = static_cast<int>(shape[2]);
                values_per_landmark = static_cast<int>(shape[3]);
            }
            else if (shape.size() == 3)
            {
                landmarks_per_pose = static_cast<int>(shape[1]);
                values_per_landmark = static_cast<int>(shape[2]);
            }

            result.raw_pose_count = num_detected;

            float scale_x = static_cast<float>(packet.frame.cols);
            float scale_y = static_cast<float>(packet.frame.rows);

            for (int p = 0; p < std::min(num_detected, num_poses_); ++p)
            {
                const float *pose_data = data + p * landmarks_per_pose * values_per_landmark;

                // Extract foot landmarks and scale to original image coords
                PoseFootState foot;
                auto extract_foot = [&](const std::array<int, 3> &indices) -> std::optional<Point>
                {
                    float sum_x = 0, sum_y = 0, sum_vis = 0;
                    int count = 0;
                    for (int idx : indices)
                    {
                        if (idx >= landmarks_per_pose)
                            continue;
                        const float *lm = pose_data + idx * values_per_landmark;
                        float x = lm[0], y = lm[1];
                        float vis = (values_per_landmark >= 4) ? lm[3] : 1.0f;
                        if (vis >= min_landmark_visibility_)
                        {
                            sum_x += x * scale_x;
                            sum_y += y * scale_y;
                            sum_vis += vis;
                            ++count;
                        }
                    }
                    if (count == 0)
                        return std::nullopt;
                    return Point{sum_x / static_cast<float>(count),
                                 sum_y / static_cast<float>(count)};
                };

                foot.left_foot = extract_foot(constants::FOOT_LANDMARK_LEFT);
                foot.right_foot = extract_foot(constants::FOOT_LANDMARK_RIGHT);
                foot.resolved_point = resolve_feet_midpoint(foot.left_foot, foot.right_foot);

                // Confidence: average visibility of all detected foot landmarks
                float total_vis = 0;
                int vis_count = 0;
                for (int idx : {27, 28, 29, 30, 31, 32})
                {
                    if (idx >= landmarks_per_pose)
                        continue;
                    const float *lm = pose_data + idx * values_per_landmark;
                    float vis = (values_per_landmark >= 4) ? lm[3] : 1.0f;
                    total_vis += vis;
                    ++vis_count;
                }
                foot.confidence = (vis_count > 0) ? total_vis / static_cast<float>(vis_count) : 0.0f;

                if (foot.resolved_point)
                    result.detections.push_back(std::move(foot));
            }

            result.status_text = "Detected " + std::to_string(result.detections.size()) + " pose(s)";
        }
        catch (const Ort::Exception &e)
        {
            result.status_text = std::string("Inference error: ") + e.what();
        }

        return result;
    }

    void PoseTrackingService::close()
    {
        impl_->session.reset();
        for (auto name : impl_->input_names)
            free(const_cast<char *>(name));
        for (auto name : impl_->output_names)
            free(const_cast<char *>(name));
        impl_->input_names.clear();
        impl_->output_names.clear();
        initialized_ = false;
    }

} // namespace sag
