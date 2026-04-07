from __future__ import annotations

from pathlib import Path
from unittest.mock import MagicMock

import numpy as np

from src.models.contracts import FramePacket
from src.services.pose_tracking_service import PoseTrackingService
from src.utils.config import PoseSettings


def _make_service(model_exists: bool = True) -> PoseTrackingService:
    settings = PoseSettings(
        min_pose_detection_confidence=0.35,
        min_pose_presence_confidence=0.35,
        min_tracking_confidence=0.35,
        min_landmark_visibility=0.2,
        use_gpu_if_available=False,
    )
    path = Path("/fake/model.task")
    service = PoseTrackingService(settings, path)
    if not model_exists:
        service._initialization_failed = True
    return service


def _make_frame_packet() -> FramePacket:
    return FramePacket(
        frame_id=1,
        frame=np.zeros((480, 640, 3), dtype=np.uint8),
        is_live=True,
    )


class TestProcessFrame:
    def test_no_frame_returns_empty(self) -> None:
        service = _make_service()
        packet = FramePacket(frame_id=1, frame=None, is_live=False)
        result = service.process_frame(packet)
        assert result.detections == []
        assert "No frame" in result.status_text

    def test_missing_landmarker_returns_empty(self) -> None:
        service = _make_service(model_exists=False)
        packet = _make_frame_packet()
        result = service.process_frame(packet)
        assert result.detections == []
        assert "unavailable" in result.status_text

    def test_stub_landmarks_produce_detections(self) -> None:
        service = _make_service()
        packet = _make_frame_packet()

        # Create stub landmark objects
        def make_landmark(x, y, vis=0.9, pres=0.9):
            lm = MagicMock()
            lm.x = x
            lm.y = y
            lm.visibility = vis
            lm.presence = pres
            return lm

        landmarks = [make_landmark(0.5, 0.5)] * 33
        # Set foot-specific landmarks
        landmarks[27] = make_landmark(0.3, 0.8, 0.9, 0.9)
        landmarks[29] = make_landmark(0.31, 0.81, 0.85, 0.85)
        landmarks[31] = make_landmark(0.32, 0.82, 0.8, 0.8)
        landmarks[28] = make_landmark(0.4, 0.8, 0.9, 0.9)
        landmarks[30] = make_landmark(0.41, 0.81, 0.85, 0.85)
        landmarks[32] = make_landmark(0.42, 0.82, 0.8, 0.8)

        mock_result = MagicMock()
        mock_result.pose_landmarks = [landmarks]

        mock_landmarker = MagicMock()
        mock_landmarker.detect_for_video.return_value = mock_result

        service._landmarker = mock_landmarker
        service._initialization_failed = False

        result = service.process_frame(packet)
        assert len(result.detections) == 1
        assert result.detections[0].left_foot is not None
        assert result.detections[0].right_foot is not None

    def test_low_visibility_filtered_out(self) -> None:
        service = _make_service()
        packet = _make_frame_packet()

        def make_landmark(x, y, vis=0.01, pres=0.01):
            lm = MagicMock()
            lm.x = x
            lm.y = y
            lm.visibility = vis
            lm.presence = pres
            return lm

        # All foot landmarks below threshold
        landmarks = [make_landmark(0.5, 0.5, 0.01, 0.01)] * 33

        mock_result = MagicMock()
        mock_result.pose_landmarks = [landmarks]

        mock_landmarker = MagicMock()
        mock_landmarker.detect_for_video.return_value = mock_result

        service._landmarker = mock_landmarker
        service._initialization_failed = False

        result = service.process_frame(packet)
        assert len(result.detections) == 0
        assert result.raw_pose_count == 1
