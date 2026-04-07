from __future__ import annotations

import numpy as np
import pytest

from src.models.calibration_model import CalibrationModel
from src.models.contracts import FramePacket
from src.models.enums import CalibrationState
from src.models.game_session_model import GameSessionModel
from src.models.grid_model import GridModel
from src.utils.config import AppConfig


@pytest.fixture()
def make_session():
    def _factory(**kwargs) -> GameSessionModel:
        return GameSessionModel(**kwargs)

    return _factory


@pytest.fixture()
def make_calibration():
    def _factory(
        homography: np.ndarray | None = None,
        inverse_homography: np.ndarray | None = None,
        state: CalibrationState = CalibrationState.VALID,
        outer_bounds=((0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 4.0)),
        playable_bounds=((0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 4.0)),
    ) -> CalibrationModel:
        h = homography if homography is not None else np.eye(3)
        inv_h = (
            inverse_homography if inverse_homography is not None else np.linalg.inv(h)
        )
        return CalibrationModel(
            state=state,
            homography=h,
            inverse_homography=inv_h,
            outer_bounds=outer_bounds,
            playable_bounds=playable_bounds,
        )

    return _factory


@pytest.fixture()
def make_grid():
    def _factory(rows: int = 3, cols: int = 3) -> GridModel:
        return GridModel(
            rows=rows, columns=cols, playable_width=4.0, playable_height=4.0
        )

    return _factory


@pytest.fixture()
def make_frame_packet():
    def _factory(live: bool = True, width: int = 640, height: int = 480) -> FramePacket:
        frame = np.zeros((height, width, 3), dtype=np.uint8)
        return FramePacket(
            frame_id=1,
            frame=frame,
            is_live=live,
            source_name="camera" if live else "fallback",
        )

    return _factory


@pytest.fixture()
def make_app_config():
    def _factory(**kwargs) -> AppConfig:
        return AppConfig(**kwargs)

    return _factory
