#pragma once
// PlayerTrackerService — mirrors src/services/player_tracker_service.py

#include "models/contracts.h"
#include "models/player_model.h"
#include "utils/constants.h"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace sag {

class PlayerTrackerService {
public:
    explicit PlayerTrackerService(
        float max_match_distance = constants::PLAYER_MATCH_MAX_DISTANCE);

    void update_players(
        std::unordered_map<std::string, PlayerModel>& players,
        const std::vector<MappedPlayerState>& detections,
        double timestamp,
        float grace_period_seconds) const;

private:
    std::string find_best_match(
        const std::unordered_map<std::string, PlayerModel>& players,
        const std::set<std::string>& candidate_ids,
        const MappedPlayerState& detection) const;

    static void apply_detection(
        PlayerModel& player,
        const MappedPlayerState& detection,
        double timestamp);

    static std::string allocate_player_id(
        const std::unordered_map<std::string, PlayerModel>& players);

    float max_match_distance_;
};

} // namespace sag
