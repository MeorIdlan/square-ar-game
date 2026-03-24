from __future__ import annotations

from dataclasses import dataclass
import logging

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

    def __post_init__(self) -> None:
        self._logger = logging.getLogger(__name__)

    def open(self) -> bool:
        if self._capture is not None and self._capture.isOpened():
            return True

        capture = cv2.VideoCapture(self.settings.camera_index)
        if not capture.isOpened():
            self._logger.warning("Unable to open camera index %s", self.settings.camera_index)
            self._capture = None
            return False

        capture.set(cv2.CAP_PROP_FRAME_WIDTH, self.settings.frame_width)
        capture.set(cv2.CAP_PROP_FRAME_HEIGHT, self.settings.frame_height)
        capture.set(cv2.CAP_PROP_FPS, self.settings.target_fps)
        self._capture = capture
        return True

    def release(self) -> None:
        if self._capture is not None:
            self._capture.release()
        self._capture = None

    @property
    def latest_frame(self) -> np.ndarray | None:
        if self._latest_frame is None:
            return None
        return self._latest_frame.copy()

    def next_frame(self) -> FramePacket:
        if self.open() and self._capture is not None:
            ok, frame = self._capture.read()
            if ok and frame is not None:
                self._frame_id += 1
                self._latest_frame = frame.copy()
                return FramePacket(frame_id=self._frame_id, frame=frame)

            self._logger.warning("Camera read failed for camera index %s", self.settings.camera_index)

        frame = self._fallback_frame()
        self._frame_id += 1
        self._latest_frame = frame.copy()
        return FramePacket(frame_id=self._frame_id, frame=frame)

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
