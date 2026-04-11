#include <gtest/gtest.h>
#include "utils/fps_tracker.h"

using namespace sag;

TEST(FpsTracker, NoFramesReturnsZero)
{
    FpsTracker tracker;
    EXPECT_FLOAT_EQ(tracker.current_fps(), 0.0f);
}

TEST(FpsTracker, SingleFrameReturnsZero)
{
    FpsTracker tracker;
    auto t = FpsTracker::time_point{};
    tracker.record_frame_at(t);
    EXPECT_FLOAT_EQ(tracker.current_fps(), 0.0f);
}

TEST(FpsTracker, TwoFramesAtThirtyFps)
{
    FpsTracker tracker(10);
    auto t0 = FpsTracker::time_point{};
    auto t1 = t0 + std::chrono::microseconds(33333); // ~30fps
    tracker.record_frame_at(t0);
    float fps = tracker.record_frame_at(t1);
    EXPECT_NEAR(fps, 30.0f, 1.0f);
}

TEST(FpsTracker, SteadyStreamAtSixtyFps)
{
    FpsTracker tracker(60);
    auto t = FpsTracker::time_point{};
    float fps = 0.0f;
    for (int i = 0; i < 60; ++i)
    {
        fps = tracker.record_frame_at(t);
        t += std::chrono::microseconds(16667); // ~60fps interval
    }
    EXPECT_NEAR(fps, 60.0f, 1.0f);
}

TEST(FpsTracker, WindowSizeLimitsHistory)
{
    FpsTracker tracker(5);
    auto t = FpsTracker::time_point{};
    // Feed 10 frames at 10fps — only last 5 should be in the window
    for (int i = 0; i < 10; ++i)
    {
        tracker.record_frame_at(t);
        t += std::chrono::milliseconds(100);
    }
    float fps = tracker.current_fps();
    EXPECT_NEAR(fps, 10.0f, 0.5f);
}

TEST(FpsTracker, ResetClearsHistory)
{
    FpsTracker tracker;
    auto t = FpsTracker::time_point{};
    tracker.record_frame_at(t);
    tracker.record_frame_at(t + std::chrono::milliseconds(33));
    EXPECT_GT(tracker.current_fps(), 0.0f);
    tracker.reset();
    EXPECT_FLOAT_EQ(tracker.current_fps(), 0.0f);
}

TEST(FpsTracker, VariableFrameRateConverges)
{
    FpsTracker tracker(10);
    auto t = FpsTracker::time_point{};
    // Start at 30fps for 5 frames, then switch to 60fps for 10 frames
    for (int i = 0; i < 5; ++i)
    {
        tracker.record_frame_at(t);
        t += std::chrono::microseconds(33333);
    }
    float fps_30 = tracker.current_fps();
    EXPECT_NEAR(fps_30, 30.0f, 1.0f);

    // Switch to 60fps — after enough frames, the window should reflect it
    for (int i = 0; i < 10; ++i)
    {
        tracker.record_frame_at(t);
        t += std::chrono::microseconds(16667);
    }
    float fps_60 = tracker.current_fps();
    EXPECT_NEAR(fps_60, 60.0f, 2.0f);
}

TEST(FpsTracker, IdenticalTimestampsReturnZero)
{
    FpsTracker tracker(5);
    auto t = FpsTracker::time_point{};
    tracker.record_frame_at(t);
    float fps = tracker.record_frame_at(t); // same timestamp
    EXPECT_FLOAT_EQ(fps, 0.0f);
}
