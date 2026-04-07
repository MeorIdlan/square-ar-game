#include <gtest/gtest.h>
#include "services/round_evaluator.h"
#include "models/game_session_model.h"

using namespace sag;

static GameSessionModel session_with_grid() {
    GameSessionModel session;
    session.grid = GridModel(2, 2);
    session.grid.cell_states[{0, 0}] = CellState::Green;
    session.grid.cell_states[{0, 1}] = CellState::Red;
    session.grid.cell_states[{1, 0}] = CellState::Green;
    session.grid.cell_states[{1, 1}] = CellState::Red;
    return session;
}

TEST(RoundEvaluatorTest, PlayerOnRedEliminated) {
    auto session = session_with_grid();
    session.round_state.participant_ids = {"P1"};
    session.players["P1"] = PlayerModel{.player_id = "P1", .tracking_state = PlayerTrackingState::Active};

    std::vector<MappedPlayerState> mapped = {{
        .player_id = "P1",
        .standing_point = Point{1.5f, 0.5f},
        .occupied_cell = CellIndex{0, 1},
        .in_bounds = true,
    }};

    RoundEvaluator evaluator;
    auto result = evaluator.evaluate(session, mapped);
    ASSERT_EQ(result.eliminated.size(), 1u);
    EXPECT_EQ(result.eliminated[0], "P1");
    EXPECT_NE(result.reason_map.at("P1").find("red"), std::string::npos);
}

TEST(RoundEvaluatorTest, PlayerOnGreenSurvives) {
    auto session = session_with_grid();
    session.round_state.participant_ids = {"P1"};
    session.players["P1"] = PlayerModel{.player_id = "P1", .tracking_state = PlayerTrackingState::Active};

    std::vector<MappedPlayerState> mapped = {{
        .player_id = "P1",
        .standing_point = Point{0.5f, 0.5f},
        .occupied_cell = CellIndex{0, 0},
        .in_bounds = true,
    }};

    RoundEvaluator evaluator;
    auto result = evaluator.evaluate(session, mapped);
    EXPECT_EQ(result.survivors.size(), 1u);
    EXPECT_EQ(result.eliminated.size(), 0u);
    EXPECT_EQ(result.survivors[0], "P1");
}

TEST(RoundEvaluatorTest, PlayerOutOfBoundsEliminated) {
    auto session = session_with_grid();
    session.round_state.participant_ids = {"P1"};
    session.players["P1"] = PlayerModel{.player_id = "P1", .tracking_state = PlayerTrackingState::Active};

    std::vector<MappedPlayerState> mapped = {{
        .player_id = "P1",
        .standing_point = Point{5.0f, 5.0f},
        .in_bounds = false,
    }};

    RoundEvaluator evaluator;
    auto result = evaluator.evaluate(session, mapped);
    ASSERT_EQ(result.eliminated.size(), 1u);
    EXPECT_NE(result.reason_map.at("P1").find("out of bounds"), std::string::npos);
}

TEST(RoundEvaluatorTest, AmbiguousPositionEliminated) {
    auto session = session_with_grid();
    session.round_state.participant_ids = {"P1"};
    session.players["P1"] = PlayerModel{.player_id = "P1", .tracking_state = PlayerTrackingState::Active};

    std::vector<MappedPlayerState> mapped = {{
        .player_id = "P1",
        .standing_point = Point{1.0f, 1.0f},
        .occupied_cell = std::nullopt,
        .in_bounds = true,
        .ambiguous = true,
    }};

    RoundEvaluator evaluator;
    auto result = evaluator.evaluate(session, mapped);
    ASSERT_EQ(result.eliminated.size(), 1u);
}

TEST(RoundEvaluatorTest, MissingPlayerEliminated) {
    auto session = session_with_grid();
    session.round_state.participant_ids = {"P1"};
    session.players["P1"] = PlayerModel{.player_id = "P1", .tracking_state = PlayerTrackingState::Missing};

    std::vector<MappedPlayerState> mapped = {{
        .player_id = "P1",
        .standing_point = std::nullopt,
        .in_bounds = false,
    }};

    RoundEvaluator evaluator;
    auto result = evaluator.evaluate(session, mapped);
    ASSERT_EQ(result.eliminated.size(), 1u);
    EXPECT_NE(result.reason_map.at("P1").find("missing"), std::string::npos);
}

TEST(RoundEvaluatorTest, PlayerNotInMappedEliminated) {
    auto session = session_with_grid();
    session.round_state.participant_ids = {"P1"};
    session.players["P1"] = PlayerModel{.player_id = "P1", .tracking_state = PlayerTrackingState::Active};

    RoundEvaluator evaluator;
    auto result = evaluator.evaluate(session, {});
    ASSERT_EQ(result.eliminated.size(), 1u);
    EXPECT_NE(result.reason_map.at("P1").find("not found"), std::string::npos);
}

TEST(RoundEvaluatorTest, AllPlayersEliminated) {
    auto session = session_with_grid();
    session.round_state.participant_ids = {"P1", "P2"};
    session.players["P1"] = PlayerModel{.player_id = "P1", .tracking_state = PlayerTrackingState::Active};
    session.players["P2"] = PlayerModel{.player_id = "P2", .tracking_state = PlayerTrackingState::Active};

    std::vector<MappedPlayerState> mapped = {
        {.player_id = "P1", .standing_point = Point{1.5f, 0.5f}, .occupied_cell = CellIndex{0, 1}, .in_bounds = true},
        {.player_id = "P2", .standing_point = Point{1.5f, 1.5f}, .occupied_cell = CellIndex{1, 1}, .in_bounds = true},
    };

    RoundEvaluator evaluator;
    auto result = evaluator.evaluate(session, mapped);
    EXPECT_EQ(result.eliminated.size(), 2u);
    EXPECT_EQ(result.survivors.size(), 0u);
}

TEST(RoundEvaluatorTest, MixedSurvivorsAndEliminated) {
    auto session = session_with_grid();
    session.round_state.participant_ids = {"P1", "P2"};
    session.players["P1"] = PlayerModel{.player_id = "P1", .tracking_state = PlayerTrackingState::Active};
    session.players["P2"] = PlayerModel{.player_id = "P2", .tracking_state = PlayerTrackingState::Active};

    std::vector<MappedPlayerState> mapped = {
        {.player_id = "P1", .standing_point = Point{0.5f, 0.5f}, .occupied_cell = CellIndex{0, 0}, .in_bounds = true},
        {.player_id = "P2", .standing_point = Point{1.5f, 1.5f}, .occupied_cell = CellIndex{1, 1}, .in_bounds = true},
    };

    RoundEvaluator evaluator;
    auto result = evaluator.evaluate(session, mapped);
    EXPECT_EQ(result.survivors.size(), 1u);
    EXPECT_EQ(result.eliminated.size(), 1u);
    EXPECT_EQ(result.survivors[0], "P1");
    EXPECT_EQ(result.eliminated[0], "P2");
}

TEST(RoundEvaluatorTest, NoOccupiedCellEliminated) {
    auto session = session_with_grid();
    session.round_state.participant_ids = {"P1"};
    session.players["P1"] = PlayerModel{.player_id = "P1", .tracking_state = PlayerTrackingState::Active};

    std::vector<MappedPlayerState> mapped = {{
        .player_id = "P1",
        .standing_point = Point{0.5f, 0.5f},
        .occupied_cell = std::nullopt,
        .in_bounds = true,
    }};

    RoundEvaluator evaluator;
    auto result = evaluator.evaluate(session, mapped);
    ASSERT_EQ(result.eliminated.size(), 1u);
    EXPECT_NE(result.reason_map.at("P1").find("no occupied cell"), std::string::npos);
}
