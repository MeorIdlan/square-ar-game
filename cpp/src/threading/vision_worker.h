#pragma once
// Vision worker — runs pose detection on the latest camera frame

#include "../models/contracts.h"
#include "../services/interfaces.h"
#include "../threading/shared_state.h"
#include <thread>

namespace sag {

class VisionWorker {
public:
    VisionWorker(IPoseTracker& pose_tracker,
                 LatestValue<FramePacket>& frame_input,
                 LatestValue<PoseResult>& pose_output)
        : pose_tracker_(pose_tracker)
        , frame_input_(frame_input)
        , pose_output_(pose_output)
    {}

    void start() {
        worker_.start([this]() { process_loop(); });
    }

    void stop() {
        worker_.stop();
    }

private:
    void process_loop() {
        auto frame = frame_input_.take();
        if (!frame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return;
        }

        PoseResult result = pose_tracker_.process_frame(*frame);
        pose_output_.set(std::move(result));
    }

    IPoseTracker&             pose_tracker_;
    LatestValue<FramePacket>& frame_input_;
    LatestValue<PoseResult>&  pose_output_;
    WorkerThread              worker_;
};

} // namespace sag
