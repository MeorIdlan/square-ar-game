from __future__ import annotations

from src.utils.constants import (
    BOUNDARY_TOLERANCE,
    DEBUG_CANVAS_SIZE,
    DETECTION_DEDUP_DISTANCE,
    FLASHING_CELL_ALPHA,
    FLASHING_CELL_COLOR,
    FOOT_LANDMARK_LEFT,
    FOOT_LANDMARK_RIGHT,
    GAME_TICK_INTERVAL_MS,
    PLAYER_MATCH_MAX_DISTANCE,
    PROJECTOR_CANVAS_SIZE,
    REPROJECTION_ERROR_THRESHOLD,
)


class TestConstants:
    def test_reprojection_error_threshold_is_float(self) -> None:
        assert isinstance(REPROJECTION_ERROR_THRESHOLD, float)
        assert REPROJECTION_ERROR_THRESHOLD == 0.15

    def test_player_match_max_distance_is_float(self) -> None:
        assert isinstance(PLAYER_MATCH_MAX_DISTANCE, float)
        assert PLAYER_MATCH_MAX_DISTANCE == 1.25

    def test_detection_dedup_distance(self) -> None:
        assert isinstance(DETECTION_DEDUP_DISTANCE, float)
        assert DETECTION_DEDUP_DISTANCE == 0.35

    def test_boundary_tolerance(self) -> None:
        assert isinstance(BOUNDARY_TOLERANCE, float)
        assert BOUNDARY_TOLERANCE == 1e-6

    def test_foot_landmark_indices(self) -> None:
        assert FOOT_LANDMARK_LEFT == (27, 29, 31)
        assert FOOT_LANDMARK_RIGHT == (28, 30, 32)
        assert len(FOOT_LANDMARK_LEFT) == 3
        assert len(FOOT_LANDMARK_RIGHT) == 3

    def test_canvas_sizes(self) -> None:
        assert PROJECTOR_CANVAS_SIZE == (1280, 720)
        assert DEBUG_CANVAS_SIZE == (640, 640)

    def test_game_tick_interval(self) -> None:
        assert isinstance(GAME_TICK_INTERVAL_MS, int)
        assert GAME_TICK_INTERVAL_MS == 100

    def test_flashing_cell_color(self) -> None:
        assert FLASHING_CELL_COLOR == (241, 196, 15)
        assert isinstance(FLASHING_CELL_ALPHA, int)
        assert 0 <= FLASHING_CELL_ALPHA <= 255
