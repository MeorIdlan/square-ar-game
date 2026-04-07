from __future__ import annotations

import logging
import sys
import threading
from dataclasses import dataclass, field
from pathlib import Path
from typing import ClassVar

import cv2
import numpy as np

from src.models.contracts import FramePacket
from src.utils.config import CameraProfile, CameraSettings


@dataclass(slots=True)
class CameraService:
    COMMON_PROFILES: ClassVar[tuple[CameraProfile, ...]] = (
        CameraProfile(1280, 720, 30),
        CameraProfile(1280, 720, 60),
        CameraProfile(1920, 1080, 30),
        CameraProfile(1920, 1080, 60),
        CameraProfile(2560, 1440, 30),
        CameraProfile(3840, 2160, 30),
    )

    settings: CameraSettings
    _frame_id: int = 0
    _capture: cv2.VideoCapture | None = None
    _logger: logging.Logger | None = None
    _latest_frame: np.ndarray | None = None
    _last_error_message: str | None = None
    _is_live_source: bool = False
    _last_reported_profile_label: str | None = None
    _opening: bool = False
    _open_thread: threading.Thread | None = None
    _open_generation: int = 0
    _lock: threading.Lock = field(default_factory=threading.Lock)

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

        if self._opening:
            return False

        self._opening = True
        self._open_generation += 1
        generation = self._open_generation
        self._open_thread = threading.Thread(
            target=self._open_in_background,
            args=(generation,),
            daemon=True,
        )
        self._open_thread.start()
        return False

    def _open_in_background(self, generation: int) -> None:
        capture = cv2.VideoCapture(self.settings.camera_index)
        with self._lock:
            if generation != self._open_generation:
                capture.release()
                return
            if not capture.isOpened():
                self._last_error_message = (
                    f"Unable to open camera index {self.settings.camera_index}."
                )
                self._logger.warning(
                    "Unable to open camera index %s", self.settings.camera_index
                )
                self._capture = None
                self._opening = False
                return

            capture.set(cv2.CAP_PROP_FRAME_WIDTH, self.settings.frame_width)
            capture.set(cv2.CAP_PROP_FRAME_HEIGHT, self.settings.frame_height)
            capture.set(cv2.CAP_PROP_FPS, self.settings.target_fps)
            self._capture = capture
            self._last_error_message = None
            self._last_reported_profile_label = None
            self._opening = False
            self._logger.info("Opened camera index %s", self.settings.camera_index)

    def set_camera_index(self, camera_index: int) -> None:
        if camera_index == self.settings.camera_index:
            return
        self._logger.info(
            "Switching camera index from %s to %s",
            self.settings.camera_index,
            camera_index,
        )
        self.release()
        self.settings.camera_index = camera_index

    def set_camera_profile(self, profile: CameraProfile) -> None:
        requested_label = profile.label
        current_profile = self.settings.profile
        if current_profile == profile:
            return
        self._logger.info(
            "Switching camera profile from %s to %s",
            current_profile.label,
            requested_label,
        )
        self.release()
        self.settings.apply_profile(profile)

    def available_camera_profiles(
        self, camera_index: int | None = None, probe: bool = False
    ) -> list[CameraProfile]:
        probe_index = (
            self.settings.camera_index if camera_index is None else camera_index
        )
        profiles: list[CameraProfile] = []

        if probe:
            for profile in self.COMMON_PROFILES:
                actual = self._probe_profile(probe_index, profile)
                if actual is None:
                    continue
                if actual.width == profile.width and actual.height == profile.height:
                    profiles.append(profile)
        else:
            profiles.extend(self.COMMON_PROFILES)

        current_profile = self.settings.profile
        if current_profile not in profiles:
            profiles.append(current_profile)

        unique_profiles = sorted(
            {
                (profile.width, profile.height, profile.fps): profile
                for profile in profiles
            }.values(),
            key=lambda profile: (profile.width * profile.height, profile.fps),
        )
        if probe:
            self._logger.info(
                "Probed %s camera profile(s) for camera index %s: %s",
                len(unique_profiles),
                probe_index,
                ", ".join(profile.label for profile in unique_profiles),
            )
        else:
            self._logger.info(
                "Loaded %s preset camera profile(s) for camera index %s",
                len(unique_profiles),
                probe_index,
            )
        return unique_profiles

    def _probe_profile(
        self, camera_index: int, profile: CameraProfile
    ) -> CameraProfile | None:
        probe = cv2.VideoCapture(camera_index)
        if not probe.isOpened():
            probe.release()
            return None

        probe.set(cv2.CAP_PROP_FRAME_WIDTH, profile.width)
        probe.set(cv2.CAP_PROP_FRAME_HEIGHT, profile.height)
        probe.set(cv2.CAP_PROP_FPS, profile.fps)
        ok, frame = probe.read()
        if not ok or frame is None:
            probe.release()
            return None

        actual_width = int(round(probe.get(cv2.CAP_PROP_FRAME_WIDTH))) or frame.shape[1]
        actual_height = (
            int(round(probe.get(cv2.CAP_PROP_FRAME_HEIGHT))) or frame.shape[0]
        )
        actual_fps = int(round(probe.get(cv2.CAP_PROP_FPS))) or profile.fps
        probe.release()
        return CameraProfile(actual_width, actual_height, actual_fps)

    def available_camera_indices(
        self, max_index: int = 5, probe: bool = True
    ) -> list[int]:
        if sys.platform.startswith("linux"):
            linux_indices = self._linux_video_device_indices(max_index)
            if self.settings.camera_index not in linux_indices:
                linux_indices.append(self.settings.camera_index)
            return sorted(set(linux_indices))

        if not probe:
            indices = list(range(max_index + 1))
            if self.settings.camera_index not in indices:
                indices.append(self.settings.camera_index)
            return sorted(set(indices))

        available: list[int] = []
        for index in range(max_index + 1):
            cap = cv2.VideoCapture(index)
            if cap.isOpened():
                available.append(index)
                cap.release()
                continue
            cap.release()
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
        with self._lock:
            self._open_generation += 1
            self._opening = False
            if self._capture is not None:
                self._capture.release()
                self._logger.info(
                    "Released camera index %s", self.settings.camera_index
                )
            self._capture = None
            self._is_live_source = False

    def reconnect(self) -> bool:
        self._logger.info(
            "Reconnect requested for camera index %s", self.settings.camera_index
        )
        self.release()
        return self._open_sync()

    def _open_sync(self) -> bool:
        """Blocking open — used only for explicit user-initiated reconnect."""
        if self._capture is not None and self._capture.isOpened():
            self._last_error_message = None
            return True

        capture = cv2.VideoCapture(self.settings.camera_index)
        if not capture.isOpened():
            self._last_error_message = (
                f"Unable to open camera index {self.settings.camera_index}."
            )
            self._logger.warning(
                "Unable to open camera index %s", self.settings.camera_index
            )
            self._capture = None
            return False

        capture.set(cv2.CAP_PROP_FRAME_WIDTH, self.settings.frame_width)
        capture.set(cv2.CAP_PROP_FRAME_HEIGHT, self.settings.frame_height)
        capture.set(cv2.CAP_PROP_FPS, self.settings.target_fps)
        self._capture = capture
        self._last_error_message = None
        self._last_reported_profile_label = None
        self._logger.info("Opened camera index %s", self.settings.camera_index)
        return True

    @property
    def latest_frame(self) -> np.ndarray | None:
        if self._latest_frame is None:
            return None
        return self._latest_frame.copy()

    @property
    def last_error_message(self) -> str | None:
        return self._last_error_message

    def next_frame(self) -> FramePacket:
        self.open()
        capture = self._capture
        if capture is not None:
            ok, frame = capture.read()
            if ok and frame is not None:
                self._frame_id += 1
                self._latest_frame = frame.copy()
                self._last_error_message = None
                if not self._is_live_source:
                    self._logger.info(
                        "Camera index %s is now delivering live frames",
                        self.settings.camera_index,
                    )
                actual_profile = self._frame_profile(frame)
                if actual_profile.label != self._last_reported_profile_label:
                    self._logger.info(
                        "Camera index %s active stream profile %s",
                        self.settings.camera_index,
                        actual_profile.label,
                    )
                    self._last_reported_profile_label = actual_profile.label
                self._is_live_source = True
                return FramePacket(
                    frame_id=self._frame_id,
                    frame=frame,
                    camera_index=self.settings.camera_index,
                    is_live=True,
                    source_name="camera",
                    actual_fps=capture.get(cv2.CAP_PROP_FPS),
                )

            self._last_error_message = (
                f"Camera read failed for camera index {self.settings.camera_index}."
            )
            self._logger.warning(
                "Camera read failed for camera index %s", self.settings.camera_index
            )

        frame = self._fallback_frame()
        self._frame_id += 1
        self._latest_frame = frame.copy()
        if self._is_live_source:
            self._logger.warning(
                "Camera index %s lost live frames; switching to fallback",
                self.settings.camera_index,
            )
        self._is_live_source = False
        return FramePacket(
            frame_id=self._frame_id,
            frame=frame,
            camera_index=self.settings.camera_index,
            is_live=False,
            source_name="fallback",
            error_message=self._last_error_message or "Live camera feed unavailable.",
            actual_fps=float(self.settings.target_fps),
        )

    @staticmethod
    def _frame_profile(frame: np.ndarray) -> CameraProfile:
        return CameraProfile(
            width=int(frame.shape[1]), height=int(frame.shape[0]), fps=0
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
