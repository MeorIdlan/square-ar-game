#include <gtest/gtest.h>
#include "services/player_tracker_service.h"

using namespace sag;

TEST(PlayerTrackerTest, NewDetectionCreatesPlayer) {
    PlayerTrackerService tracker;
    std::unordered_map<std::string, PlayerModel> players;
    std::vector<MappedPlayerState> detections = {
        {.player_id = "det-1", .standing_point = Point{1.0f, 1.0f}, .confidence = 0.9f},
    };

    tracker.update_players(players, detections, 1.0, 0.35f);
    ASSERT_EQ(players.size(), 1u);
    EXPECT_TRUE(players.contains("P1"));
    EXPECT_EQ(players["P1"].tracking_state, PlayerTrackingState::Active);
    ASSERT_TRUE(players["P1"].standing_point.has_value());
    EXPECT_NEAR(players["P1"].standing_point->x, 1.0f, 0.01f);
}

TEST(PlayerTrackerTest, MatchesExistingPlayerByDistance) {
    PlayerTrackerService tracker;
    std::unordered_map<std::string, PlayerModel> players;
    players["P1"] = PlayerModel{
        .player_id = "P1",
        .tracking_state = PlayerTrackingState::Active,
        .standing_point = Point{1.0f, 1.0f},
        .last_seen_at = 0.5,
    };

    std::vector<MappedPlayerState> detections = {
        {.player_id = "det-1", .standing_point = Point{1.1f, 1.0f}, .confidence = 0.9f},
    };

    tracker.update_players(players, detections, 1.0, 0.35f);
    ASSERT_EQ(players.size(), 1u);
    EXPECT_TRUE(players.contains("P1"));
    EXPECT_NEAR(players["P1"].standing_point->x, 1.1f, 0.01f);
}

TEST(PlayerTrackerTest, FarDetectionCreatesNewPlayer) {
    PlayerTrackerService tracker;
    std::unordered_map<std::string, PlayerModel> players;
    players["P1"] = PlayerModel{
        .player_id = "P1",
        .tracking_state = PlayerTrackingState::Active,
        .standing_point = Point{1.0f, 1.0f},
        .last_seen_at = 0.5,
    };

    std::vector<MappedPlayerState> detections = {
        {.player_id = "det-1", .standing_point = Point{1.0f, 1.0f}, .confidence = 0.9f},
        {.player_id = "det-2", .standing_point = Point{5.0f, 5.0f}, .confidence = 0.8f},
    };

    tracker.update_players(players, detections, 1.0, 0.35f);
    ASSERT_EQ(players.size(), 2u);
    EXPECT_TRUE(players.contains("P1"));
    EXPECT_TRUE(players.contains("P2"));
}

TEST(PlayerTrackerTest, UnmatchedPlayerBecomesMissing) {
    PlayerTrackerService tracker;
    std::unordered_map<std::string, PlayerModel> players;
    players["P1"] = PlayerModel{
        .player_id = "P1",
        .tracking_state = PlayerTrackingState::Active,
        .standing_point = Point{1.0f, 1.0f},
        .last_seen_at = 0.0,
    };

    // No detections and timestamp exceeds grace period
    tracker.update_players(players, {}, 0.50, 0.35f);
    EXPECT_EQ(players["P1"].tracking_state, PlayerTrackingState::Missing);
    EXPECT_FALSE(players["P1"].standing_point.has_value());
}

TEST(PlayerTrackerTest, GracePeriodPreventsMissing) {
    PlayerTrackerService tracker;
    std::unordered_map<std::string, PlayerModel> players;
    players["P1"] = PlayerModel{
        .player_id = "P1",
        .tracking_state = PlayerTrackingState::Active,
        .standing_point = Point{1.0f, 1.0f},
        .last_seen_at = 0.0,
    };

    // Within grace period
    tracker.update_players(players, {}, 0.20, 0.35f);
    EXPECT_EQ(players["P1"].tracking_state, PlayerTrackingState::Active);
    EXPECT_TRUE(players["P1"].standing_point.has_value());
}

TEST(PlayerTrackerTest, EliminatedPlayersNotMatched) {
    PlayerTrackerService tracker;
    std::unordered_map<std::string, PlayerModel> players;
    players["P1"] = PlayerModel{
        .player_id = "P1",
        .tracking_state = PlayerTrackingState::Eliminated,
        .standing_point = Point{1.0f, 1.0f},
        .last_seen_at = 0.5,
    };

    std::vector<MappedPlayerState> detections = {
        {.player_id = "det-1", .standing_point = Point{1.0f, 1.0f}, .confidence = 0.9f},
    };

    tracker.update_players(players, detections, 1.0, 0.35f);
    // Should create new player instead of matching eliminated one
    EXPECT_EQ(players.size(), 2u);
    EXPECT_TRUE(players.contains("P2"));
}

TEST(PlayerTrackerTest, PlayerIdAllocation) {
    PlayerTrackerService tracker;
    std::unordered_map<std::string, PlayerModel> players;

    // Add 3 detections at once
    std::vector<MappedPlayerState> detections = {
        {.player_id = "d1", .standing_point = Point{0.0f, 0.0f}, .confidence = 0.9f},
        {.player_id = "d2", .standing_point = Point{3.0f, 0.0f}, .confidence = 0.8f},
        {.player_id = "d3", .standing_point = Point{6.0f, 0.0f}, .confidence = 0.7f},
    };

    tracker.update_players(players, detections, 1.0, 0.35f);
    EXPECT_EQ(players.size(), 3u);
    EXPECT_TRUE(players.contains("P1"));
    EXPECT_TRUE(players.contains("P2"));
    EXPECT_TRUE(players.contains("P3"));
}

TEST(PlayerTrackerTest, NoStandingPointSkipped) {
    PlayerTrackerService tracker;
    std::unordered_map<std::string, PlayerModel> players;

    std::vector<MappedPlayerState> detections = {
        {.player_id = "d1"},  // no standing point
    };

    tracker.update_players(players, detections, 1.0, 0.35f);
    EXPECT_EQ(players.size(), 0u);
}
