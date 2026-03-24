from __future__ import annotations

from PyQt6.QtGui import QColor, QImage, QPainter, QPen

from src.models.contracts import RenderState
from src.models.enums import CellState


class DebugRenderService:
    def render(self, width: int, height: int, render_state: RenderState) -> QImage:
        image = QImage(width, height, QImage.Format.Format_RGB32)
        image.fill(QColor("#111111"))

        painter = QPainter(image)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)
        painter.setPen(QPen(QColor("white"), 1))

        margin = 30
        grid_width = width - margin * 2
        grid_height = height - margin * 2
        rows = max((cell[0] for cell in render_state.grid_cells), default=-1) + 1
        columns = max((cell[1] for cell in render_state.grid_cells), default=-1) + 1
        rows = max(rows, 1)
        columns = max(columns, 1)
        cell_width = grid_width / columns
        cell_height = grid_height / rows

        for (row, column), state in render_state.grid_cells.items():
            color = QColor("#404040")
            if state is CellState.GREEN:
                color = QColor("#2e8b57")
            elif state is CellState.RED:
                color = QColor("#a52a2a")
            elif state is CellState.FLASHING:
                color = QColor("#b8860b")

            painter.fillRect(
                int(margin + column * cell_width),
                int(margin + row * cell_height),
                int(cell_width - 2),
                int(cell_height - 2),
                color,
            )

        painter.setPen(QPen(QColor("#00bfff"), 6))
        for player in render_state.players:
            if player.standing_point is None:
                continue
            x = margin + player.standing_point[0] / max(columns, 1) * grid_width
            y = margin + player.standing_point[1] / max(rows, 1) * grid_height
            painter.drawPoint(int(x), int(y))

        painter.setPen(QColor("white"))
        painter.drawText(16, 20, render_state.status_text)
        painter.end()
        return image
