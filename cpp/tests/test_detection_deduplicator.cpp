#include <gtest/gtest.h>
#include "services/detection_deduplicator.h"

using namespace sag;

TEST(DeduplicatorTest, TwoFarApartBothKept) {
    std::vector<MappedPlayerState> detections = {
        {.player_id = "d1", .standing_point = Point{0.0f, 0.0f}, .in_bounds = true, .confidence = 0.9f},
        {.player_id = "d2", .standing_point = Point{5.0f, 5.0f}, .in_bounds = true, .confidence = 0.8f},
    };
    auto result = deduplicate_detections(detections, 0.35f);
    EXPECT_EQ(result.size(), 2u);
}

TEST(DeduplicatorTest, TwoCloseSameCellOneRemoved) {
    std::vector<MappedPlayerState> detections = {
        {.player_id = "d1", .standing_point = Point{1.0f, 1.0f}, .occupied_cell = CellIndex{0, 0}, .in_bounds = true, .confidence = 0.9f},
        {.player_id = "d2", .standing_point = Point{1.1f, 1.1f}, .occupied_cell = CellIndex{0, 0}, .in_bounds = true, .confidence = 0.8f},
    };
    auto result = deduplicate_detections(detections, 0.35f);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].player_id, "d1");
}

TEST(DeduplicatorTest, EmptyReturnsEmpty) {
    auto result = deduplicate_detections({}, 0.35f);
    EXPECT_TRUE(result.empty());
}

TEST(DeduplicatorTest, CloseByDistanceDifferentCells) {
    std::vector<MappedPlayerState> detections = {
        {.player_id = "d1", .standing_point = Point{1.0f, 1.0f}, .occupied_cell = CellIndex{0, 0}, .in_bounds = true, .confidence = 0.5f},
        {.player_id = "d2", .standing_point = Point{1.2f, 1.0f}, .occupied_cell = CellIndex{0, 1}, .in_bounds = true, .confidence = 0.6f},
    };
    auto result = deduplicate_detections(detections, 0.35f);
    EXPECT_EQ(result.size(), 1u);
}

TEST(DeduplicatorTest, NoneStandingPointKept) {
    std::vector<MappedPlayerState> detections = {
        {.player_id = "d1", .confidence = 0.0f},
    };
    auto result = deduplicate_detections(detections, 0.35f);
    EXPECT_EQ(result.size(), 1u);
}

TEST(DeduplicatorTest, HigherConfidencePreferred) {
    std::vector<MappedPlayerState> detections = {
        {.player_id = "d1", .standing_point = Point{1.0f, 1.0f}, .in_bounds = true, .confidence = 0.3f},
        {.player_id = "d2", .standing_point = Point{1.1f, 1.0f}, .in_bounds = true, .confidence = 0.9f},
    };
    auto result = deduplicate_detections(detections, 0.35f);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].player_id, "d2");
}
