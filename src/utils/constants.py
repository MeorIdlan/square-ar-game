from __future__ import annotations

# --- Calibration ---
REPROJECTION_ERROR_THRESHOLD: float = 0.15

# --- Player tracking ---
PLAYER_MATCH_MAX_DISTANCE: float = 1.25

# --- Detection deduplication ---
# NOTE: This shares the same numeric value as
# RoundTimingSettings.missed_detection_grace_seconds by coincidence.
# They are independent tuning parameters — change each on its own merits.
DETECTION_DEDUP_DISTANCE: float = 0.35

# --- Floor mapping ---
BOUNDARY_TOLERANCE: float = 1e-6

# --- MediaPipe pose landmark indices ---
FOOT_LANDMARK_LEFT: tuple[int, int, int] = (27, 29, 31)
FOOT_LANDMARK_RIGHT: tuple[int, int, int] = (28, 30, 32)

# --- Canvas sizes ---
PROJECTOR_CANVAS_SIZE: tuple[int, int] = (1280, 720)
DEBUG_CANVAS_SIZE: tuple[int, int] = (640, 640)

# --- Game loop ---
GAME_TICK_INTERVAL_MS: int = 100

# --- Overlay rendering ---
FLASHING_CELL_COLOR: tuple[int, int, int] = (241, 196, 15)
FLASHING_CELL_ALPHA: int = 90
