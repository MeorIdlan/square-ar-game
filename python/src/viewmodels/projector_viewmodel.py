from __future__ import annotations

from PyQt6.QtCore import QObject, pyqtSignal
from PyQt6.QtGui import QImage

from src.models.calibration_model import CalibrationModel
from src.models.contracts import FramePacket
from src.models.contracts import RenderState
from src.services.protocols import OverlayRenderServiceProtocol


class ProjectorViewModel(QObject):
    image_updated = pyqtSignal(QImage)
    render_requested = pyqtSignal(object, object, object)

    def __init__(self, render_service: OverlayRenderServiceProtocol) -> None:
        super().__init__()
        self._render_service = render_service

    def update_render_state(
        self,
        render_state: RenderState,
        frame_packet: FramePacket | None = None,
        calibration: CalibrationModel | None = None,
    ) -> None:
        self.render_requested.emit(render_state, frame_packet, calibration)

    def forward_rendered_image(self, image: QImage) -> None:
        self.image_updated.emit(image)
