from __future__ import annotations

import cv2
import numpy as np
from PyQt6.QtCore import QPointF, Qt
from PyQt6.QtGui import QColor, QFont, QImage, QPainter, QPen, QPolygonF

from src.models.calibration_model import CalibrationModel
from src.models.contracts import FramePacket
from src.models.contracts import RenderState
from src.models.enums import CellState
from src.utils.image_conversion import numpy_to_qimage


class OverlayRenderService:
    def render(
        self,
        width: int,
        height: int,
        render_state: RenderState,
        frame_packet: FramePacket | None = None,
        calibration: CalibrationModel | None = None,
    ) -> QImage:
        image = self._prepare_base_image(width, height, frame_packet)

        painter = QPainter(image)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)
        self._draw_grid_overlay(painter, render_state, calibration)
        self._draw_player_markers(painter, render_state, calibration)
        self._draw_hud(painter, render_state, image.width(), image.height())

        painter.end()
        return image

    def _prepare_base_image(self, width: int, height: int, frame_packet: FramePacket | None) -> QImage:
        if frame_packet is None or frame_packet.frame is None:
            image = QImage(width, height, QImage.Format.Format_RGB32)
            image.fill(QColor("black"))
            return image

        frame = frame_packet.frame
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        return numpy_to_qimage(rgb_frame)

    def _draw_grid_overlay(
        self,
        painter: QPainter,
        render_state: RenderState,
        calibration: CalibrationModel | None,
    ) -> None:
        if calibration is None or calibration.homography is None or calibration.playable_bounds is None:
            return

        inverse_homography = np.linalg.inv(calibration.homography)
        rows = max((cell[0] for cell in render_state.grid_cells), default=-1) + 1
        columns = max((cell[1] for cell in render_state.grid_cells), default=-1) + 1
        if rows <= 0 or columns <= 0:
            return

        min_x = min(vertex[0] for vertex in calibration.playable_bounds)
        max_x = max(vertex[0] for vertex in calibration.playable_bounds)
        min_y = min(vertex[1] for vertex in calibration.playable_bounds)
        max_y = max(vertex[1] for vertex in calibration.playable_bounds)
        cell_width = (max_x - min_x) / columns
        cell_height = (max_y - min_y) / rows

        painter.setPen(QPen(QColor(255, 255, 255, 180), 2))
        for (row, column), state in render_state.grid_cells.items():
            polygon = self._project_floor_cell(
                inverse_homography,
                min_x + column * cell_width,
                min_y + row * cell_height,
                cell_width,
                cell_height,
            )
            fill_color = self._cell_fill_color(state)
            painter.setBrush(fill_color)
            painter.drawPolygon(polygon)

    def _draw_player_markers(
        self,
        painter: QPainter,
        render_state: RenderState,
        calibration: CalibrationModel | None,
    ) -> None:
        if calibration is None or calibration.homography is None:
            return

        inverse_homography = np.linalg.inv(calibration.homography)
        for player in render_state.players:
            self._draw_floor_point(painter, inverse_homography, player.left_foot, QColor("#ffd166"), 8)
            self._draw_floor_point(painter, inverse_homography, player.right_foot, QColor("#ef476f"), 8)
            self._draw_floor_point(painter, inverse_homography, player.standing_point, QColor("#00d1ff"), 11)

    def _draw_floor_point(
        self,
        painter: QPainter,
        inverse_homography: np.ndarray,
        point: tuple[float, float] | None,
        color: QColor,
        radius: int,
    ) -> None:
        if point is None:
            return

        point_array = np.array([[[point[0], point[1]]]], dtype=np.float32)
        projected = cv2.perspectiveTransform(point_array, inverse_homography)[0][0]
        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(color)
        painter.drawEllipse(QPointF(float(projected[0]), float(projected[1])), radius, radius)

    def _draw_hud(self, painter: QPainter, render_state: RenderState, width: int, height: int) -> None:
        painter.setPen(QColor("white"))
        painter.setFont(QFont("Sans Serif", 14))
        painter.drawText(24, 36, f"Status: {render_state.status_text}")
        painter.drawText(24, 64, f"Phase: {render_state.phase.name}")
        painter.drawText(24, 92, f"Timer: {render_state.timer_text}")
        painter.drawText(24, 120, f"Camera: {render_state.camera_status_text}")
        painter.drawText(24, 148, f"Pose: {render_state.pose_status_text}")
        painter.drawText(24, 176, f"Display: {render_state.display_status_text}")

        painter.setPen(QColor("#ffd166"))
        painter.drawText(24, 204, f"Calibration: {render_state.calibration_status_text}")
        painter.setPen(QColor("white"))

        for index, player in enumerate(render_state.players, start=1):
            painter.drawText(24, height - 24 - (index - 1) * 20, f"{player.player_id}: {player.status_text}")

    def _project_floor_cell(
        self,
        inverse_homography: np.ndarray,
        x: float,
        y: float,
        width: float,
        height: float,
    ) -> QPolygonF:
        floor_points = np.array(
            [[[x, y]], [[x + width, y]], [[x + width, y + height]], [[x, y + height]]],
            dtype=np.float32,
        )
        image_points = cv2.perspectiveTransform(floor_points, inverse_homography).reshape(-1, 2)
        return QPolygonF([QPointF(float(point[0]), float(point[1])) for point in image_points])

    def _cell_fill_color(self, state: CellState) -> QColor:
        if state is CellState.GREEN:
            return QColor(46, 204, 113, 110)
        if state is CellState.RED:
            return QColor(231, 76, 60, 110)
        if state is CellState.FLASHING:
            return QColor(241, 196, 15, 90)
        return QColor(127, 140, 141, 70)
