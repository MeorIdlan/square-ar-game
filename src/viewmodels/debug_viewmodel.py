from __future__ import annotations

from PyQt6.QtCore import QObject, pyqtSignal

from src.models.contracts import RenderState
from src.services.debug_render_service import DebugRenderService


class DebugViewModel(QObject):
    image_updated = pyqtSignal(object)
    render_requested = pyqtSignal(object)

    def __init__(self, render_service: DebugRenderService) -> None:
        super().__init__()
        self._render_service = render_service

    def update_render_state(self, render_state: RenderState) -> None:
        self.render_requested.emit(render_state)

    def forward_rendered_image(self, image: object) -> None:
        self.image_updated.emit(image)
