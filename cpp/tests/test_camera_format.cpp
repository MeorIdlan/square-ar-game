#include <gtest/gtest.h>
#include "utils/config.h"
#include "services/camera_service.h"

using namespace sag;

// ─── CameraProfile::label() ───────────────────────────────────────────────────

TEST(CameraProfileLabel, Returns720pLabel)
{
    CameraProfile p{1280, 720, 30};
    EXPECT_EQ(p.label(), "720p 30 FPS");
}

TEST(CameraProfileLabel, Returns1080pLabel)
{
    CameraProfile p{1920, 1080, 60};
    EXPECT_EQ(p.label(), "1080p 60 FPS");
}

TEST(CameraProfileLabel, Returns4KLabel)
{
    CameraProfile p{3840, 2160, 30};
    EXPECT_EQ(p.label(), "30 FPS 4K");
}

TEST(CameraProfileLabel, Returns480pLabel)
{
    CameraProfile p{640, 480, 30};
    EXPECT_EQ(p.label(), "480p 30 FPS");
}

// ─── normalize_camera_formats() ───────────────────────────────────────────────

TEST(NormalizeCameraFormats, EmptyInputReturnsEmpty)
{
    EXPECT_TRUE(normalize_camera_formats({}).empty());
}

TEST(NormalizeCameraFormats, RemovesExactDuplicates)
{
    std::vector<CameraProfile> input = {
        {1280, 720, 30},
        {1280, 720, 30},
        {1920, 1080, 30},
    };
    auto result = normalize_camera_formats(input);
    ASSERT_EQ(result.size(), 2u);
}

TEST(NormalizeCameraFormats, SortsByPixelCountDescending)
{
    std::vector<CameraProfile> input = {
        {640, 480, 30},
        {1920, 1080, 30},
        {1280, 720, 30},
    };
    auto result = normalize_camera_formats(input);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].width, 1920);
    EXPECT_EQ(result[1].width, 1280);
    EXPECT_EQ(result[2].width, 640);
}

TEST(NormalizeCameraFormats, SortsByFpsDescendingForSameResolution)
{
    std::vector<CameraProfile> input = {
        {1920, 1080, 30},
        {1920, 1080, 60},
        {1920, 1080, 15},
    };
    auto result = normalize_camera_formats(input);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0].fps, 60);
    EXPECT_EQ(result[1].fps, 30);
    EXPECT_EQ(result[2].fps, 15);
}

TEST(NormalizeCameraFormats, FiltersOutInvalidEntries)
{
    std::vector<CameraProfile> input = {
        {0, 480, 30},    // zero width
        {640, 0, 30},    // zero height
        {640, 480, 0},   // zero fps
        {1280, 720, 30}, // valid
    };
    auto result = normalize_camera_formats(input);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].width, 1280);
}

TEST(NormalizeCameraFormats, MixedResolutionsAndFpsAreSortedCorrectly)
{
    std::vector<CameraProfile> input = {
        {1280, 720, 30},
        {1920, 1080, 30},
        {1920, 1080, 60},
        {640, 480, 30},
        {1280, 720, 60},
    };
    auto result = normalize_camera_formats(input);
    ASSERT_EQ(result.size(), 5u);
    // Highest pixel count first
    EXPECT_EQ(result[0].height, 1080);
    EXPECT_EQ(result[0].fps, 60);
    EXPECT_EQ(result[1].height, 1080);
    EXPECT_EQ(result[1].fps, 30);
    EXPECT_EQ(result[2].height, 720);
    EXPECT_EQ(result[2].fps, 60);
    EXPECT_EQ(result[3].height, 720);
    EXPECT_EQ(result[3].fps, 30);
    EXPECT_EQ(result[4].height, 480);
}
