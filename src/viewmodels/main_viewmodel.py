from __future__ import annotations

import logging
from pathlib import Path

from PyQt6.QtCore import QObject, pyqtSignal

from src.models.contracts import FramePacket, MappedPlayerState, PoseResult
from src.models.enums import AppState, PlayerTrackingState
from src.models.game_session_model import GameSessionModel
from src.services.camera_service import CameraService
from src.services.floor_mapping_service import FloorMappingService
from src.services.game_engine_service import GameEngineService
from src.services.player_tracker_service import PlayerTrackerService
from src.services.calibration_service import CalibrationService
from src.services.pose_tracking_service import PoseTrackingService
from src.utils.app_paths import application_root
from src.utils.config import AppConfig, CameraProfile, ConfigStore, POSE_MODEL_OPTIONS
from src.viewmodels.calibration_viewmodel import CalibrationViewModel
from src.viewmodels.debug_viewmodel import DebugViewModel
from src.viewmodels.game_viewmodel import GameViewModel
from src.viewmodels.projector_viewmodel import ProjectorViewModel


class MainViewModel(QObject):
    status_changed = pyqtSignal(str)
    session_changed = pyqtSignal(object)
    projector_screen_changed = pyqtSignal(int)
    camera_capture_interval_changed = pyqtSignal(int)

    def __init__(
        self,
        config: AppConfig,
        config_store: ConfigStore,
        session_model: GameSessionModel,
        calibration_viewmodel: CalibrationViewModel,
        game_viewmodel: GameViewModel,
        projector_viewmodel: ProjectorViewModel,
        debug_viewmodel: DebugViewModel,
        camera_service: CameraService,
        pose_tracking_service: PoseTrackingService,
        floor_mapping_service: FloorMappingService,
        game_engine_service: GameEngineService,
        player_tracker_service: PlayerTrackerService,
    ) -> None:
        super().__init__()
        self._logger = logging.getLogger(__name__)
        self._config = config
        self._config_store = config_store
        self._session_model = session_model
        self.calibration_viewmodel = calibration_viewmodel
        self.game_viewmodel = game_viewmodel
        self.projector_viewmodel = projector_viewmodel
        self.debug_viewmodel = debug_viewmodel
        self._camera_service = camera_service
        self._pose_tracking_service = pose_tracking_service
        self._floor_mapping_service = floor_mapping_service
        self._game_engine_service = game_engine_service
        self._player_tracker_service = player_tracker_service
        self._latest_frame_packet: FramePacket | None = None

        self.calibration_viewmodel.calibration_updated.connect(self._on_calibration_updated)
        self.game_viewmodel.session_updated.connect(self._publish_session)
        self._logger.info("Main viewmodel initialized")

    @property
    def config(self) -> AppConfig:
        return self._config

    @property
    def session_model(self) -> GameSessionModel:
        return self._session_model

    def initialize(self) -> None:
        self._session_model.app_state = AppState.CAMERA_READY
        self._session_model.status_message = "Ready to calibrate"
        self._session_model.camera_status_message = "Waiting for camera frames"
        self._session_model.pose_status_message = "Pose tracking idle"
        self._session_model.display_status_message = (
            f"Projector assigned to display {self._config.display.projector_screen_index}"
        )
        self.projector_screen_changed.emit(self._config.display.projector_screen_index)
        self.camera_capture_interval_changed.emit(self._target_interval_ms())
        self._logger.info("Main viewmodel initialization complete")
        self._publish_session(self._session_model)

    def calibrate(self) -> None:
        self._session_model.app_state = AppState.CALIBRATING
        self._session_model.status_message = "Attempting ArUco calibration from latest camera frame"
        self.status_changed.emit(self._session_model.status_message)
        self.calibration_viewmodel.calibrate(self._latest_frame_packet)

    def reset_calibration(self) -> None:
        self.calibration_viewmodel.reset()

    def supported_aruco_dictionaries(self) -> list[str]:
        return CalibrationService.supported_dictionary_names()

    def supported_pose_models(self) -> list[tuple[str, str]]:
        root = application_root()
        supported: list[tuple[str, str]] = []
        for label, relative_path in POSE_MODEL_OPTIONS.items():
            candidate = Path(relative_path)
            if not candidate.is_absolute():
                candidate = root / candidate
            if candidate.exists() or self._config.pose.model_asset_path == relative_path:
                supported.append((label, relative_path))
        return supported

    def current_pose_model_path(self) -> str:
        return self._config.pose.model_asset_path

    def update_pose_model(self, model_asset_path: str) -> None:
        if self._config.pose.model_asset_path == model_asset_path:
            return

        resolved_path = Path(model_asset_path)
        if not resolved_path.is_absolute():
            resolved_path = application_root() / resolved_path

        self._config.pose.model_asset_path = model_asset_path
        self._pose_tracking_service.set_model_asset_path(resolved_path)
        self._session_model.pose_status_message = f"Pose model switched to {resolved_path.name}"
        self._session_model.status_message = f"Pose model set to {resolved_path.name}"
        self._logger.info("Pose model updated to %s", model_asset_path)
        self._publish_session(self._session_model)

    def update_aruco_dictionary(self, dictionary_name: str) -> None:
        if dictionary_name not in CalibrationService.supported_dictionary_names():
            return
        self._config.aruco_dictionary = dictionary_name
        self._session_model.status_message = f"ArUco dictionary set to {dictionary_name}"
        self._logger.info("ArUco dictionary updated to %s", dictionary_name)
        self._publish_session(self._session_model)

    def available_camera_indices(self) -> list[int]:
        return self._camera_service.available_camera_indices()

    def available_camera_profiles(self, probe: bool = False) -> list[CameraProfile]:
        return self._camera_service.available_camera_profiles(self._config.camera.camera_index, probe=probe)

    def update_camera_index(self, camera_index: int) -> None:
        self._config.camera.camera_index = camera_index
        self._camera_service.set_camera_index(camera_index)
        self._session_model.status_message = f"Switched to camera index {camera_index}"
        self._session_model.camera_status_message = (
            f"Camera {camera_index} selected at {self._config.camera.profile.label}. Waiting for live frames"
        )
        self._logger.info("Camera index updated to %s", camera_index)
        self._publish_session(self._session_model)

    def update_camera_profile(self, profile: CameraProfile) -> None:
        self._config.camera.apply_profile(profile)
        self._camera_service.set_camera_profile(profile)
        self.camera_capture_interval_changed.emit(self._target_interval_ms())
        self._session_model.status_message = f"Camera profile set to {profile.label}"
        self._session_model.camera_status_message = (
            f"Camera {self._config.camera.camera_index} selected at {profile.label}. Reconnect in progress"
        )
        if self._camera_service.reconnect():
            self._session_model.status_message = f"Camera profile applied: {profile.label}"
        else:
            detail = self._camera_service.last_error_message or "Live camera feed unavailable."
            self._session_model.camera_status_message = f"Fallback frame in use. {detail}"
            self._session_model.status_message = f"Camera profile failed: {profile.label}"
            self._logger.warning("Camera profile apply failed for %s: %s", profile.label, detail)
        self._publish_session(self._session_model)

    def _target_interval_ms(self) -> int:
        return max(1, int(round(1000 / max(self._config.camera.target_fps, 1))))

    def refresh_camera_sources(self) -> list[int]:
        indices = self._camera_service.available_camera_indices()
        if indices:
            self._session_model.status_message = f"Detected camera indices: {', '.join(str(index) for index in indices)}"
        else:
            self._session_model.status_message = "No camera indices detected"
        self._publish_session(self._session_model)
        return indices

    def reconnect_camera(self) -> None:
        if self._camera_service.reconnect():
            self._session_model.camera_status_message = (
                f"Camera {self._config.camera.camera_index} reconnected. Waiting for live frame confirmation"
            )
            self._session_model.status_message = f"Reconnect requested for camera {self._config.camera.camera_index}"
        else:
            detail = self._camera_service.last_error_message or "Live camera feed unavailable."
            self._session_model.camera_status_message = f"Fallback frame in use. {detail}"
            self._session_model.status_message = f"Reconnect failed for camera {self._config.camera.camera_index}"
            self._logger.warning("Camera reconnect failed: %s", detail)
        self._publish_session(self._session_model)

    def update_display_probe(self, screen_count: int) -> None:
        self._session_model.display_status_message = (
            f"Projector assigned to display {self._config.display.projector_screen_index} | "
            f"{screen_count} display(s) detected"
        )
        self.session_changed.emit(self._session_model)

    def update_projector_screen_index(self, screen_index: int) -> None:
        self._config.display.projector_screen_index = screen_index
        self.projector_screen_changed.emit(screen_index)
        self._session_model.status_message = f"Projector assigned to display {screen_index}"
        self._session_model.display_status_message = f"Projector assigned to display {screen_index}"
        self._logger.info("Projector screen updated to %s", screen_index)
        self._publish_session(self._session_model)

    def start_round(self) -> None:
        self.game_viewmodel.start_round()

    def lock_round(self) -> None:
        self.game_viewmodel.lock_round()

    def force_next_round(self) -> None:
        self.game_viewmodel.force_next_round()

    def reset_session(self) -> None:
        self.game_viewmodel.reset_session()

    def revive_player(self, player_id: str) -> None:
        self.game_viewmodel.revive_player(player_id)

    def eliminate_player(self, player_id: str) -> None:
        self.game_viewmodel.eliminate_player(player_id)

    def remove_player(self, player_id: str) -> None:
        self.game_viewmodel.remove_player(player_id)

    def update_grid_rows(self, value: int) -> None:
        self._config.grid.rows = value
        self._session_model.grid.rows = value
        self._session_model.grid.reset_states()
        self._publish_session(self._session_model)

    def update_grid_columns(self, value: int) -> None:
        self._config.grid.columns = value
        self._session_model.grid.columns = value
        self._session_model.grid.reset_states()
        self._publish_session(self._session_model)

    def update_timing_setting(self, field_name: str, value: float) -> None:
        if not hasattr(self._config.timings, field_name):
            return
        setattr(self._config.timings, field_name, value)
        setattr(self._session_model.round_state.timings, field_name, value)
        self._publish_session(self._session_model)

    def save_config(self) -> None:
        self._config_store.save(self._config)
        self.status_changed.emit(f"Saved settings to {self._config_store.config_path}")
        self._logger.info("Save settings requested")

    def handle_frame_packet(self, frame_packet: FramePacket) -> None:
        self._latest_frame_packet = frame_packet
        if frame_packet.is_live:
            height = 0 if frame_packet.frame is None else frame_packet.frame.shape[0]
            width = 0 if frame_packet.frame is None else frame_packet.frame.shape[1]
            fps_text = ""
            if frame_packet.actual_fps is not None and frame_packet.actual_fps > 0:
                fps_text = f" @ {frame_packet.actual_fps:.1f} FPS"
            self._session_model.camera_status_message = f"Camera {frame_packet.camera_index} live at {width}x{height}{fps_text}"
        else:
            detail = frame_packet.error_message or "Live camera feed unavailable."
            self._session_model.camera_status_message = f"Fallback frame in use. {detail}"
        render_state = self._game_engine_service.build_render_state(self._session_model)
        self.projector_viewmodel.update_render_state(
            render_state,
            frame_packet=self._latest_frame_packet,
            calibration=self._session_model.calibration,
        )
        self.debug_viewmodel.update_render_state(render_state)
        self.session_changed.emit(self._session_model)

    def handle_pose_result(self, pose_result: PoseResult) -> None:
        mapped_detections: list[MappedPlayerState] = []
        for index, pose_state in enumerate(pose_result.detections, start=1):
            mapped_detections.append(
                self._floor_mapping_service.map_detection(
                    player_id=f"detection-{index}",
                    pose_state=pose_state,
                    grid=self._session_model.grid,
                    calibration=self._session_model.calibration,
                )
            )

        self._player_tracker_service.update_players(
            self._session_model.players,
            mapped_detections,
            pose_result.timestamp,
            self._session_model.round_state.timings.missed_detection_grace_seconds,
        )

        mapped_count = sum(1 for detection in mapped_detections if detection.standing_point is not None)
        in_bounds_count = sum(1 for detection in mapped_detections if detection.in_bounds)

        active_count = sum(
            1
            for player in self._session_model.players.values()
            if player.tracking_state is PlayerTrackingState.ACTIVE
        )
        self._session_model.pose_status_message = (
            f"{pose_result.status_text} | mapped {mapped_count} | in bounds {in_bounds_count}"
        )
        self._session_model.status_message = f"Tracking {active_count} active player(s)"

        self._publish_session(self._session_model)

    def _on_calibration_updated(self, calibration_model: object) -> None:
        self._session_model.calibration = calibration_model
        self._session_model.app_state = AppState.CALIBRATED if calibration_model.is_valid else AppState.CAMERA_READY
        self._session_model.status_message = calibration_model.validation_message
        self._logger.info("Calibration update applied valid=%s", calibration_model.is_valid)
        self._publish_session(self._session_model)

    def _publish_session(self, session_model: GameSessionModel) -> None:
        render_state = self._game_engine_service.build_render_state(session_model)
        self.projector_viewmodel.update_render_state(
            render_state,
            frame_packet=self._latest_frame_packet,
            calibration=session_model.calibration,
        )
        self.debug_viewmodel.update_render_state(render_state)
        self.status_changed.emit(session_model.status_message)
        self.session_changed.emit(session_model)
