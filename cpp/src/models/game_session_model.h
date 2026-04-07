#pragma once
// GameSessionModel — mirrors src/models/game_session_model.py

#include "models/calibration_model.h"
#include "models/enums.h"
#include "models/grid_model.h"
#include "models/player_model.h"
#include "models/round_model.h"

#include <optional>
#include <string>
#include <unordered_map>

namespace sag {

struct GameSessionModel {
    AppState app_state = AppState::Booting;

    CalibrationModel calibration;
    GridModel grid;
    RoundModel round_state;

    std::unordered_map<std::string, PlayerModel> players;

    std::optional<std::string> winner_id;

    std::string status_message          = "Booting";
    std::string camera_status_message   = "Waiting for camera frames";
    std::string pose_status_message     = "Pose tracking idle";
    std::string display_status_message  = "Projector not assigned";
};

} // namespace sag
