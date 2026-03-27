from __future__ import annotations

import unittest

import numpy as np

from src.models.calibration_model import CalibrationModel
from src.models.enums import CalibrationState
from src.services.calibration_service import CalibrationService
from src.utils.config import AppConfig


class CalibrationServiceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.config = AppConfig()
        self.service = CalibrationService(self.config)
        self.model = CalibrationModel()

    def test_calibration_requires_all_four_markers(self) -> None:
        detected = {
            0: np.array([[0.0, 0.0], [10.0, 0.0], [10.0, 10.0], [0.0, 10.0]], dtype=np.float32),
            1: np.array([[90.0, 0.0], [100.0, 0.0], [100.0, 10.0], [90.0, 10.0]], dtype=np.float32),
            2: np.array([[90.0, 90.0], [100.0, 90.0], [100.0, 100.0], [90.0, 100.0]], dtype=np.float32),
        }

        result = self.service.calibrate_from_detected_markers(self.model, detected)

        self.assertEqual(result.state, CalibrationState.INVALID)
        self.assertIn("Missing marker IDs", result.validation_message)

    def test_valid_marker_layout_computes_homography(self) -> None:
        detected = {
            0: np.array([[0.0, 0.0], [20.0, 0.0], [20.0, 20.0], [0.0, 20.0]], dtype=np.float32),
            1: np.array([[180.0, 0.0], [200.0, 0.0], [200.0, 20.0], [180.0, 20.0]], dtype=np.float32),
            2: np.array([[180.0, 180.0], [200.0, 180.0], [200.0, 200.0], [180.0, 200.0]], dtype=np.float32),
            3: np.array([[0.0, 180.0], [20.0, 180.0], [20.0, 200.0], [0.0, 200.0]], dtype=np.float32),
        }

        result = self.service.calibrate_from_detected_markers(self.model, detected)

        self.assertEqual(result.state, CalibrationState.VALID)
        self.assertIsNotNone(result.homography)
        self.assertEqual(result.outer_bounds, ((0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 4.0)))
        self.assertEqual(result.playable_bounds, ((0.2, 0.2), (3.8, 0.2), (3.8, 3.8), (0.2, 3.8)))

    def test_valid_marker_layout_computes_homography_when_ids_are_not_in_corner_order(self) -> None:
        detected = {
            0: np.array([[180.0, 180.0], [200.0, 180.0], [200.0, 200.0], [180.0, 200.0]], dtype=np.float32),
            1: np.array([[0.0, 180.0], [20.0, 180.0], [20.0, 200.0], [0.0, 200.0]], dtype=np.float32),
            2: np.array([[0.0, 0.0], [20.0, 0.0], [20.0, 20.0], [0.0, 20.0]], dtype=np.float32),
            3: np.array([[180.0, 0.0], [200.0, 0.0], [200.0, 20.0], [180.0, 20.0]], dtype=np.float32),
        }

        result = self.service.calibrate_from_detected_markers(self.model, detected)

        self.assertEqual(result.state, CalibrationState.VALID)
        self.assertIsNotNone(result.homography)

    def test_calibration_rejects_fallback_frame_sources(self) -> None:
        frame = np.zeros((64, 64, 3), dtype=np.uint8)

        result = self.service.calibrate_from_frame(
            self.model,
            frame,
            is_live_source=False,
            source_error="Unable to open camera index 0.",
        )

        self.assertEqual(result.state, CalibrationState.INVALID)
        self.assertIn("requires a live camera frame", result.validation_message)
        self.assertIn("Unable to open camera index 0.", result.validation_message)


if __name__ == "__main__":
    unittest.main()