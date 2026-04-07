from __future__ import annotations

from unittest.mock import MagicMock, patch

import numpy as np

from src.services.camera_service import CameraService
from src.utils.config import CameraProfile, CameraSettings


class TestCameraServiceOpen:
    def test_open_valid_index(self) -> None:
        settings = CameraSettings(camera_index=0)
        service = CameraService(settings=settings)
        mock_cap = MagicMock()
        mock_cap.isOpened.return_value = True
        with patch(
            "src.services.camera_service.cv2.VideoCapture", return_value=mock_cap
        ):
            # First call kicks off background thread, returns False
            assert service.open() is False
            if service._open_thread is not None:
                service._open_thread.join(timeout=5)
            # Second call returns True because capture is now set
            assert service.open() is True

    def test_open_invalid_index(self) -> None:
        settings = CameraSettings(camera_index=99)
        service = CameraService(settings=settings)
        mock_cap = MagicMock()
        mock_cap.isOpened.return_value = False
        with patch(
            "src.services.camera_service.cv2.VideoCapture", return_value=mock_cap
        ):
            assert service.open() is False
            if service._open_thread is not None:
                service._open_thread.join(timeout=5)
            assert service.last_error_message is not None


class TestCameraServiceNextFrame:
    def test_returns_live_packet_when_camera_open(self) -> None:
        settings = CameraSettings(camera_index=0)
        service = CameraService(settings=settings)
        fake_frame = np.zeros((720, 1280, 3), dtype=np.uint8)
        mock_cap = MagicMock()
        mock_cap.isOpened.return_value = True
        mock_cap.read.return_value = (True, fake_frame)
        mock_cap.get.return_value = 30.0
        with patch(
            "src.services.camera_service.cv2.VideoCapture", return_value=mock_cap
        ):
            service.open()
            if service._open_thread is not None:
                service._open_thread.join(timeout=5)
            packet = service.next_frame()
            assert packet.is_live is True
            assert packet.frame is not None
            assert packet.source_name == "camera"

    def test_returns_fallback_when_camera_not_open(self) -> None:
        settings = CameraSettings(camera_index=0)
        service = CameraService(settings=settings)
        mock_cap = MagicMock()
        mock_cap.isOpened.return_value = False
        with patch(
            "src.services.camera_service.cv2.VideoCapture", return_value=mock_cap
        ):
            packet = service.next_frame()
            assert packet.is_live is False
            assert packet.source_name == "fallback"
            assert packet.frame is not None


class TestCameraServiceSetProfile:
    def test_set_camera_profile_updates_settings(self) -> None:
        settings = CameraSettings(camera_index=0)
        service = CameraService(settings=settings)
        new_profile = CameraProfile(1920, 1080, 60)
        service.set_camera_profile(new_profile)
        assert settings.frame_width == 1920
        assert settings.frame_height == 1080
        assert settings.target_fps == 60

    def test_same_profile_is_noop(self) -> None:
        settings = CameraSettings(
            camera_index=0, frame_width=1280, frame_height=720, target_fps=30
        )
        service = CameraService(settings=settings)
        service.set_camera_profile(CameraProfile(1280, 720, 30))
        assert settings.frame_width == 1280


class TestCameraServiceAvailableIndices:
    def test_linux_parses_video_devices(self, tmp_path) -> None:
        dev_path = tmp_path / "dev"
        dev_path.mkdir()
        (dev_path / "video0").touch()
        (dev_path / "video1").touch()
        (dev_path / "video4").touch()

        settings = CameraSettings(camera_index=0)
        service = CameraService(settings=settings)
        with (
            patch("src.services.camera_service.sys") as mock_sys,
            patch("src.services.camera_service.Path") as mock_path_cls,
        ):
            mock_sys.platform = "linux"
            mock_dev = MagicMock()
            mock_dev.glob.return_value = sorted(
                [
                    dev_path / "video0",
                    dev_path / "video1",
                    dev_path / "video4",
                ]
            )
            mock_path_cls.return_value = mock_dev
            indices = service.available_camera_indices(max_index=5)
            assert 0 in indices


class TestCameraServiceAsyncOpen:
    """open() should not block when camera is slow to initialize."""

    def test_open_returns_false_while_background_thread_running(self) -> None:
        """First call to open() should kick off background thread and return False."""
        import threading

        settings = CameraSettings(camera_index=0)
        service = CameraService(settings=settings)

        barrier = threading.Barrier(2, timeout=5)

        original_video_capture = None

        def slow_capture(index):
            nonlocal original_video_capture
            barrier.wait()  # block until test signals
            mock_cap = MagicMock()
            mock_cap.isOpened.return_value = True
            mock_cap.get.return_value = 30.0
            original_video_capture = mock_cap
            return mock_cap

        with patch(
            "src.services.camera_service.cv2.VideoCapture", side_effect=slow_capture
        ):
            # First open should start background thread and return False
            result = service.open()
            assert result is False
            assert service._opening is True
            # Let the background thread complete
            barrier.wait()
            # Wait for background thread to finish
            service._open_thread.join(timeout=5)
            assert service._opening is False
            assert service._capture is not None

    def test_next_frame_returns_fallback_while_opening(self) -> None:
        """next_frame must return fallback immediately while camera opens."""
        import threading

        settings = CameraSettings(camera_index=0)
        service = CameraService(settings=settings)

        barrier = threading.Barrier(2, timeout=5)

        def slow_capture(index):
            barrier.wait()
            mock_cap = MagicMock()
            mock_cap.isOpened.return_value = True
            mock_cap.get.return_value = 30.0
            return mock_cap

        with patch(
            "src.services.camera_service.cv2.VideoCapture", side_effect=slow_capture
        ):
            service.open()  # starts background thread
            packet = service.next_frame()
            assert packet.is_live is False
            assert packet.source_name == "fallback"
            # release the background thread
            barrier.wait()
            service._open_thread.join(timeout=5)

    def test_open_returns_true_after_background_completes(self) -> None:
        """After background open succeeds, open() returns True immediately."""
        settings = CameraSettings(camera_index=0)
        service = CameraService(settings=settings)
        mock_cap = MagicMock()
        mock_cap.isOpened.return_value = True
        mock_cap.get.return_value = 30.0

        with patch(
            "src.services.camera_service.cv2.VideoCapture", return_value=mock_cap
        ):
            service.open()
            if service._open_thread is not None:
                service._open_thread.join(timeout=5)
            # Now open should return True because capture is set
            assert service.open() is True

    def test_release_cancels_pending_open(self) -> None:
        """release() while background open is in progress should discard the result."""
        import threading

        settings = CameraSettings(camera_index=0)
        service = CameraService(settings=settings)

        barrier = threading.Barrier(2, timeout=5)

        def slow_capture(index):
            barrier.wait()
            mock_cap = MagicMock()
            mock_cap.isOpened.return_value = True
            return mock_cap

        with patch(
            "src.services.camera_service.cv2.VideoCapture", side_effect=slow_capture
        ):
            service.open()
            # release while still opening
            service.release()
            assert service._opening is False
            # let background thread finish
            barrier.wait()
            if service._open_thread is not None:
                service._open_thread.join(timeout=5)
            # capture should still be None because release was called
            assert service._capture is None
