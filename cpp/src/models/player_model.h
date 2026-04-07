#pragma once
// PlayerModel — mirrors src/models/player_model.py

#include "models/enums.h"
#include "utils/math_helpers.h"

#include <optional>
#include <string>

namespace sag {

struct PlayerModel {
    std::string player_id;
    PlayerTrackingState tracking_state = PlayerTrackingState::Unknown;

    std::optional<Point> standing_point;
    std::optional<Point> left_foot;
    std::optional<Point> right_foot;
    std::optional<CellIndex> occupied_cell;

    float confidence    = 0.0f;
    double last_seen_at = 0.0;

    bool active_in_round    = false;
    bool eliminated_in_round = false;
};

} // namespace sag
