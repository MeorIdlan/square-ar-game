from __future__ import annotations

from PyQt6.QtCore import QObject, pyqtSignal
from PyQt6.QtGui import QImage

from src.models.contracts import RenderState
from src.services.protocols import DebugRenderServiceProtocol


class DebugViewModel(QObject):
    image_updated = pyqtSignal(QImage)
    render_requested = pyqtSignal(object)

    def __init__(self, render_service: DebugRenderServiceProtocol) -> None:
        super().__init__()
        self._render_service = render_service

    def update_render_state(self, render_state: RenderState) -> None:
        self.render_requested.emit(render_state)

    def forward_rendered_image(self, image: QImage) -> None:
        self.image_updated.emit(image)
