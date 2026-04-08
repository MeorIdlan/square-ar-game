#include "services/game_engine_service.h"

#include <algorithm>
#include <format>

namespace sag {

GameEngineService::GameEngineService(std::optional<unsigned> seed)
    : rng_(seed.value_or(std::random_device{}())) {}

// ── Round lifecycle ──────────────────────────────────────────────────

void GameEngineService::start_round(GameSessionModel& session) {
    std::vector<std::string> active_candidates;
    for (auto& [id, player] : session.players) {
        if (player.tracking_state == PlayerTrackingState::Active
            && player.occupied_cell.has_value()) {
            active_candidates.push_back(id);
        }
    }

    if (active_candidates.empty()) {
        session.round_state.phase = RoundPhase::Idle;
        session.round_state.timer_remaining = 0.0f;
        session.status_message = "No active participants in valid cells";
        return;
    }

    if (active_candidates.size() == 1) {
        session.winner_id = active_candidates[0];
        session.app_state = AppState::Finished;
        session.round_state.phase = RoundPhase::Finished;
        session.round_state.timer_remaining = 0.0f;
        session.status_message = "Winner: " + *session.winner_id;
        return;
    }

    auto& rs = session.round_state;
    rs.round_number += 1;
    rs.phase = RoundPhase::Flashing;
    rs.timer_remaining = rs.timings.flashing_duration_seconds;
    rs.flash_elapsed_seconds = 0.0f;
    rs.participant_ids.clear();
    rs.survivor_ids.clear();
    rs.eliminated_ids.clear();

    for (auto& [id, player] : session.players) {
        player.active_in_round =
            (player.tracking_state == PlayerTrackingState::Active
             && player.occupied_cell.has_value());
        player.eliminated_in_round = false;
        if (player.active_in_round)
            rs.participant_ids.push_back(id);
    }

    session.app_state = AppState::Running;
    randomize_flashing_pattern(session);
    session.status_message = std::format("Round {} flashing", rs.round_number);
}

void GameEngineService::randomize_flashing_pattern(GameSessionModel& session) {
    std::uniform_int_distribution<int> dist(0, 1);
    for (auto& [idx, state] : session.grid.cell_states) {
        state = dist(rng_) ? CellState::Green : CellState::Red;
    }
}

void GameEngineService::lock_round(GameSessionModel& session) {
    session.round_state.phase = RoundPhase::Locked;
    session.round_state.timer_remaining = session.round_state.timings.reaction_window_seconds;

    // Final pattern randomization on lock
    std::uniform_int_distribution<int> dist(0, 1);
    for (auto& [idx, state] : session.grid.cell_states) {
        state = dist(rng_) ? CellState::Green : CellState::Red;
    }
    session.status_message = std::format("Round {} locked", session.round_state.round_number);
}

void GameEngineService::begin_check(GameSessionModel& session) {
    session.round_state.phase = RoundPhase::Checking;
    session.round_state.timer_remaining = session.round_state.timings.lock_delay_seconds;
    session.status_message = std::format("Round {} checking positions", session.round_state.round_number);
}

void GameEngineService::begin_preparing_next_round(GameSessionModel& session) {
    session.round_state.phase = RoundPhase::Preparing;
    session.round_state.timer_remaining = session.round_state.timings.inter_round_delay_seconds;
    session.status_message = std::format("Next round in {:.1f}s", session.round_state.timer_remaining);
}

// ── Tick (timer-driven state machine) ────────────────────────────────

void GameEngineService::tick(GameSessionModel& session, float delta_seconds) {
    auto& rs = session.round_state;
    if (rs.phase == RoundPhase::Idle || rs.phase == RoundPhase::Finished)
        return;

    rs.timer_remaining = std::max(0.0f, rs.timer_remaining - delta_seconds);

    switch (rs.phase) {
    case RoundPhase::Flashing: {
        rs.flash_elapsed_seconds += delta_seconds;
        float interval = std::max(rs.timings.flash_interval_seconds, 0.05f);
        while (rs.flash_elapsed_seconds >= interval && rs.timer_remaining > 0.0f) {
            rs.flash_elapsed_seconds -= interval;
            randomize_flashing_pattern(session);
        }
        if (rs.timer_remaining <= 0.0f) {
            lock_round(session);
        } else {
            session.status_message = std::format("Round {} flashing", rs.round_number);
        }
        break;
    }
    case RoundPhase::Locked:
        if (rs.timer_remaining <= 0.0f)
            begin_check(session);
        else
            session.status_message = std::format("Round {} locked", rs.round_number);
        break;

    case RoundPhase::Checking:
        if (rs.timer_remaining <= 0.0f)
            evaluate_round(session);
        else
            session.status_message = std::format("Round {} checking positions", rs.round_number);
        break;

    case RoundPhase::Results:
        if (rs.timer_remaining <= 0.0f) {
            if (session.app_state == AppState::Finished)
                rs.phase = RoundPhase::Finished;
            else
                begin_preparing_next_round(session);
        }
        break;

    case RoundPhase::Preparing:
        if (rs.timer_remaining <= 0.0f)
            start_round(session);
        else
            session.status_message = std::format("Next round in {:.1f}s", rs.timer_remaining);
        break;

    default:
        break;
    }
}

// ── Evaluation ───────────────────────────────────────────────────────

std::vector<MappedPlayerState> GameEngineService::current_mapped_players(
    const GameSessionModel& session) const
{
    std::vector<MappedPlayerState> result;
    for (auto& [id, player] : session.players) {
        MappedPlayerState mp;
        mp.player_id = player.player_id;
        mp.standing_point = player.standing_point;
        mp.left_foot = player.left_foot;
        mp.right_foot = player.right_foot;
        mp.occupied_cell = player.occupied_cell;
        mp.in_bounds = player.occupied_cell.has_value();
        mp.ambiguous = !player.occupied_cell.has_value() && player.standing_point.has_value();
        mp.confidence = player.confidence;
        mp.last_seen_at = player.last_seen_at;
        result.push_back(std::move(mp));
    }
    return result;
}

void GameEngineService::evaluate_round(
    GameSessionModel& session,
    const std::vector<MappedPlayerState>* mapped_players)
{
    auto players_list = mapped_players
        ? *mapped_players
        : current_mapped_players(session);

    auto& rs = session.round_state;
    rs.phase = RoundPhase::Results;
    rs.timer_remaining = rs.timings.elimination_display_seconds;

    auto evaluation = evaluator_.evaluate(session, players_list);
    rs.survivor_ids   = std::move(evaluation.survivors);
    rs.eliminated_ids = std::move(evaluation.eliminated);

    for (auto& participant_id : rs.participant_ids) {
        auto it = session.players.find(participant_id);
        if (it == session.players.end()) continue;

        bool eliminated = std::find(rs.eliminated_ids.begin(), rs.eliminated_ids.end(), participant_id)
                       != rs.eliminated_ids.end();
        it->second.eliminated_in_round = eliminated;
        it->second.active_in_round = !eliminated;
        it->second.tracking_state = eliminated
            ? PlayerTrackingState::Eliminated
            : PlayerTrackingState::Active;
    }

    refresh_round_outcome(session);
}

// ── Operator overrides ───────────────────────────────────────────────

void GameEngineService::revive_player(GameSessionModel& session, const std::string& player_id) {
    auto it = session.players.find(player_id);
    if (it == session.players.end()) return;

    auto& player = it->second;
    player.eliminated_in_round = false;
    player.active_in_round = true;
    player.tracking_state = player.standing_point
        ? PlayerTrackingState::Active
        : PlayerTrackingState::Missing;

    ensure_membership(session.round_state.survivor_ids, player_id);
    remove_membership(session.round_state.eliminated_ids, player_id);
    ensure_membership(session.round_state.participant_ids, player_id);
    refresh_round_outcome(session);
}

void GameEngineService::eliminate_player(GameSessionModel& session, const std::string& player_id) {
    auto it = session.players.find(player_id);
    if (it == session.players.end()) return;

    auto& player = it->second;
    player.eliminated_in_round = true;
    player.active_in_round = false;
    player.tracking_state = PlayerTrackingState::Eliminated;

    ensure_membership(session.round_state.eliminated_ids, player_id);
    remove_membership(session.round_state.survivor_ids, player_id);
    ensure_membership(session.round_state.participant_ids, player_id);
    refresh_round_outcome(session);
}

void GameEngineService::remove_player(GameSessionModel& session, const std::string& player_id) {
    session.players.erase(player_id);
    remove_membership(session.round_state.participant_ids, player_id);
    remove_membership(session.round_state.survivor_ids, player_id);
    remove_membership(session.round_state.eliminated_ids, player_id);
    if (session.winner_id == player_id)
        session.winner_id = std::nullopt;
    refresh_round_outcome(session);
}

void GameEngineService::reset_session(GameSessionModel& session) {
    session.winner_id = std::nullopt;
    session.app_state = session.calibration.is_valid()
        ? AppState::Calibrated
        : AppState::CameraReady;
    session.status_message = "Session reset";
    session.grid.reset_states(CellState::Neutral);

    auto& rs = session.round_state;
    rs.phase = RoundPhase::Idle;
    rs.timer_remaining = 0.0f;
    rs.flash_elapsed_seconds = 0.0f;
    rs.participant_ids.clear();
    rs.survivor_ids.clear();
    rs.eliminated_ids.clear();

    for (auto& [id, player] : session.players) {
        player.active_in_round = false;
        player.eliminated_in_round = false;
        if (player.tracking_state == PlayerTrackingState::Eliminated)
            player.tracking_state = PlayerTrackingState::Missing;
    }
}

void GameEngineService::force_next_round(GameSessionModel& session) {
    start_round(session);
}

// ── Render state builder ─────────────────────────────────────────────

RenderState GameEngineService::build_render_state(const GameSessionModel& session) const {
    RenderState rs;
    rs.phase = session.round_state.phase;
    rs.timer_text = std::format("{:.1f}", session.round_state.timer_remaining);
    rs.status_text = session.status_message;
    rs.camera_status_text = session.camera_status_message;
    rs.pose_status_text = session.pose_status_message;
    rs.calibration_status_text = session.calibration.validation_message;
    rs.display_status_text = session.display_status_message;

    for (auto& [idx, state] : session.grid.cell_states)
        rs.grid_cells[idx] = state;

    for (auto& [id, player] : session.players) {
        if (player.tracking_state == PlayerTrackingState::Missing
            && !player.standing_point && !player.active_in_round)
            continue;

        rs.players.push_back(RenderPlayerState{
            .player_id = player.player_id,
            .standing_point = player.standing_point,
            .left_foot = player.left_foot,
            .right_foot = player.right_foot,
            .occupied_cell = player.occupied_cell,
            .status_text = std::string(to_string(player.tracking_state)),
        });
    }

    return rs;
}

// ── Private helpers ──────────────────────────────────────────────────

void GameEngineService::refresh_round_outcome(GameSessionModel& session) {
    auto& rs = session.round_state;

    rs.survivor_ids.clear();
    for (auto& pid : rs.participant_ids) {
        if (std::find(rs.eliminated_ids.begin(), rs.eliminated_ids.end(), pid) == rs.eliminated_ids.end())
            rs.survivor_ids.push_back(pid);
    }

    if (rs.survivor_ids.size() == 1) {
        session.winner_id = rs.survivor_ids[0];
        session.app_state = AppState::Finished;
        session.status_message = "Winner: " + *session.winner_id;
    } else {
        session.winner_id = std::nullopt;
        if (rs.phase == RoundPhase::Finished)
            rs.phase = RoundPhase::Results;
        session.status_message = std::format(
            "Round {} results: {} survivors, {} eliminated",
            rs.round_number, rs.survivor_ids.size(), rs.eliminated_ids.size());
    }
}

void GameEngineService::ensure_membership(std::vector<std::string>& values, const std::string& id) {
    if (std::find(values.begin(), values.end(), id) == values.end())
        values.push_back(id);
}

void GameEngineService::remove_membership(std::vector<std::string>& values, const std::string& id) {
    std::erase(values, id);
}

} // namespace sag
