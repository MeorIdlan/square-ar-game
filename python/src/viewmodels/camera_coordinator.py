from __future__ import annotations

import logging

from PyQt6.QtCore import QObject, pyqtSignal

from src.services.protocols import CameraServiceProtocol
from src.utils.config import AppConfig, CameraProfile


class CameraCoordinator(QObject):
    camera_status_changed = pyqtSignal(str)
    capture_interval_changed = pyqtSignal(int)

    def __init__(
        self, config: AppConfig, camera_service: CameraServiceProtocol
    ) -> None:
        super().__init__()
        self._config = config
        self._camera_service = camera_service
        self._logger = logging.getLogger(__name__)

    def available_camera_indices(self, probe: bool = True) -> list[int]:
        return self._camera_service.available_camera_indices(probe=probe)

    def available_camera_profiles(self, probe: bool = False) -> list[CameraProfile]:
        return self._camera_service.available_camera_profiles(
            self._config.camera.camera_index, probe=probe
        )

    def update_camera_index(self, camera_index: int) -> None:
        self._config.camera.camera_index = camera_index
        self._camera_service.set_camera_index(camera_index)
        status = f"Camera {camera_index} selected at {self._config.camera.profile.label}. Waiting for live frames"
        self._logger.info("Camera index updated to %s", camera_index)
        self.camera_status_changed.emit(status)

    def update_camera_profile(self, profile: CameraProfile) -> None:
        self._config.camera.apply_profile(profile)
        self._camera_service.set_camera_profile(profile)
        self.capture_interval_changed.emit(self.target_interval_ms())
        status = f"Camera {self._config.camera.camera_index} set to {profile.label}. Waiting for live frames"
        self.camera_status_changed.emit(status)

    def reconnect(self) -> None:
        self._camera_service.release()
        status = f"Camera {self._config.camera.camera_index} reconnecting. Waiting for live frames"
        self._logger.info(
            "Camera reconnect requested for index %s", self._config.camera.camera_index
        )
        self.camera_status_changed.emit(status)

    def refresh_sources(self) -> list[int]:
        return self._camera_service.available_camera_indices(probe=False)

    def target_interval_ms(self) -> int:
        return max(1, int(round(1000 / max(self._config.camera.target_fps, 1))))
