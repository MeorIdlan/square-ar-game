from __future__ import annotations

from PyQt6.QtCore import QObject, pyqtSignal, pyqtSlot

from src.models.contracts import FramePacket, PoseResult
from src.services.pose_tracking_service import PoseTrackingService


class VisionWorker(QObject):
    pose_ready = pyqtSignal(object)

    def __init__(self, pose_tracking_service: PoseTrackingService) -> None:
        super().__init__()
        self._pose_tracking_service = pose_tracking_service

    @pyqtSlot(object)
    def process_frame(self, frame_packet: FramePacket) -> None:
        result: PoseResult = self._pose_tracking_service.process_frame(frame_packet)
        self.pose_ready.emit(result)
