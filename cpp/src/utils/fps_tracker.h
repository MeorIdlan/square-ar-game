#pragma once
// FPS tracker — maintains a rolling window of frame timestamps

#include <chrono>
#include <deque>

namespace sag
{

    class FpsTracker
    {
    public:
        using clock = std::chrono::steady_clock;
        using time_point = clock::time_point;

        explicit FpsTracker(int window_size = 30) : window_size_(window_size) {}

        // Record a frame and return current FPS estimate
        float record_frame()
        {
            return record_frame_at(clock::now());
        }

        // Record a frame at a specific time point (for testing)
        float record_frame_at(time_point tp)
        {
            timestamps_.push_back(tp);
            while (static_cast<int>(timestamps_.size()) > window_size_)
            {
                timestamps_.pop_front();
            }
            return current_fps();
        }

        float current_fps() const
        {
            if (timestamps_.size() < 2)
                return 0.0f;
            auto duration = std::chrono::duration<float>(
                                timestamps_.back() - timestamps_.front())
                                .count();
            if (duration <= 0.0f)
                return 0.0f;
            return static_cast<float>(timestamps_.size() - 1) / duration;
        }

        void reset() { timestamps_.clear(); }

    private:
        int window_size_;
        std::deque<time_point> timestamps_;
    };

} // namespace sag
