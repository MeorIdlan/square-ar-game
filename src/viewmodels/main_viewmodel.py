from __future__ import annotations

from PyQt6.QtCore import QObject, pyqtSignal

from src.models.contracts import FramePacket, PoseResult
from src.models.enums import AppState, PlayerTrackingState
from src.models.game_session_model import GameSessionModel
from src.services.floor_mapping_service import FloorMappingService
from src.services.game_engine_service import GameEngineService
from src.utils.config import AppConfig, ConfigStore
from src.viewmodels.calibration_viewmodel import CalibrationViewModel
from src.viewmodels.debug_viewmodel import DebugViewModel
from src.viewmodels.game_viewmodel import GameViewModel
from src.viewmodels.projector_viewmodel import ProjectorViewModel


class MainViewModel(QObject):
    status_changed = pyqtSignal(str)
    session_changed = pyqtSignal(object)

    def __init__(
        self,
        config: AppConfig,
        config_store: ConfigStore,
        session_model: GameSessionModel,
        calibration_viewmodel: CalibrationViewModel,
        game_viewmodel: GameViewModel,
        projector_viewmodel: ProjectorViewModel,
        debug_viewmodel: DebugViewModel,
        floor_mapping_service: FloorMappingService,
        game_engine_service: GameEngineService,
    ) -> None:
        super().__init__()
        self._config = config
        self._config_store = config_store
        self._session_model = session_model
        self.calibration_viewmodel = calibration_viewmodel
        self.game_viewmodel = game_viewmodel
        self.projector_viewmodel = projector_viewmodel
        self.debug_viewmodel = debug_viewmodel
        self._floor_mapping_service = floor_mapping_service
        self._game_engine_service = game_engine_service
        self._latest_frame_packet: FramePacket | None = None

        self.calibration_viewmodel.calibration_updated.connect(self._on_calibration_updated)
        self.game_viewmodel.session_updated.connect(self._publish_session)

    @property
    def config(self) -> AppConfig:
        return self._config

    @property
    def session_model(self) -> GameSessionModel:
        return self._session_model

    def initialize(self) -> None:
        self._session_model.app_state = AppState.CAMERA_READY
        self._session_model.status_message = "Ready to calibrate"
        self.game_viewmodel.ensure_demo_players()
        self._publish_session(self._session_model)

    def calibrate(self) -> None:
        self._session_model.app_state = AppState.CALIBRATING
        self._session_model.status_message = "Attempting ArUco calibration from latest camera frame"
        self.status_changed.emit(self._session_model.status_message)
        self.calibration_viewmodel.calibrate(self._latest_frame_packet)

    def start_round(self) -> None:
        self.game_viewmodel.start_round()

    def lock_round(self) -> None:
        self.game_viewmodel.lock_round()

    def save_config(self) -> None:
        self._config_store.save(self._config)
        self.status_changed.emit(f"Saved settings to {self._config_store.config_path}")

    def handle_frame_packet(self, frame_packet: FramePacket) -> None:
        self._latest_frame_packet = frame_packet

    def handle_pose_result(self, pose_result: PoseResult) -> None:
        for index, pose_state in enumerate(pose_result.detections, start=1):
            player_id = f"P{index}"
            mapped = self._floor_mapping_service.map_detection(
                player_id=player_id,
                pose_state=pose_state,
                grid=self._session_model.grid,
                calibration=self._session_model.calibration,
            )
            player = self._session_model.players.get(player_id)
            if player is None:
                continue
            player.standing_point = mapped.standing_point
            player.left_foot = mapped.left_foot
            player.right_foot = mapped.right_foot
            player.occupied_cell = mapped.occupied_cell
            player.confidence = mapped.confidence
            player.tracking_state = PlayerTrackingState.ACTIVE if mapped.in_bounds else PlayerTrackingState.MISSING

        self._publish_session(self._session_model)

    def _on_calibration_updated(self, calibration_model: object) -> None:
        self._session_model.calibration = calibration_model
        self._session_model.app_state = AppState.CALIBRATED if calibration_model.is_valid else AppState.CAMERA_READY
        self._session_model.status_message = calibration_model.validation_message
        self._publish_session(self._session_model)

    def _publish_session(self, session_model: GameSessionModel) -> None:
        render_state = self._game_engine_service.build_render_state(session_model)
        self.projector_viewmodel.update_render_state(render_state)
        self.debug_viewmodel.update_render_state(render_state)
        self.status_changed.emit(session_model.status_message)
        self.session_changed.emit(session_model)
