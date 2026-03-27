from __future__ import annotations

from PyQt6.QtCore import QObject, QTimer, Qt, pyqtSignal, pyqtSlot

from src.models.contracts import FramePacket
from src.services.camera_service import CameraService


class CameraWorker(QObject):
    frame_ready = pyqtSignal(object)

    def __init__(self, camera_service: CameraService, initial_interval_ms: int) -> None:
        super().__init__()
        self._camera_service = camera_service
        self._interval_ms = max(1, initial_interval_ms)
        self._timer = QTimer(self)
        self._timer.setTimerType(Qt.TimerType.PreciseTimer)
        self._timer.setInterval(self._interval_ms)
        self._timer.timeout.connect(self._emit_frame)

    @pyqtSlot()
    def start(self) -> None:
        self._timer.start(self._interval_ms)

    @pyqtSlot(int)
    def set_interval(self, interval_ms: int) -> None:
        self._interval_ms = max(1, interval_ms)
        was_active = self._timer.isActive()
        if was_active:
            self._timer.stop()
        self._timer.setInterval(self._interval_ms)
        if was_active:
            self._timer.start(self._interval_ms)

    @pyqtSlot()
    def stop(self) -> None:
        self._timer.stop()

    def _emit_frame(self) -> None:
        frame_packet: FramePacket = self._camera_service.next_frame()
        self.frame_ready.emit(frame_packet)
