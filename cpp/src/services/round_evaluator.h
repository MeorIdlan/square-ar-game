#pragma once
// RoundEvaluator — pure evaluation logic, mirrors src/services/round_evaluator.py

#include "models/contracts.h"
#include "models/enums.h"
#include "models/game_session_model.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace sag {

struct EvaluationResult {
    std::vector<std::string> survivors;
    std::vector<std::string> eliminated;
    std::unordered_map<std::string, std::string> reason_map;
};

class RoundEvaluator {
public:
    EvaluationResult evaluate(
        const GameSessionModel& session,
        const std::vector<MappedPlayerState>& mapped_players) const;

private:
    struct EliminationCheck {
        bool eliminated = false;
        std::string reason;
    };

    static EliminationCheck check_elimination(
        const MappedPlayerState* mapped_player,
        PlayerTrackingState tracking_state,
        const CellStateMap& cell_states);
};

} // namespace sag
