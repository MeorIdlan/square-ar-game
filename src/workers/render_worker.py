from __future__ import annotations

from PyQt6.QtCore import QObject, QTimer, pyqtSignal, pyqtSlot

from src.models.calibration_model import CalibrationModel
from src.models.contracts import FramePacket, RenderState
from src.services.debug_render_service import DebugRenderService
from src.services.overlay_render_service import OverlayRenderService


class ProjectorRenderWorker(QObject):
    image_ready = pyqtSignal(object)

    def __init__(self, render_service: OverlayRenderService) -> None:
        super().__init__()
        self._render_service = render_service
        self._latest_request: tuple[RenderState, FramePacket | None, CalibrationModel | None] | None = None
        self._processing = False

    @pyqtSlot(object, object, object)
    def render_latest(
        self,
        render_state: RenderState,
        frame_packet: FramePacket | None,
        calibration: CalibrationModel | None,
    ) -> None:
        self._latest_request = (render_state, frame_packet, calibration)
        if self._processing:
            return
        self._processing = True
        QTimer.singleShot(0, self._process_latest)

    @pyqtSlot()
    def _process_latest(self) -> None:
        request = self._latest_request
        self._latest_request = None
        if request is None:
            self._processing = False
            return

        render_state, frame_packet, calibration = request
        image = self._render_service.render(1280, 720, render_state, frame_packet=frame_packet, calibration=calibration)
        self.image_ready.emit(image)

        if self._latest_request is not None:
            QTimer.singleShot(0, self._process_latest)
            return

        self._processing = False


class DebugRenderWorker(QObject):
    image_ready = pyqtSignal(object)

    def __init__(self, render_service: DebugRenderService) -> None:
        super().__init__()
        self._render_service = render_service
        self._latest_state: RenderState | None = None
        self._processing = False

    @pyqtSlot(object)
    def render_latest(self, render_state: RenderState) -> None:
        self._latest_state = render_state
        if self._processing:
            return
        self._processing = True
        QTimer.singleShot(0, self._process_latest)

    @pyqtSlot()
    def _process_latest(self) -> None:
        render_state = self._latest_state
        self._latest_state = None
        if render_state is None:
            self._processing = False
            return

        image = self._render_service.render(640, 640, render_state)
        self.image_ready.emit(image)

        if self._latest_state is not None:
            QTimer.singleShot(0, self._process_latest)
            return

        self._processing = False
