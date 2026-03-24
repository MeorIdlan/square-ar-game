from __future__ import annotations

from PyQt6.QtCore import QObject, pyqtSignal

from src.models.contracts import RenderState
from src.services.debug_render_service import DebugRenderService


class DebugViewModel(QObject):
    image_updated = pyqtSignal(object)

    def __init__(self, render_service: DebugRenderService) -> None:
        super().__init__()
        self._render_service = render_service

    def update_render_state(self, render_state: RenderState) -> None:
        image = self._render_service.render(640, 640, render_state)
        self.image_updated.emit(image)
