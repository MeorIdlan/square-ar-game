#pragma once
// All tuning constants — mirrors src/utils/constants.py

#include <array>
#include <cstdint>

namespace sag::constants {

// ── Calibration ───────────────────────────────────────────────────────
inline constexpr float REPROJECTION_ERROR_THRESHOLD = 0.15f;

// ── Player tracking ───────────────────────────────────────────────────
inline constexpr float PLAYER_MATCH_MAX_DISTANCE = 1.25f;

// ── Detection deduplication ───────────────────────────────────────────
inline constexpr float DETECTION_DEDUP_DISTANCE = 0.35f;

// ── Floor mapping ─────────────────────────────────────────────────────
inline constexpr float BOUNDARY_TOLERANCE = 1e-6f;

// ── MediaPipe pose landmark indices ───────────────────────────────────
inline constexpr std::array<int, 3> FOOT_LANDMARK_LEFT  = {27, 29, 31};
inline constexpr std::array<int, 3> FOOT_LANDMARK_RIGHT = {28, 30, 32};

// ── Canvas sizes ──────────────────────────────────────────────────────
inline constexpr int PROJECTOR_CANVAS_WIDTH  = 1280;
inline constexpr int PROJECTOR_CANVAS_HEIGHT = 720;
inline constexpr int DEBUG_CANVAS_WIDTH  = 640;
inline constexpr int DEBUG_CANVAS_HEIGHT = 640;

// ── Game loop ─────────────────────────────────────────────────────────
inline constexpr int GAME_TICK_INTERVAL_MS = 100;

// ── Overlay rendering (BGR order to match OpenCV) ─────────────────────
inline constexpr uint8_t FLASHING_CELL_COLOR_B = 15;
inline constexpr uint8_t FLASHING_CELL_COLOR_G = 196;
inline constexpr uint8_t FLASHING_CELL_COLOR_R = 241;
inline constexpr uint8_t FLASHING_CELL_ALPHA   = 90;

// ── Overlay cell alpha ────────────────────────────────────────────────
inline constexpr uint8_t CELL_OVERLAY_ALPHA = 110;

// ── Player marker sizes (pixels) ─────────────────────────────────────
inline constexpr int FOOT_MARKER_RADIUS    = 8;
inline constexpr int STANDING_MARKER_RADIUS = 11;

// ── Default round timing ─────────────────────────────────────────────
namespace defaults {
    inline constexpr float FLASHING_DURATION_SECONDS     = 3.0f;
    inline constexpr float FLASH_INTERVAL_SECONDS        = 0.25f;
    inline constexpr float REACTION_WINDOW_SECONDS       = 1.5f;
    inline constexpr float LOCK_DELAY_SECONDS            = 0.25f;
    inline constexpr float ELIMINATION_DISPLAY_SECONDS   = 2.5f;
    inline constexpr float INTER_ROUND_DELAY_SECONDS     = 1.5f;
    inline constexpr float MISSED_DETECTION_GRACE_SECONDS = 0.35f;

    inline constexpr int GRID_ROWS            = 4;
    inline constexpr int GRID_COLUMNS         = 4;
    inline constexpr float PLAYABLE_WIDTH     = 4.0f;
    inline constexpr float PLAYABLE_HEIGHT    = 4.0f;
    inline constexpr float PLAYABLE_INSET     = 0.2f;

    inline constexpr int CAMERA_INDEX         = 0;
    inline constexpr int FRAME_WIDTH          = 1280;
    inline constexpr int FRAME_HEIGHT         = 720;
    inline constexpr int TARGET_FPS           = 30;
} // namespace defaults

} // namespace sag::constants
