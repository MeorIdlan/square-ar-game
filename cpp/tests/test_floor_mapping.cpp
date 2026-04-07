#include <gtest/gtest.h>
#include "services/floor_mapping_service.h"

using namespace sag;

static CalibrationModel make_identity_calibration() {
    CalibrationModel cal;
    cal.state = CalibrationState::Valid;
    cal.homography = cv::Mat::eye(3, 3, CV_64F); // identity → pixel == floor
    cal.inverse_homography = cv::Mat::eye(3, 3, CV_64F);
    cal.playable_bounds = Quad{Point{0, 0}, Point{4, 0}, Point{4, 4}, Point{0, 4}};
    return cal;
}

TEST(FloorMappingTest, IdentityHomographyMapsDirectly) {
    FloorMappingService service;
    auto cal = make_identity_calibration();

    auto result = service.map_image_point_to_floor(Point{1.5f, 2.5f}, cal);
    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(result->x, 1.5f, 0.01f);
    EXPECT_NEAR(result->y, 2.5f, 0.01f);
}

TEST(FloorMappingTest, NullPointReturnsNullopt) {
    FloorMappingService service;
    auto cal = make_identity_calibration();

    auto result = service.map_image_point_to_floor(std::nullopt, cal);
    EXPECT_FALSE(result.has_value());
}

TEST(FloorMappingTest, EmptyHomographyReturnsNullopt) {
    FloorMappingService service;
    CalibrationModel cal;

    auto result = service.map_image_point_to_floor(Point{1.0f, 1.0f}, cal);
    EXPECT_FALSE(result.has_value());
}

TEST(FloorMappingTest, ResolveCellCenterOfGrid) {
    FloorMappingService service;
    auto cal = make_identity_calibration();
    GridModel grid(4, 4);

    auto cell = service.resolve_cell(Point{0.5f, 0.5f}, grid, cal);
    ASSERT_TRUE(cell.has_value());
    EXPECT_EQ(cell->row, 0);
    EXPECT_EQ(cell->col, 0);
}

TEST(FloorMappingTest, ResolveCellBottomRight) {
    FloorMappingService service;
    auto cal = make_identity_calibration();
    GridModel grid(4, 4);

    auto cell = service.resolve_cell(Point{3.5f, 3.5f}, grid, cal);
    ASSERT_TRUE(cell.has_value());
    EXPECT_EQ(cell->row, 3);
    EXPECT_EQ(cell->col, 3);
}

TEST(FloorMappingTest, ResolveCellOnBoundaryReturnsNullopt) {
    FloorMappingService service;
    auto cal = make_identity_calibration();
    GridModel grid(4, 4);

    // Exactly on cell boundary at x=1.0
    auto cell = service.resolve_cell(Point{1.0f, 0.5f}, grid, cal);
    EXPECT_FALSE(cell.has_value());
}

TEST(FloorMappingTest, OutsidePlayableAreaReturnsNullopt) {
    FloorMappingService service;
    auto cal = make_identity_calibration();
    GridModel grid(4, 4);

    auto cell = service.resolve_cell(Point{5.0f, 5.0f}, grid, cal);
    EXPECT_FALSE(cell.has_value());
}

TEST(FloorMappingTest, IsInsidePlayableArea) {
    FloorMappingService service;
    auto cal = make_identity_calibration();

    EXPECT_TRUE(service.is_inside_playable_area(Point{2.0f, 2.0f}, cal));
    EXPECT_FALSE(service.is_inside_playable_area(Point{-0.1f, 2.0f}, cal));
    EXPECT_FALSE(service.is_inside_playable_area(Point{2.0f, 4.1f}, cal));
}

TEST(FloorMappingTest, MapDetectionFullPipeline) {
    FloorMappingService service;
    auto cal = make_identity_calibration();
    GridModel grid(4, 4);

    PoseFootState pose;
    pose.left_foot = Point{0.3f, 0.3f};
    pose.right_foot = Point{0.7f, 0.7f};
    pose.confidence = 0.9f;

    auto mapped = service.map_detection("P1", pose, grid, cal);
    EXPECT_EQ(mapped.player_id, "P1");
    ASSERT_TRUE(mapped.standing_point.has_value());
    EXPECT_NEAR(mapped.standing_point->x, 0.5f, 0.01f);
    EXPECT_NEAR(mapped.standing_point->y, 0.5f, 0.01f);
    ASSERT_TRUE(mapped.occupied_cell.has_value());
    EXPECT_EQ(mapped.occupied_cell->row, 0);
    EXPECT_EQ(mapped.occupied_cell->col, 0);
    EXPECT_TRUE(mapped.in_bounds);
    EXPECT_FALSE(mapped.ambiguous);
}

TEST(FloorMappingTest, MapDetectionNoFeetReturnsNoBounds) {
    FloorMappingService service;
    auto cal = make_identity_calibration();
    GridModel grid(4, 4);

    PoseFootState pose;
    auto mapped = service.map_detection("P1", pose, grid, cal);
    EXPECT_FALSE(mapped.standing_point.has_value());
    EXPECT_FALSE(mapped.in_bounds);
    EXPECT_FALSE(mapped.ambiguous);
}

TEST(FloorMappingTest, NoBoundsReturnsFalseForInsideCheck) {
    FloorMappingService service;
    CalibrationModel cal;
    cal.state = CalibrationState::Valid;

    EXPECT_FALSE(service.is_inside_playable_area(Point{1.0f, 1.0f}, cal));
}
