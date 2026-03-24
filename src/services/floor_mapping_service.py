from __future__ import annotations

from math import isclose

from src.models.calibration_model import CalibrationModel
from src.models.contracts import CellIndex, MappedPlayerState, Point, PoseFootState
from src.models.grid_model import GridModel


class FloorMappingService:
    def __init__(self, boundary_tolerance: float = 1e-6) -> None:
        self._boundary_tolerance = boundary_tolerance

    def resolve_feet_midpoint(self, pose_state: PoseFootState) -> Point | None:
        if pose_state.left_foot and pose_state.right_foot:
            return (
                (pose_state.left_foot[0] + pose_state.right_foot[0]) / 2.0,
                (pose_state.left_foot[1] + pose_state.right_foot[1]) / 2.0,
            )
        return pose_state.left_foot or pose_state.right_foot

    def map_detection(
        self,
        player_id: str,
        pose_state: PoseFootState,
        grid: GridModel,
        calibration: CalibrationModel,
    ) -> MappedPlayerState:
        standing_point = self.resolve_feet_midpoint(pose_state)
        occupied_cell = self.resolve_cell(standing_point, grid, calibration)
        in_bounds = standing_point is not None and self.is_inside_playable_area(standing_point, calibration)
        ambiguous = in_bounds and occupied_cell is None
        return MappedPlayerState(
            player_id=player_id,
            standing_point=standing_point,
            left_foot=pose_state.left_foot,
            right_foot=pose_state.right_foot,
            occupied_cell=occupied_cell,
            in_bounds=in_bounds,
            ambiguous=ambiguous,
            confidence=pose_state.confidence,
        )

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

        x, y = point
        if isclose(x % grid.cell_width, 0.0, abs_tol=self._boundary_tolerance) or isclose(
            y % grid.cell_height,
            0.0,
            abs_tol=self._boundary_tolerance,
        ):
            return None

        column = int(x // grid.cell_width)
        row = int(y // grid.cell_height)
        if row < 0 or row >= grid.rows or column < 0 or column >= grid.columns:
            return None
        return row, column
