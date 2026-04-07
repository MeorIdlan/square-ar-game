from __future__ import annotations

import numpy as np

from src.models.calibration_model import CalibrationMarkerLayout, CalibrationModel
from src.models.contracts import MappedPlayerState, PoseFootState, RenderState
from src.models.enums import (
    AppState,
    CalibrationState,
    CellState,
    PlayerTrackingState,
    RoundPhase,
)
from src.models.game_session_model import GameSessionModel
from src.models.grid_model import CellGeometry, GridModel
from src.models.player_model import PlayerModel
from src.models.round_model import RoundModel, RoundTimingSettings


class TestCalibrationModel:
    def test_is_valid_when_state_valid_and_homography_set(self) -> None:
        model = CalibrationModel(state=CalibrationState.VALID, homography=np.eye(3))
        assert model.is_valid is True

    def test_is_not_valid_when_state_invalid(self) -> None:
        model = CalibrationModel(state=CalibrationState.INVALID, homography=np.eye(3))
        assert model.is_valid is False

    def test_is_not_valid_when_homography_none(self) -> None:
        model = CalibrationModel(state=CalibrationState.VALID, homography=None)
        assert model.is_valid is False

    def test_detected_marker_ids(self) -> None:
        model = CalibrationModel(
            detected_marker_corners={
                2: np.array([[0, 0], [1, 0], [1, 1], [0, 1]]),
                0: np.array([[0, 0], [1, 0], [1, 1], [0, 1]]),
            }
        )
        assert model.detected_marker_ids == [0, 2]

    def test_missing_marker_ids(self) -> None:
        model = CalibrationModel(
            detected_marker_corners={
                0: np.array([[0, 0], [1, 0], [1, 1], [0, 1]]),
                2: np.array([[0, 0], [1, 0], [1, 1], [0, 1]]),
            }
        )
        assert model.missing_marker_ids == [1, 3]

    def test_detected_marker_count(self) -> None:
        model = CalibrationModel(
            detected_marker_corners={i: np.zeros((4, 2)) for i in range(3)}
        )
        assert model.detected_marker_count == 3

    def test_inverse_homography_field(self) -> None:
        h = np.eye(3) * 2.0
        inv = np.linalg.inv(h)
        model = CalibrationModel(homography=h, inverse_homography=inv)
        np.testing.assert_array_almost_equal(model.inverse_homography, inv)

    def test_default_marker_layout(self) -> None:
        layout = CalibrationMarkerLayout()
        assert layout.marker_ids == (0, 1, 2, 3)


class TestGridModel:
    def test_default_construction(self) -> None:
        grid = GridModel()
        assert grid.rows == 4
        assert grid.columns == 4
        assert len(grid.cell_states) == 16

    def test_reset_states(self) -> None:
        grid = GridModel(rows=2, columns=2)
        grid.cell_states[(0, 0)] = CellState.RED
        grid.reset_states()
        assert all(s is CellState.NEUTRAL for s in grid.cell_states.values())

    def test_reset_states_custom(self) -> None:
        grid = GridModel(rows=2, columns=2)
        grid.reset_states(CellState.GREEN)
        assert all(s is CellState.GREEN for s in grid.cell_states.values())

    def test_cell_geometry_top_left(self) -> None:
        grid = GridModel(rows=4, columns=4, playable_width=4.0, playable_height=4.0)
        geom = grid.cell_geometry(0, 0)
        assert geom.index == (0, 0)
        assert geom.top_left == (0.0, 0.0)
        assert geom.bottom_right == (1.0, 1.0)

    def test_cell_geometry_bottom_right(self) -> None:
        grid = GridModel(rows=4, columns=4, playable_width=4.0, playable_height=4.0)
        geom = grid.cell_geometry(3, 3)
        assert geom.index == (3, 3)
        assert geom.top_left == (3.0, 3.0)
        assert geom.bottom_right == (4.0, 4.0)

    def test_cell_geometry_center(self) -> None:
        grid = GridModel(rows=4, columns=4, playable_width=4.0, playable_height=4.0)
        geom = grid.cell_geometry(1, 2)
        assert geom.top_left == (2.0, 1.0)
        assert geom.bottom_right == (3.0, 2.0)

    def test_cell_geometry_width_height(self) -> None:
        geom = CellGeometry(index=(0, 0), top_left=(0.0, 0.0), bottom_right=(2.0, 3.0))
        assert geom.width == 2.0
        assert geom.height == 3.0

    def test_cell_width_and_height(self) -> None:
        grid = GridModel(rows=2, columns=4, playable_width=8.0, playable_height=6.0)
        assert grid.cell_width == 2.0
        assert grid.cell_height == 3.0


class TestPlayerModel:
    def test_default_construction(self) -> None:
        player = PlayerModel(player_id="P1")
        assert player.player_id == "P1"
        assert player.tracking_state is PlayerTrackingState.UNKNOWN
        assert player.standing_point is None
        assert player.occupied_cell is None
        assert player.confidence == 0.0
        assert player.active_in_round is False
        assert player.eliminated_in_round is False

    def test_field_mutation(self) -> None:
        player = PlayerModel(player_id="P1")
        player.standing_point = (1.0, 2.0)
        player.tracking_state = PlayerTrackingState.ACTIVE
        assert player.standing_point == (1.0, 2.0)
        assert player.tracking_state is PlayerTrackingState.ACTIVE


class TestRoundModel:
    def test_defaults(self) -> None:
        model = RoundModel()
        assert model.round_number == 0
        assert model.phase is RoundPhase.IDLE
        assert model.timer_remaining == 0.0
        assert model.participant_ids == []
        assert model.survivor_ids == []
        assert model.eliminated_ids == []

    def test_timing_defaults(self) -> None:
        timings = RoundTimingSettings()
        assert timings.flashing_duration_seconds == 3.0
        assert timings.flash_interval_seconds == 0.25
        assert timings.reaction_window_seconds == 1.5
        assert timings.missed_detection_grace_seconds == 0.35


class TestGameSessionModel:
    def test_default_construction(self) -> None:
        session = GameSessionModel()
        assert session.app_state is AppState.BOOTING
        assert session.winner_id is None
        assert isinstance(session.calibration, CalibrationModel)
        assert isinstance(session.grid, GridModel)
        assert isinstance(session.round_state, RoundModel)
        assert session.players == {}


class TestRenderState:
    def test_default(self) -> None:
        rs = RenderState()
        assert rs.phase is RoundPhase.IDLE
        assert rs.timer_text == "00.0"
        assert rs.grid_cells == {}
        assert rs.players == []


class TestContracts:
    def test_pose_foot_state(self) -> None:
        pfs = PoseFootState(left_foot=(1.0, 2.0), right_foot=(3.0, 4.0), confidence=0.9)
        assert pfs.left_foot == (1.0, 2.0)
        assert pfs.confidence == 0.9

    def test_mapped_player_state(self) -> None:
        mps = MappedPlayerState(
            player_id="P1", standing_point=(1.0, 1.0), in_bounds=True
        )
        assert mps.player_id == "P1"
        assert mps.ambiguous is False

    def test_mapped_player_state_defaults(self) -> None:
        mps = MappedPlayerState(player_id="X", standing_point=None)
        assert mps.in_bounds is False
        assert mps.occupied_cell is None
        assert mps.ambiguous is False
