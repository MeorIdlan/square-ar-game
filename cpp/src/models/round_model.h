#pragma once
// RoundModel + RoundTimingSettings — mirrors src/models/round_model.py

#include "models/enums.h"
#include "utils/constants.h"

#include <string>
#include <vector>

namespace sag {

struct RoundTimingSettings {
    float flashing_duration_seconds     = constants::defaults::FLASHING_DURATION_SECONDS;
    float flash_interval_seconds        = constants::defaults::FLASH_INTERVAL_SECONDS;
    float reaction_window_seconds       = constants::defaults::REACTION_WINDOW_SECONDS;
    float lock_delay_seconds            = constants::defaults::LOCK_DELAY_SECONDS;
    float elimination_display_seconds   = constants::defaults::ELIMINATION_DISPLAY_SECONDS;
    float inter_round_delay_seconds     = constants::defaults::INTER_ROUND_DELAY_SECONDS;
    float missed_detection_grace_seconds = constants::defaults::MISSED_DETECTION_GRACE_SECONDS;
};

struct RoundModel {
    int round_number           = 0;
    RoundPhase phase           = RoundPhase::Idle;
    float timer_remaining      = 0.0f;
    float flash_elapsed_seconds = 0.0f;

    RoundTimingSettings timings;

    std::vector<std::string> participant_ids;
    std::vector<std::string> survivor_ids;
    std::vector<std::string> eliminated_ids;
};

} // namespace sag
