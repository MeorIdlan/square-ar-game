from __future__ import annotations

import logging
from pathlib import Path

import cv2
import mediapipe as mp

from src.models.contracts import FramePacket, PoseResult
from src.models.contracts import Point, PoseFootState
from src.utils.config import PoseSettings


class PoseTrackingService:
    def __init__(self, settings: PoseSettings, model_asset_path: Path) -> None:
        self._settings = settings
        self._model_asset_path = model_asset_path
        self._logger = logging.getLogger(__name__)
        self._landmarker: object | None = None
        self._initialization_failed = False
        self._last_status_text: str | None = None
        self._missing_model_logged = False
        self._logger.info(
            "Pose tracking initialized model=%s num_poses=%s thresholds=(detect=%s presence=%s tracking=%s landmark=%s)",
            self._model_asset_path,
            self._settings.num_poses,
            self._settings.min_pose_detection_confidence,
            self._settings.min_pose_presence_confidence,
            self._settings.min_tracking_confidence,
            self._settings.min_landmark_visibility,
        )

    @property
    def model_asset_path(self) -> Path:
        return self._model_asset_path

    def set_model_asset_path(self, model_asset_path: Path) -> None:
        if self._model_asset_path == model_asset_path:
            return
        self.close()
        self._model_asset_path = model_asset_path
        self._initialization_failed = False
        self._last_status_text = None
        self._missing_model_logged = False
        self._logger.info("Pose model switched to %s", self._model_asset_path)

    def process_frame(self, frame_packet: FramePacket) -> PoseResult:
        frame = frame_packet.frame
        if frame is None:
            return PoseResult(frame_id=frame_packet.frame_id, status_text="No frame available for pose tracking")

        landmarker = self._ensure_landmarker()
        if landmarker is None:
            return PoseResult(frame_id=frame_packet.frame_id, status_text="Pose landmarker unavailable")

        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)
        timestamp_ms = max(int(frame_packet.timestamp * 1000), 0)
        result = landmarker.detect_for_video(mp_image, timestamp_ms)

        detections: list[PoseFootState] = []
        pose_landmarks = getattr(result, "pose_landmarks", [])
        for landmarks in pose_landmarks:
            foot_state = self._extract_pose_foot_state(landmarks, frame.shape[1], frame.shape[0])
            if foot_state is not None:
                detections.append(foot_state)

        raw_pose_count = len(pose_landmarks)
        if raw_pose_count == 0:
            status_text = "Pose detector found 0 bodies"
        elif not detections:
            status_text = f"Pose detector found {raw_pose_count} body(s), but foot landmarks were below visibility thresholds"
        else:
            status_text = f"Pose detector found {len(detections)} usable pose(s) from {raw_pose_count} body(s)"

        if status_text != self._last_status_text:
            self._logger.info("%s", status_text)
            self._last_status_text = status_text

        return PoseResult(
            frame_id=frame_packet.frame_id,
            timestamp=frame_packet.timestamp,
            detections=detections,
            raw_pose_count=raw_pose_count,
            status_text=status_text,
        )

    def close(self) -> None:
        if self._landmarker is not None:
            self._landmarker.close()
        self._landmarker = None

    def _ensure_landmarker(self):
        if self._initialization_failed:
            return None

        if self._landmarker is not None:
            return self._landmarker

        if not self._model_asset_path.exists():
            if not self._missing_model_logged:
                self._logger.warning("Pose model asset not found at %s", self._model_asset_path)
                self._missing_model_logged = True
            self._initialization_failed = True
            return None

        options = mp.tasks.vision.PoseLandmarkerOptions(
            base_options=mp.tasks.BaseOptions(model_asset_path=str(self._model_asset_path)),
            running_mode=mp.tasks.vision.RunningMode.VIDEO,
            num_poses=self._settings.num_poses,
            min_pose_detection_confidence=self._settings.min_pose_detection_confidence,
            min_pose_presence_confidence=self._settings.min_pose_presence_confidence,
            min_tracking_confidence=self._settings.min_tracking_confidence,
            output_segmentation_masks=False,
        )
        try:
            self._landmarker = mp.tasks.vision.PoseLandmarker.create_from_options(options)
        except (OSError, RuntimeError, ValueError) as error:
            self._initialization_failed = True
            self._logger.warning("Unable to initialize MediaPipe pose landmarker: %s", error)
            return None
        self._logger.info("MediaPipe pose landmarker initialized successfully")
        return self._landmarker

    def _extract_pose_foot_state(self, landmarks: list[object], width: int, height: int) -> PoseFootState | None:
        left_foot, left_score = self._extract_weighted_foot_point(landmarks, (27, 29, 31), width, height)
        right_foot, right_score = self._extract_weighted_foot_point(landmarks, (28, 30, 32), width, height)

        if left_foot is None and right_foot is None:
            return None

        visible_scores = [score for score in (left_score, right_score) if score > 0.0]
        confidence = sum(visible_scores) / len(visible_scores) if visible_scores else 0.0
        resolved_point = None
        if left_foot and right_foot:
            resolved_point = ((left_foot[0] + right_foot[0]) / 2.0, (left_foot[1] + right_foot[1]) / 2.0)
        else:
            resolved_point = left_foot or right_foot

        return PoseFootState(
            left_foot=left_foot,
            right_foot=right_foot,
            resolved_point=resolved_point,
            confidence=confidence,
        )

    def _extract_weighted_foot_point(
        self,
        landmarks: list[object],
        indices: tuple[int, int, int],
        width: int,
        height: int,
    ) -> tuple[Point | None, float]:
        weighted_points: list[tuple[float, Point]] = []
        for index in indices:
            landmark = landmarks[index]
            visibility = float(getattr(landmark, "visibility", 0.0))
            presence = float(getattr(landmark, "presence", visibility))
            confidence = min(visibility, presence)
            if confidence < self._settings.min_landmark_visibility:
                continue

            x = float(getattr(landmark, "x", 0.0))
            y = float(getattr(landmark, "y", 0.0))
            if not (0.0 <= x <= 1.0 and 0.0 <= y <= 1.0):
                continue

            weighted_points.append((confidence, (x * width, y * height)))

        if not weighted_points:
            return None, 0.0

        total_weight = sum(weight for weight, _ in weighted_points)
        resolved_point = (
            sum(weight * point[0] for weight, point in weighted_points) / total_weight,
            sum(weight * point[1] for weight, point in weighted_points) / total_weight,
        )
        return resolved_point, total_weight / len(weighted_points)
