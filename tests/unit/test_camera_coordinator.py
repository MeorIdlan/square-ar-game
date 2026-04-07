from __future__ import annotations

from unittest.mock import MagicMock

from src.utils.config import AppConfig, CameraProfile
from src.viewmodels.camera_coordinator import CameraCoordinator


def _make_coordinator() -> tuple[CameraCoordinator, MagicMock]:
    config = AppConfig()
    mock_camera = MagicMock()
    mock_camera.available_camera_indices.return_value = [0, 1]
    mock_camera.available_camera_profiles.return_value = [CameraProfile(1280, 720, 30)]
    mock_camera.reconnect.return_value = True
    mock_camera.last_error_message = None
    coordinator = CameraCoordinator(config, mock_camera)
    return coordinator, mock_camera


class TestCameraCoordinator:
    def test_update_camera_index_emits_signal(self, qtbot) -> None:
        coordinator, mock_camera = _make_coordinator()
        with qtbot.waitSignal(coordinator.camera_status_changed, timeout=1000):
            coordinator.update_camera_index(2)
        mock_camera.set_camera_index.assert_called_once_with(2)

    def test_reconnect_success_emits_status(self, qtbot) -> None:
        coordinator, mock_camera = _make_coordinator()
        with qtbot.waitSignal(coordinator.camera_status_changed, timeout=1000):
            coordinator.reconnect()
        mock_camera.release.assert_called_once()

    def test_reconnect_emits_waiting_status(self, qtbot) -> None:
        coordinator, mock_camera = _make_coordinator()
        signals = []
        coordinator.camera_status_changed.connect(signals.append)
        coordinator.reconnect()
        assert len(signals) == 1
        assert "Waiting" in signals[0]

    def test_available_camera_profiles(self) -> None:
        coordinator, mock_camera = _make_coordinator()
        profiles = coordinator.available_camera_profiles()
        assert len(profiles) == 1
        mock_camera.available_camera_profiles.assert_called_once()

    def test_target_interval_ms(self) -> None:
        coordinator, _ = _make_coordinator()
        interval = coordinator.target_interval_ms()
        assert interval == 33  # 1000/30 ≈ 33

    def test_refresh_sources(self) -> None:
        coordinator, mock_camera = _make_coordinator()
        result = coordinator.refresh_sources()
        assert result == [0, 1]
        mock_camera.available_camera_indices.assert_called_once_with(probe=False)
