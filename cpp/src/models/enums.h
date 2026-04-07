#pragma once
// square-ar-game  C++ reimplementation
// Enumerations mirroring src/models/enums.py

#include <cstdint>
#include <string_view>

namespace sag {

enum class AppState : uint8_t {
    Booting,
    CameraReady,
    Calibrating,
    Calibrated,
    Running,
    Paused,
    Finished,
    Error,
};

enum class RoundPhase : uint8_t {
    Idle,
    Preparing,
    Flashing,
    Locked,
    Checking,
    Results,
    Finished,
};

enum class PlayerTrackingState : uint8_t {
    Unknown,
    Active,
    Missing,
    Eliminated,
};

enum class CellState : uint8_t {
    Inactive,
    Neutral,
    Flashing,
    Green,
    Red,
    Locked,
};

enum class CalibrationState : uint8_t {
    NotCalibrated,
    Detecting,
    Valid,
    Invalid,
};

// ── String conversion helpers ─────────────────────────────────────────

constexpr std::string_view to_string(AppState s) {
    switch (s) {
        case AppState::Booting:      return "Booting";
        case AppState::CameraReady:  return "CameraReady";
        case AppState::Calibrating:  return "Calibrating";
        case AppState::Calibrated:   return "Calibrated";
        case AppState::Running:      return "Running";
        case AppState::Paused:       return "Paused";
        case AppState::Finished:     return "Finished";
        case AppState::Error:        return "Error";
    }
    return "Unknown";
}

constexpr std::string_view to_string(RoundPhase p) {
    switch (p) {
        case RoundPhase::Idle:      return "Idle";
        case RoundPhase::Preparing: return "Preparing";
        case RoundPhase::Flashing:  return "Flashing";
        case RoundPhase::Locked:    return "Locked";
        case RoundPhase::Checking:  return "Checking";
        case RoundPhase::Results:   return "Results";
        case RoundPhase::Finished:  return "Finished";
    }
    return "Unknown";
}

constexpr std::string_view to_string(PlayerTrackingState s) {
    switch (s) {
        case PlayerTrackingState::Unknown:    return "unknown";
        case PlayerTrackingState::Active:     return "active";
        case PlayerTrackingState::Missing:    return "missing";
        case PlayerTrackingState::Eliminated: return "eliminated";
    }
    return "unknown";
}

constexpr std::string_view to_string(CellState s) {
    switch (s) {
        case CellState::Inactive: return "Inactive";
        case CellState::Neutral:  return "Neutral";
        case CellState::Flashing: return "Flashing";
        case CellState::Green:    return "Green";
        case CellState::Red:      return "Red";
        case CellState::Locked:   return "Locked";
    }
    return "Unknown";
}

constexpr std::string_view to_string(CalibrationState s) {
    switch (s) {
        case CalibrationState::NotCalibrated: return "Not calibrated";
        case CalibrationState::Detecting:     return "Detecting";
        case CalibrationState::Valid:         return "Valid";
        case CalibrationState::Invalid:       return "Invalid";
    }
    return "Unknown";
}

} // namespace sag
