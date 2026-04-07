#pragma once
// Camera capture worker — runs on a dedicated thread, captures frames at target FPS

#include "../models/contracts.h"
#include "../services/interfaces.h"
#include "../threading/shared_state.h"
#include <chrono>
#include <thread>

namespace sag {

class CameraWorker {
public:
    CameraWorker(ICameraService& camera, LatestValue<FramePacket>& output, int target_fps)
        : camera_(camera)
        , output_(output)
        , interval_ms_(target_fps > 0 ? 1000 / target_fps : 33)
    {}

    void set_interval_ms(int ms) { interval_ms_ = std::max(1, ms); }

    void start() {
        worker_.start([this]() { capture_loop(); });
    }

    void stop() {
        worker_.stop();
    }

private:
    void capture_loop() {
        auto start = std::chrono::steady_clock::now();

        FramePacket packet = camera_.next_frame();
        output_.set(std::move(packet));

        auto elapsed = std::chrono::steady_clock::now() - start;
        auto target  = std::chrono::milliseconds(interval_ms_.load());
        if (elapsed < target) {
            std::this_thread::sleep_for(target - elapsed);
        }
    }

    ICameraService&           camera_;
    LatestValue<FramePacket>& output_;
    std::atomic<int>          interval_ms_;
    WorkerThread              worker_;
};

} // namespace sag
