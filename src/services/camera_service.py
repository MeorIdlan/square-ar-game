from __future__ import annotations

from dataclasses import dataclass
import logging
from pathlib import Path
import sys

import cv2
import numpy as np

from src.models.contracts import FramePacket
from src.utils.config import CameraSettings


@dataclass(slots=True)
class CameraService:
    settings: CameraSettings
    _frame_id: int = 0
    _capture: cv2.VideoCapture | None = None
    _logger: logging.Logger | None = None
    _latest_frame: np.ndarray | None = None
    _last_error_message: str | None = None
    _is_live_source: bool = False

    def __post_init__(self) -> None:
        self._logger = logging.getLogger(__name__)
        self._logger.info(
            "Camera service initialized index=%s resolution=%sx%s fps=%s",
            self.settings.camera_index,
            self.settings.frame_width,
            self.settings.frame_height,
            self.settings.target_fps,
        )

    def open(self) -> bool:
        if self._capture is not None and self._capture.isOpened():
            self._last_error_message = None
            return True

        capture = cv2.VideoCapture(self.settings.camera_index)
        if not capture.isOpened():
            self._last_error_message = f"Unable to open camera index {self.settings.camera_index}."
            self._logger.warning("Unable to open camera index %s", self.settings.camera_index)
            self._capture = None
            return False

        capture.set(cv2.CAP_PROP_FRAME_WIDTH, self.settings.frame_width)
        capture.set(cv2.CAP_PROP_FRAME_HEIGHT, self.settings.frame_height)
        capture.set(cv2.CAP_PROP_FPS, self.settings.target_fps)
        self._capture = capture
        self._last_error_message = None
        self._logger.info("Opened camera index %s", self.settings.camera_index)
        return True

    def set_camera_index(self, camera_index: int) -> None:
        if camera_index == self.settings.camera_index:
            return
        self._logger.info("Switching camera index from %s to %s", self.settings.camera_index, camera_index)
        self.release()
        self.settings.camera_index = camera_index

    def available_camera_indices(self, max_index: int = 5) -> list[int]:
        if sys.platform.startswith("linux"):
            linux_indices = self._linux_video_device_indices(max_index)
            if self.settings.camera_index not in linux_indices:
                linux_indices.append(self.settings.camera_index)
            return sorted(set(linux_indices))

        available: list[int] = []
        for index in range(max_index + 1):
            probe = cv2.VideoCapture(index)
            if probe.isOpened():
                available.append(index)
                probe.release()
                continue
            probe.release()
        if self.settings.camera_index not in available:
            available.append(self.settings.camera_index)
        return sorted(set(available))

    @staticmethod
    def _linux_video_device_indices(max_index: int) -> list[int]:
        indices: list[int] = []
        for path in sorted(Path("/dev").glob("video*")):
            suffix = path.name.replace("video", "", 1)
            if suffix.isdigit():
                index = int(suffix)
                if index <= max_index:
                    indices.append(index)
        return indices

    def release(self) -> None:
        if self._capture is not None:
            self._capture.release()
            self._logger.info("Released camera index %s", self.settings.camera_index)
        self._capture = None
        self._is_live_source = False

    def reconnect(self) -> bool:
        self._logger.info("Reconnect requested for camera index %s", self.settings.camera_index)
        self.release()
        return self.open()

    @property
    def latest_frame(self) -> np.ndarray | None:
        if self._latest_frame is None:
            return None
        return self._latest_frame.copy()

    @property
    def last_error_message(self) -> str | None:
        return self._last_error_message

    def next_frame(self) -> FramePacket:
        if self.open() and self._capture is not None:
            ok, frame = self._capture.read()
            if ok and frame is not None:
                self._frame_id += 1
                self._latest_frame = frame.copy()
                self._last_error_message = None
                if not self._is_live_source:
                    self._logger.info("Camera index %s is now delivering live frames", self.settings.camera_index)
                self._is_live_source = True
                return FramePacket(
                    frame_id=self._frame_id,
                    frame=frame,
                    camera_index=self.settings.camera_index,
                    is_live=True,
                    source_name="camera",
                )

            self._last_error_message = f"Camera read failed for camera index {self.settings.camera_index}."
            self._logger.warning("Camera read failed for camera index %s", self.settings.camera_index)

        frame = self._fallback_frame()
        self._frame_id += 1
        self._latest_frame = frame.copy()
        if self._is_live_source:
            self._logger.warning("Camera index %s lost live frames; switching to fallback", self.settings.camera_index)
        self._is_live_source = False
        return FramePacket(
            frame_id=self._frame_id,
            frame=frame,
            camera_index=self.settings.camera_index,
            is_live=False,
            source_name="fallback",
            error_message=self._last_error_message or "Live camera feed unavailable.",
        )

    def _fallback_frame(self) -> np.ndarray:
        height = self.settings.frame_height
        width = self.settings.frame_width
        frame = np.zeros((height, width, 3), dtype=np.uint8)
        cv2.putText(
            frame,
            "Camera unavailable",
            (40, 80),
            cv2.FONT_HERSHEY_SIMPLEX,
            1.2,
            (0, 0, 255),
            2,
            cv2.LINE_AA,
        )
        cv2.putText(
            frame,
            f"Index {self.settings.camera_index}",
            (40, 130),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.9,
            (255, 255, 255),
            2,
            cv2.LINE_AA,
        )
        return frame
