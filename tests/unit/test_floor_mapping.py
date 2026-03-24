from __future__ import annotations

import unittest

from src.models.calibration_model import CalibrationModel
from src.models.contracts import PoseFootState
from src.models.enums import CalibrationState
from src.models.grid_model import GridModel
from src.services.floor_mapping_service import FloorMappingService


class FloorMappingServiceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.service = FloorMappingService(boundary_tolerance=1e-9)
        self.grid = GridModel(rows=4, columns=4, playable_width=4.0, playable_height=4.0)
        self.calibration = CalibrationModel(
            state=CalibrationState.VALID,
            homography=None,
            outer_bounds=((0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 4.0)),
            playable_bounds=((0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 4.0)),
        )

    def test_resolves_midpoint_from_two_feet(self) -> None:
        pose_state = PoseFootState(left_foot=(1.0, 1.0), right_foot=(2.0, 1.0), confidence=0.9)
        self.assertEqual(self.service.resolve_feet_midpoint(pose_state), (1.5, 1.0))

    def test_single_foot_fallback(self) -> None:
        pose_state = PoseFootState(left_foot=(1.25, 2.75), confidence=0.9)
        self.assertEqual(self.service.resolve_feet_midpoint(pose_state), (1.25, 2.75))

    def test_maps_point_to_cell_when_inside_bounds(self) -> None:
        pose_state = PoseFootState(left_foot=(1.2, 1.2), right_foot=(1.6, 1.2), confidence=0.8)
        mapped = self.service.map_detection("P1", pose_state, self.grid, self.calibration)
        self.assertTrue(mapped.in_bounds)
        self.assertEqual(mapped.occupied_cell, (1, 1))
        self.assertFalse(mapped.ambiguous)

    def test_boundary_points_are_ambiguous(self) -> None:
        pose_state = PoseFootState(left_foot=(1.0, 1.0), right_foot=(1.0, 2.0), confidence=0.8)
        mapped = self.service.map_detection("P1", pose_state, self.grid, self.calibration)
        self.assertTrue(mapped.in_bounds)
        self.assertIsNone(mapped.occupied_cell)
        self.assertTrue(mapped.ambiguous)

    def test_out_of_bounds_points_are_invalid(self) -> None:
        pose_state = PoseFootState(left_foot=(4.5, 1.0), confidence=0.8)
        mapped = self.service.map_detection("P1", pose_state, self.grid, self.calibration)
        self.assertFalse(mapped.in_bounds)
        self.assertIsNone(mapped.occupied_cell)


if __name__ == "__main__":
    unittest.main()
