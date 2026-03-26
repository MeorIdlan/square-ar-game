from __future__ import annotations

from PyQt6.QtCore import QObject, QTimer, pyqtSignal

from src.models.contracts import FramePacket
from src.services.camera_service import CameraService


class CameraWorker(QObject):
    frame_ready = pyqtSignal(object)

    def __init__(self, camera_service: CameraService) -> None:
        super().__init__()
        self._camera_service = camera_service
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._emit_frame)

    def start(self, interval_ms: int) -> None:
        self._timer.start(interval_ms)

    def set_interval(self, interval_ms: int) -> None:
        was_active = self._timer.isActive()
        self._timer.setInterval(interval_ms)
        if was_active and not self._timer.isActive():
            self._timer.start(interval_ms)

    def stop(self) -> None:
        self._timer.stop()

    def _emit_frame(self) -> None:
        frame_packet: FramePacket = self._camera_service.next_frame()
        self.frame_ready.emit(frame_packet)
