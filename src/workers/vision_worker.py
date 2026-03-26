from __future__ import annotations

from PyQt6.QtCore import QObject, QTimer, pyqtSignal, pyqtSlot

from src.models.contracts import FramePacket, PoseResult
from src.services.pose_tracking_service import PoseTrackingService


class VisionWorker(QObject):
    pose_ready = pyqtSignal(object)

    def __init__(self, pose_tracking_service: PoseTrackingService) -> None:
        super().__init__()
        self._pose_tracking_service = pose_tracking_service
        self._latest_frame_packet: FramePacket | None = None
        self._processing = False

    @pyqtSlot(object)
    def process_frame(self, frame_packet: FramePacket) -> None:
        self._latest_frame_packet = frame_packet
        if self._processing:
            return
        self._processing = True
        QTimer.singleShot(0, self._process_latest_frame)

    @pyqtSlot()
    def _process_latest_frame(self) -> None:
        frame_packet = self._latest_frame_packet
        self._latest_frame_packet = None
        if frame_packet is None:
            self._processing = False
            return

        result: PoseResult = self._pose_tracking_service.process_frame(frame_packet)
        self.pose_ready.emit(result)

        if self._latest_frame_packet is not None:
            QTimer.singleShot(0, self._process_latest_frame)
            return

        self._processing = False
