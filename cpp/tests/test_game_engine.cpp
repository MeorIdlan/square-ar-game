#include <gtest/gtest.h>
#include "services/game_engine_service.h"

using namespace sag;

static GameSessionModel make_session_with_players() {
    GameSessionModel session;
    session.app_state = AppState::Calibrated;
    session.grid = GridModel(4, 4);

    session.players["P1"] = PlayerModel{
        .player_id = "P1",
        .tracking_state = PlayerTrackingState::Active,
        .occupied_cell = CellIndex{0, 0},
    };
    session.players["P2"] = PlayerModel{
        .player_id = "P2",
        .tracking_state = PlayerTrackingState::Active,
        .occupied_cell = CellIndex{1, 1},
    };
    return session;
}

// ── start_round ──────────────────────────────────────────────────────

TEST(GameEngineTest, StartRoundWithTwoPlayers) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    engine.start_round(session);

    EXPECT_EQ(session.round_state.phase, RoundPhase::Flashing);
    EXPECT_EQ(session.app_state, AppState::Running);
    EXPECT_EQ(session.round_state.round_number, 1);
    EXPECT_EQ(session.round_state.participant_ids.size(), 2u);
    EXPECT_GT(session.round_state.timer_remaining, 0.0f);
}

TEST(GameEngineTest, StartRoundNoPlayersRejected) {
    GameEngineService engine(42);
    GameSessionModel session;
    engine.start_round(session);

    EXPECT_EQ(session.round_state.phase, RoundPhase::Idle);
    EXPECT_EQ(session.status_message, "No active participants in valid cells");
}

TEST(GameEngineTest, StartRoundOneCandidateImmediateWinner) {
    GameEngineService engine(42);
    GameSessionModel session;
    session.players["P1"] = PlayerModel{
        .player_id = "P1",
        .tracking_state = PlayerTrackingState::Active,
        .occupied_cell = CellIndex{0, 0},
    };

    engine.start_round(session);
    EXPECT_EQ(session.app_state, AppState::Finished);
    EXPECT_EQ(session.round_state.phase, RoundPhase::Finished);
    EXPECT_TRUE(session.winner_id.has_value());
    EXPECT_EQ(*session.winner_id, "P1");
}

// ── Pattern randomization ────────────────────────────────────────────

TEST(GameEngineTest, RandomizeFlashingPatternSetsGreenOrRed) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    engine.randomize_flashing_pattern(session);

    for (auto& [idx, state] : session.grid.cell_states) {
        EXPECT_TRUE(state == CellState::Green || state == CellState::Red);
    }
}

// ── lock_round ───────────────────────────────────────────────────────

TEST(GameEngineTest, LockRoundSetsLockedPhase) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    engine.start_round(session);
    engine.lock_round(session);

    EXPECT_EQ(session.round_state.phase, RoundPhase::Locked);
    EXPECT_GT(session.round_state.timer_remaining, 0.0f);
}

// ── Tick (full lifecycle) ────────────────────────────────────────────

TEST(GameEngineTest, TickFlashingToLocked) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    engine.start_round(session);
    ASSERT_EQ(session.round_state.phase, RoundPhase::Flashing);

    // Tick past flashing duration
    engine.tick(session, 3.1f);
    EXPECT_EQ(session.round_state.phase, RoundPhase::Locked);
}

TEST(GameEngineTest, TickLockedToChecking) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    engine.start_round(session);
    engine.tick(session, 3.1f);  // → Locked
    ASSERT_EQ(session.round_state.phase, RoundPhase::Locked);

    engine.tick(session, 1.6f);  // → Checking
    EXPECT_EQ(session.round_state.phase, RoundPhase::Checking);
}

TEST(GameEngineTest, TickCheckingToResults) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    engine.start_round(session);
    engine.tick(session, 3.1f);  // → Locked
    engine.tick(session, 1.6f);  // → Checking
    ASSERT_EQ(session.round_state.phase, RoundPhase::Checking);

    engine.tick(session, 0.3f);  // → Results
    EXPECT_EQ(session.round_state.phase, RoundPhase::Results);
}

TEST(GameEngineTest, TickIdleDoesNothing) {
    GameEngineService engine(42);
    GameSessionModel session;
    session.round_state.phase = RoundPhase::Idle;
    engine.tick(session, 1.0f);
    EXPECT_EQ(session.round_state.phase, RoundPhase::Idle);
}

TEST(GameEngineTest, TickFinishedDoesNothing) {
    GameEngineService engine(42);
    GameSessionModel session;
    session.round_state.phase = RoundPhase::Finished;
    engine.tick(session, 1.0f);
    EXPECT_EQ(session.round_state.phase, RoundPhase::Finished);
}

// ── evaluate_round ───────────────────────────────────────────────────

TEST(GameEngineTest, EvaluateRoundWithMappedPlayers) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    engine.start_round(session);

    // Set cells: P1 on green, P2 on red
    session.grid.cell_states[{0, 0}] = CellState::Green;
    session.grid.cell_states[{1, 1}] = CellState::Red;

    std::vector<MappedPlayerState> mapped = {
        {.player_id = "P1", .standing_point = Point{0.5f, 0.5f}, .occupied_cell = CellIndex{0, 0}, .in_bounds = true},
        {.player_id = "P2", .standing_point = Point{1.5f, 1.5f}, .occupied_cell = CellIndex{1, 1}, .in_bounds = true},
    };

    engine.evaluate_round(session, &mapped);
    EXPECT_EQ(session.round_state.phase, RoundPhase::Results);

    auto& p1 = session.players["P1"];
    auto& p2 = session.players["P2"];
    EXPECT_FALSE(p1.eliminated_in_round);
    EXPECT_TRUE(p2.eliminated_in_round);
    EXPECT_EQ(p2.tracking_state, PlayerTrackingState::Eliminated);
}

TEST(GameEngineTest, EvaluateRoundSingleSurvivorWins) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    engine.start_round(session);

    // Both on red but only 2 players, one survives → winner
    session.grid.cell_states[{0, 0}] = CellState::Green;
    session.grid.cell_states[{1, 1}] = CellState::Red;

    std::vector<MappedPlayerState> mapped = {
        {.player_id = "P1", .standing_point = Point{0.5f, 0.5f}, .occupied_cell = CellIndex{0, 0}, .in_bounds = true},
        {.player_id = "P2", .standing_point = Point{1.5f, 1.5f}, .occupied_cell = CellIndex{1, 1}, .in_bounds = true},
    };

    engine.evaluate_round(session, &mapped);
    EXPECT_EQ(session.app_state, AppState::Finished);
    EXPECT_TRUE(session.winner_id.has_value());
    EXPECT_EQ(*session.winner_id, "P1");
}

// ── Operator overrides ───────────────────────────────────────────────

TEST(GameEngineTest, RevivePlayer) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    session.players["P1"].tracking_state = PlayerTrackingState::Eliminated;
    session.players["P1"].eliminated_in_round = true;
    session.players["P1"].standing_point = Point{0.5f, 0.5f};
    session.round_state.participant_ids = {"P1", "P2"};
    session.round_state.eliminated_ids = {"P1"};
    session.round_state.survivor_ids = {"P2"};

    engine.revive_player(session, "P1");
    EXPECT_FALSE(session.players["P1"].eliminated_in_round);
    EXPECT_EQ(session.players["P1"].tracking_state, PlayerTrackingState::Active);
}

TEST(GameEngineTest, EliminatePlayer) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    session.round_state.participant_ids = {"P1", "P2"};
    session.round_state.survivor_ids = {"P1", "P2"};

    engine.eliminate_player(session, "P1");
    EXPECT_TRUE(session.players["P1"].eliminated_in_round);
    EXPECT_EQ(session.players["P1"].tracking_state, PlayerTrackingState::Eliminated);
}

TEST(GameEngineTest, EliminateToWinner) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    session.round_state.participant_ids = {"P1", "P2"};
    session.round_state.survivor_ids = {"P1", "P2"};

    engine.eliminate_player(session, "P1");
    // P2 should now be the winner
    EXPECT_EQ(session.app_state, AppState::Finished);
    EXPECT_TRUE(session.winner_id.has_value());
    EXPECT_EQ(*session.winner_id, "P2");
}

TEST(GameEngineTest, RemovePlayer) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    session.round_state.participant_ids = {"P1", "P2"};

    engine.remove_player(session, "P1");
    EXPECT_FALSE(session.players.contains("P1"));
    EXPECT_EQ(session.players.size(), 1u);
}

// ── reset_session ────────────────────────────────────────────────────

TEST(GameEngineTest, ResetSession) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    engine.start_round(session);
    engine.tick(session, 10.0f); // run through phases
    engine.reset_session(session);

    EXPECT_EQ(session.round_state.phase, RoundPhase::Idle);
    EXPECT_EQ(session.round_state.timer_remaining, 0.0f);
    EXPECT_TRUE(session.round_state.participant_ids.empty());
    EXPECT_TRUE(session.round_state.survivor_ids.empty());
    EXPECT_TRUE(session.round_state.eliminated_ids.empty());
    EXPECT_FALSE(session.winner_id.has_value());
}

TEST(GameEngineTest, ResetSessionResetsEliminatedToMissing) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    session.players["P1"].tracking_state = PlayerTrackingState::Eliminated;
    engine.reset_session(session);
    EXPECT_EQ(session.players["P1"].tracking_state, PlayerTrackingState::Missing);
}

// ── build_render_state ───────────────────────────────────────────────

TEST(GameEngineTest, BuildRenderState) {
    GameEngineService engine(42);
    auto session = make_session_with_players();
    session.round_state.phase = RoundPhase::Flashing;
    session.round_state.timer_remaining = 2.5f;

    auto rs = engine.build_render_state(session);
    EXPECT_EQ(rs.phase, RoundPhase::Flashing);
    EXPECT_EQ(rs.timer_text, "2.5");
    EXPECT_EQ(rs.grid_cells.size(), session.grid.cell_states.size());
}

// ── Full lifecycle integration ───────────────────────────────────────

TEST(GameEngineTest, FullLifecycleToWinner) {
    GameEngineService engine(42);

    GameSessionModel session;
    session.app_state = AppState::Calibrated;
    session.grid = GridModel(2, 2);

    session.players["P1"] = PlayerModel{
        .player_id = "P1",
        .tracking_state = PlayerTrackingState::Active,
        .standing_point = Point{0.5f, 0.5f},
        .occupied_cell = CellIndex{0, 0},
    };
    session.players["P2"] = PlayerModel{
        .player_id = "P2",
        .tracking_state = PlayerTrackingState::Active,
        .standing_point = Point{1.5f, 1.5f},
        .occupied_cell = CellIndex{1, 1},
    };

    // Start round
    engine.start_round(session);
    EXPECT_EQ(session.round_state.phase, RoundPhase::Flashing);

    // Progress through flashing → locked → checking
    engine.tick(session, 3.1f);  // → Locked
    engine.tick(session, 1.6f);  // → Checking

    // Before evaluation, set up definitive cell state
    session.grid.cell_states[{0, 0}] = CellState::Green;
    session.grid.cell_states[{0, 1}] = CellState::Green;
    session.grid.cell_states[{1, 0}] = CellState::Red;
    session.grid.cell_states[{1, 1}] = CellState::Red;

    std::vector<MappedPlayerState> mapped = {
        {.player_id = "P1", .standing_point = Point{0.5f, 0.5f}, .occupied_cell = CellIndex{0, 0}, .in_bounds = true},
        {.player_id = "P2", .standing_point = Point{1.5f, 1.5f}, .occupied_cell = CellIndex{1, 1}, .in_bounds = true},
    };

    engine.evaluate_round(session, &mapped);

    // P2 eliminated (on red), P1 is the winner
    EXPECT_EQ(session.app_state, AppState::Finished);
    EXPECT_TRUE(session.winner_id.has_value());
    EXPECT_EQ(*session.winner_id, "P1");
    EXPECT_TRUE(session.players["P2"].eliminated_in_round);
}
