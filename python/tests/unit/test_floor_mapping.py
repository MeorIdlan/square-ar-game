from __future__ import annotations

import unittest

import numpy as np

from src.models.calibration_model import CalibrationModel
from src.models.contracts import PoseFootState
from src.models.enums import CalibrationState
from src.models.grid_model import GridModel
from src.services.floor_mapping_service import FloorMappingService
from src.utils.constants import BOUNDARY_TOLERANCE


class FloorMappingServiceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.service = FloorMappingService(boundary_tolerance=BOUNDARY_TOLERANCE)
        self.grid = GridModel(
            rows=4, columns=4, playable_width=4.0, playable_height=4.0
        )
        self.calibration = CalibrationModel(
            state=CalibrationState.VALID,
            homography=np.eye(3),
            outer_bounds=((0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 4.0)),
            playable_bounds=((0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 4.0)),
        )

    def test_resolves_midpoint_from_two_feet(self) -> None:
        self.assertEqual(
            self.service.resolve_feet_midpoint((1.0, 1.0), (2.0, 1.0)), (1.5, 1.0)
        )

    def test_single_foot_fallback(self) -> None:
        self.assertEqual(
            self.service.resolve_feet_midpoint((1.25, 2.75), None), (1.25, 2.75)
        )

    def test_maps_point_to_cell_when_inside_bounds(self) -> None:
        pose_state = PoseFootState(
            left_foot=(1.2, 1.2), right_foot=(1.6, 1.2), confidence=0.8
        )
        mapped = self.service.map_detection(
            "P1", pose_state, self.grid, self.calibration
        )
        self.assertTrue(mapped.in_bounds)
        self.assertEqual(mapped.occupied_cell, (1, 1))
        self.assertFalse(mapped.ambiguous)

    def test_boundary_points_are_ambiguous(self) -> None:
        pose_state = PoseFootState(
            left_foot=(1.0, 1.0), right_foot=(1.0, 2.0), confidence=0.8
        )
        mapped = self.service.map_detection(
            "P1", pose_state, self.grid, self.calibration
        )
        self.assertTrue(mapped.in_bounds)
        self.assertIsNone(mapped.occupied_cell)
        self.assertTrue(mapped.ambiguous)

    def test_out_of_bounds_points_are_invalid(self) -> None:
        pose_state = PoseFootState(left_foot=(4.5, 1.0), confidence=0.8)
        mapped = self.service.map_detection(
            "P1", pose_state, self.grid, self.calibration
        )
        self.assertFalse(mapped.in_bounds)
        self.assertIsNone(mapped.occupied_cell)

    def test_maps_points_using_homography_before_cell_resolution(self) -> None:
        self.calibration.homography = np.array(
            [[0.02, 0.0, 0.0], [0.0, 0.02, 0.0], [0.0, 0.0, 1.0]],
            dtype=np.float32,
        )
        pose_state = PoseFootState(
            left_foot=(60.0, 60.0), right_foot=(80.0, 60.0), confidence=0.8
        )
        mapped = self.service.map_detection(
            "P1", pose_state, self.grid, self.calibration
        )
        self.assertIsNotNone(mapped.standing_point)
        assert mapped.standing_point is not None
        self.assertAlmostEqual(mapped.standing_point[0], 1.4, places=4)
        self.assertAlmostEqual(mapped.standing_point[1], 1.2, places=4)
        self.assertEqual(mapped.occupied_cell, (1, 1))

    def test_negative_floor_coordinates_out_of_bounds(self) -> None:
        pose_state = PoseFootState(left_foot=(-1.0, -1.0), confidence=0.8)
        mapped = self.service.map_detection(
            "P1", pose_state, self.grid, self.calibration
        )
        self.assertFalse(mapped.in_bounds)

    def test_identity_homography_passthrough(self) -> None:
        self.calibration.homography = np.eye(3, dtype=np.float32)
        point = self.service.map_image_point_to_floor((2.5, 1.5), self.calibration)
        self.assertIsNotNone(point)
        assert point is not None
        self.assertAlmostEqual(point[0], 2.5, places=4)
        self.assertAlmostEqual(point[1], 1.5, places=4)

    def test_resolve_feet_midpoint_both_none(self) -> None:
        result = self.service.resolve_feet_midpoint(None, None)
        self.assertIsNone(result)


if __name__ == "__main__":
    unittest.main()
