from __future__ import annotations

import numpy as np

from src.models.calibration_model import CalibrationModel
from src.models.contracts import RenderState
from src.models.enums import CalibrationState, CellState
from src.services.overlay_render_service import OverlayRenderService
from src.utils.constants import PROJECTOR_CANVAS_SIZE


class TestOverlayRenderService:
    def test_render_returns_correct_size(self) -> None:
        service = OverlayRenderService()
        render_state = RenderState()
        image = service.render(
            PROJECTOR_CANVAS_SIZE[0], PROJECTOR_CANVAS_SIZE[1], render_state
        )
        assert image.width() == 1280
        assert image.height() == 720

    def test_render_with_calibration_no_exception(self) -> None:
        service = OverlayRenderService()
        h = np.eye(3, dtype=np.float32)
        calibration = CalibrationModel(
            state=CalibrationState.VALID,
            homography=h,
            inverse_homography=np.linalg.inv(h),
            playable_bounds=((0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 4.0)),
        )
        render_state = RenderState(
            grid_cells={
                (0, 0): CellState.GREEN,
                (0, 1): CellState.RED,
                (1, 0): CellState.FLASHING,
                (1, 1): CellState.NEUTRAL,
            },
        )
        image = service.render(1280, 720, render_state, calibration=calibration)
        assert image.width() == 1280

    def test_render_no_players_no_exception(self) -> None:
        service = OverlayRenderService()
        render_state = RenderState(players=[])
        image = service.render(640, 480, render_state)
        assert image.width() == 640

    def test_uses_cached_inverse_homography(self) -> None:
        service = OverlayRenderService()
        h = np.eye(3, dtype=np.float32) * 2.0
        inv_h = np.linalg.inv(h)
        calibration = CalibrationModel(
            state=CalibrationState.VALID,
            homography=h,
            inverse_homography=inv_h,
            playable_bounds=((0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 4.0)),
        )
        render_state = RenderState(
            grid_cells={(0, 0): CellState.GREEN},
        )
        # Should not raise, even though h is not identity
        image = service.render(1280, 720, render_state, calibration=calibration)
        assert image.width() == 1280
