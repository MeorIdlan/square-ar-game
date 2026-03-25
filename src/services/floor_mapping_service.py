from __future__ import annotations

from math import isclose

import cv2
import numpy as np

from src.models.calibration_model import CalibrationModel
from src.models.contracts import CellIndex, MappedPlayerState, Point, PoseFootState
from src.models.grid_model import GridModel


class FloorMappingService:
    def __init__(self, boundary_tolerance: float = 1e-6) -> None:
        self._boundary_tolerance = boundary_tolerance

    def resolve_feet_midpoint(self, left_foot: Point | None, right_foot: Point | None) -> Point | None:
        if left_foot and right_foot:
            return (
                (left_foot[0] + right_foot[0]) / 2.0,
                (left_foot[1] + right_foot[1]) / 2.0,
            )
        return left_foot or right_foot

    def map_detection(
        self,
        player_id: str,
        pose_state: PoseFootState,
        grid: GridModel,
        calibration: CalibrationModel,
    ) -> MappedPlayerState:
        left_foot = self.map_image_point_to_floor(pose_state.left_foot, calibration)
        right_foot = self.map_image_point_to_floor(pose_state.right_foot, calibration)
        standing_point = self.resolve_feet_midpoint(left_foot, right_foot)
        occupied_cell = self.resolve_cell(standing_point, grid, calibration)
        in_bounds = standing_point is not None and self.is_inside_playable_area(standing_point, calibration)
        ambiguous = in_bounds and occupied_cell is None
        return MappedPlayerState(
            player_id=player_id,
            standing_point=standing_point,
            left_foot=left_foot,
            right_foot=right_foot,
            occupied_cell=occupied_cell,
            in_bounds=in_bounds,
            ambiguous=ambiguous,
            confidence=pose_state.confidence,
        )

    def map_image_point_to_floor(self, point: Point | None, calibration: CalibrationModel) -> Point | None:
        if point is None or calibration.homography is None:
            return None

        point_array = np.array([[point]], dtype=np.float32)
        mapped = cv2.perspectiveTransform(point_array, calibration.homography)[0][0]
        return float(mapped[0]), float(mapped[1])

    def is_inside_playable_area(self, point: Point, calibration: CalibrationModel) -> bool:
        if calibration.playable_bounds is None:
            return False

        min_x = min(vertex[0] for vertex in calibration.playable_bounds)
        max_x = max(vertex[0] for vertex in calibration.playable_bounds)
        min_y = min(vertex[1] for vertex in calibration.playable_bounds)
        max_y = max(vertex[1] for vertex in calibration.playable_bounds)
        return min_x <= point[0] <= max_x and min_y <= point[1] <= max_y

    def resolve_cell(
        self,
        point: Point | None,
        grid: GridModel,
        calibration: CalibrationModel,
    ) -> CellIndex | None:
        if point is None or not self.is_inside_playable_area(point, calibration):
            return None

        if calibration.playable_bounds is None:
            return None

        min_x = min(vertex[0] for vertex in calibration.playable_bounds)
        max_x = max(vertex[0] for vertex in calibration.playable_bounds)
        min_y = min(vertex[1] for vertex in calibration.playable_bounds)
        max_y = max(vertex[1] for vertex in calibration.playable_bounds)
        playable_width = max_x - min_x
        playable_height = max_y - min_y
        cell_width = playable_width / grid.columns
        cell_height = playable_height / grid.rows

        x, y = point
        relative_x = x - min_x
        relative_y = y - min_y
        if isclose(relative_x % cell_width, 0.0, abs_tol=self._boundary_tolerance) or isclose(
            relative_y % cell_height,
            0.0,
            abs_tol=self._boundary_tolerance,
        ):
            return None

        column = int(relative_x // cell_width)
        row = int(relative_y // cell_height)
        if row < 0 or row >= grid.rows or column < 0 or column >= grid.columns:
            return None
        return row, column
