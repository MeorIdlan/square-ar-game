#include "services/round_evaluator.h"

namespace sag {

EvaluationResult RoundEvaluator::evaluate(
    const GameSessionModel& session,
    const std::vector<MappedPlayerState>& mapped_players) const
{
    std::unordered_map<std::string, const MappedPlayerState*> players_by_id;
    for (auto& mp : mapped_players)
        players_by_id[mp.player_id] = &mp;

    EvaluationResult result;

    for (auto& participant_id : session.round_state.participant_ids) {
        const MappedPlayerState* mapped = nullptr;
        if (auto it = players_by_id.find(participant_id); it != players_by_id.end())
            mapped = it->second;

        auto session_it = session.players.find(participant_id);
        PlayerTrackingState tracking = (session_it != session.players.end())
            ? session_it->second.tracking_state
            : PlayerTrackingState::Missing;

        auto check = check_elimination(mapped, tracking, session.grid.cell_states);

        if (check.eliminated) {
            result.eliminated.push_back(participant_id);
            result.reason_map[participant_id] = std::move(check.reason);
        } else {
            result.survivors.push_back(participant_id);
        }
    }

    return result;
}

RoundEvaluator::EliminationCheck RoundEvaluator::check_elimination(
    const MappedPlayerState* mapped_player,
    PlayerTrackingState tracking_state,
    const CellStateMap& cell_states)
{
    if (mapped_player == nullptr)
        return {true, "not found in mapped players"};

    if (tracking_state == PlayerTrackingState::Missing)
        return {true, "player missing"};

    if (!mapped_player->in_bounds)
        return {true, "out of bounds"};

    if (!mapped_player->occupied_cell.has_value())
        return {true, "no occupied cell"};

    if (mapped_player->ambiguous)
        return {true, "ambiguous position"};

    auto it = cell_states.find(*mapped_player->occupied_cell);
    if (it != cell_states.end() && it->second == CellState::Red)
        return {true, "standing on red cell"};

    return {false, ""};
}

} // namespace sag
