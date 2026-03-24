from __future__ import annotations

from PyQt6.QtGui import QColor, QImage, QPainter, QPen

from src.models.contracts import RenderState


class OverlayRenderService:
    def render(self, width: int, height: int, render_state: RenderState) -> QImage:
        image = QImage(width, height, QImage.Format.Format_RGB32)
        image.fill(QColor("black"))

        painter = QPainter(image)
        painter.setPen(QColor("white"))
        painter.drawText(24, 36, f"Status: {render_state.status_text}")
        painter.drawText(24, 64, f"Phase: {render_state.phase.name}")
        painter.drawText(24, 92, f"Timer: {render_state.timer_text}")

        painter.setPen(QPen(QColor("white"), 2))
        grid_origin_x = 40
        grid_origin_y = 120
        cell_size = 72
        for (row, column), state in render_state.grid_cells.items():
            color_name = {
                "GREEN": "green",
                "RED": "red",
                "FLASHING": "yellow",
            }.get(state.name, "gray")
            painter.fillRect(grid_origin_x + column * cell_size, grid_origin_y + row * cell_size, cell_size - 4, cell_size - 4, QColor(color_name))

        for index, player in enumerate(render_state.players, start=1):
            painter.drawText(24, height - 24 - (index - 1) * 20, f"{player.player_id}: {player.status_text}")

        painter.end()
        return image
