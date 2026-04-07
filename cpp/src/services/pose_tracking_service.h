#pragma once
// PoseTrackingService — ONNX Runtime pose detection

#include "services/interfaces.h"
#include "utils/constants.h"

#include <memory>
#include <string>

// Forward declare to avoid ONNX headers in the public interface
namespace Ort { class Session; class Env; class SessionOptions; }

namespace sag {

class PoseTrackingService : public IPoseTracker {
public:
    PoseTrackingService();
    ~PoseTrackingService();

    bool initialize(const std::string& model_path, int num_poses) override;
    PoseResult process_frame(const FramePacket& packet) override;
    void close() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    float min_landmark_visibility_ = 0.2f;
    int num_poses_ = 4;
    bool initialized_ = false;

    PoseFootState extract_foot_state(const float* landmarks, int num_landmarks) const;
};

} // namespace sag
