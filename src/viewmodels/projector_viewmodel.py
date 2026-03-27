from __future__ import annotations

from PyQt6.QtCore import QObject, pyqtSignal

from src.models.calibration_model import CalibrationModel
from src.models.contracts import FramePacket
from src.models.contracts import RenderState
from src.services.overlay_render_service import OverlayRenderService


class ProjectorViewModel(QObject):
    image_updated = pyqtSignal(object)
    render_requested = pyqtSignal(object, object, object)

    def __init__(self, render_service: OverlayRenderService) -> None:
        super().__init__()
        self._render_service = render_service

    def update_render_state(
        self,
        render_state: RenderState,
        frame_packet: FramePacket | None = None,
        calibration: CalibrationModel | None = None,
    ) -> None:
        self.render_requested.emit(render_state, frame_packet, calibration)

    def forward_rendered_image(self, image: object) -> None:
        self.image_updated.emit(image)
