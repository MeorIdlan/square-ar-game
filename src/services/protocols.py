from __future__ import annotations

from pathlib import Path
from typing import Protocol

import numpy as np
from PyQt6.QtGui import QImage

from src.models.calibration_model import CalibrationModel
from src.models.contracts import (
    CellIndex,
    FramePacket,
    MappedPlayerState,
    Point,
    PoseFootState,
    PoseResult,
    RenderState,
)
from src.models.game_session_model import GameSessionModel
from src.models.grid_model import GridModel
from src.models.player_model import PlayerModel
from src.utils.config import CameraProfile


class CameraServiceProtocol(Protocol):
    @property
    def last_error_message(self) -> str | None: ...

    def open(self) -> bool: ...

    def release(self) -> None: ...

    def next_frame(self) -> FramePacket: ...

    def set_camera_index(self, camera_index: int) -> None: ...

    def set_camera_profile(self, profile: CameraProfile) -> None: ...

    def available_camera_indices(self, max_index: int = 5) -> list[int]: ...

    def available_camera_profiles(
        self, camera_index: int | None = None, probe: bool = False
    ) -> list[CameraProfile]: ...

    def reconnect(self) -> bool: ...


class CalibrationServiceProtocol(Protocol):
    def calibrate_from_frame(
        self,
        model: CalibrationModel,
        frame: np.ndarray | None,
        *,
        is_live_source: bool = True,
        source_error: str | None = None,
    ) -> CalibrationModel: ...

    def reset(self, model: CalibrationModel) -> CalibrationModel: ...


class FloorMappingServiceProtocol(Protocol):
    def map_detection(
        self,
        player_id: str,
        pose_state: PoseFootState,
        grid: GridModel,
        calibration: CalibrationModel,
    ) -> MappedPlayerState: ...

    def map_image_point_to_floor(
        self, point: Point | None, calibration: CalibrationModel
    ) -> Point | None: ...

    def resolve_cell(
        self, point: Point | None, grid: GridModel, calibration: CalibrationModel
    ) -> CellIndex | None: ...

    def is_inside_playable_area(
        self, point: Point, calibration: CalibrationModel
    ) -> bool: ...


class GameEngineServiceProtocol(Protocol):
    def start_round(self, session: GameSessionModel) -> None: ...

    def tick(self, session: GameSessionModel, delta_seconds: float) -> None: ...

    def evaluate_round(
        self,
        session: GameSessionModel,
        mapped_players: list[MappedPlayerState] | None = None,
    ) -> None: ...

    def build_render_state(self, session: GameSessionModel) -> RenderState: ...

    def revive_player(self, session: GameSessionModel, player_id: str) -> None: ...

    def eliminate_player(self, session: GameSessionModel, player_id: str) -> None: ...

    def remove_player(self, session: GameSessionModel, player_id: str) -> None: ...

    def reset_session(self, session: GameSessionModel) -> None: ...

    def force_next_round(self, session: GameSessionModel) -> None: ...


class PlayerTrackerServiceProtocol(Protocol):
    def update_players(
        self,
        players: dict[str, PlayerModel],
        detections: list[MappedPlayerState],
        timestamp: float,
        grace_period_seconds: float,
    ) -> None: ...


class PoseTrackingServiceProtocol(Protocol):
    @property
    def active_delegate_name(self) -> str: ...

    def process_frame(self, frame_packet: FramePacket) -> PoseResult: ...

    def set_model_asset_path(self, model_asset_path: Path) -> None: ...

    def close(self) -> None: ...


class OverlayRenderServiceProtocol(Protocol):
    def render(
        self,
        width: int,
        height: int,
        render_state: RenderState,
        frame_packet: FramePacket | None = None,
        calibration: CalibrationModel | None = None,
    ) -> QImage: ...


class DebugRenderServiceProtocol(Protocol):
    def render(self, width: int, height: int, render_state: RenderState) -> QImage: ...
