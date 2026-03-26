from __future__ import annotations

from dataclasses import replace
from typing import Iterable
import logging

import cv2
import numpy as np

from src.models.calibration_model import CalibrationModel
from src.models.enums import CalibrationState
from src.utils.config import AppConfig


class CalibrationService:
    SUPPORTED_DICTIONARIES: dict[str, int] = {
        "DICT_4X4_50": cv2.aruco.DICT_4X4_50,
        "DICT_4X4_100": cv2.aruco.DICT_4X4_100,
        "DICT_4X4_250": cv2.aruco.DICT_4X4_250,
        "DICT_4X4_1000": cv2.aruco.DICT_4X4_1000,
        "DICT_5X5_50": cv2.aruco.DICT_5X5_50,
        "DICT_5X5_100": cv2.aruco.DICT_5X5_100,
        "DICT_5X5_250": cv2.aruco.DICT_5X5_250,
        "DICT_5X5_1000": cv2.aruco.DICT_5X5_1000,
        "DICT_6X6_50": cv2.aruco.DICT_6X6_50,
        "DICT_6X6_100": cv2.aruco.DICT_6X6_100,
        "DICT_6X6_250": cv2.aruco.DICT_6X6_250,
        "DICT_6X6_1000": cv2.aruco.DICT_6X6_1000,
        "DICT_7X7_50": cv2.aruco.DICT_7X7_50,
        "DICT_7X7_100": cv2.aruco.DICT_7X7_100,
        "DICT_7X7_250": cv2.aruco.DICT_7X7_250,
        "DICT_7X7_1000": cv2.aruco.DICT_7X7_1000,
    }

    def __init__(self, config: AppConfig) -> None:
        self._config = config
        self._detector_parameters = cv2.aruco.DetectorParameters()
        self._logger = logging.getLogger(__name__)

    @classmethod
    def supported_dictionary_names(cls) -> list[str]:
        return list(cls.SUPPORTED_DICTIONARIES.keys())

    def _dictionary_name(self) -> str:
        dictionary_name = self._config.aruco_dictionary
        if dictionary_name not in self.SUPPORTED_DICTIONARIES:
            return "DICT_6X6_1000"
        return dictionary_name

    def _aruco_dictionary(self):
        dictionary_id = self.SUPPORTED_DICTIONARIES[self._dictionary_name()]
        return cv2.aruco.getPredefinedDictionary(dictionary_id)

    def calibrate_from_frame(
        self,
        model: CalibrationModel,
        frame: np.ndarray | None,
        *,
        is_live_source: bool = True,
        source_error: str | None = None,
    ) -> CalibrationModel:
        if frame is None:
            self._logger.warning("Calibration requested without a camera frame")
            return replace(
                model,
                state=CalibrationState.INVALID,
                detected_marker_corners={},
                homography=None,
                outer_bounds=None,
                playable_bounds=None,
                validation_message="No camera frame available for calibration.",
            )

        if not is_live_source:
            detail = source_error or "Live camera feed unavailable."
            self._logger.warning("Calibration rejected because live camera is unavailable: %s", detail)
            return replace(
                model,
                state=CalibrationState.INVALID,
                detected_marker_corners={},
                homography=None,
                outer_bounds=None,
                playable_bounds=None,
                validation_message=(
                    "Calibration requires a live camera frame. "
                    f"Current source is fallback mode: {detail}"
                ),
            )

        detected_markers = self.detect_markers(frame)
        self._logger.info("Calibration marker detection found %s marker(s)", len(detected_markers))
        return self.calibrate_from_detected_markers(model, detected_markers)

    def reset(self, model: CalibrationModel) -> CalibrationModel:
        return replace(
            model,
            state=CalibrationState.NOT_CALIBRATED,
            detected_marker_corners={},
            homography=None,
            outer_bounds=None,
            playable_bounds=None,
            validation_message="Calibration reset. Ready to calibrate.",
        )

    def detect_markers(self, frame: np.ndarray) -> dict[int, np.ndarray]:
        grayscale = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        detector = cv2.aruco.ArucoDetector(self._aruco_dictionary(), self._detector_parameters)
        corners, ids, _ = detector.detectMarkers(grayscale)
        if ids is None:
            return {}

        detected: dict[int, np.ndarray] = {}
        for marker_id, marker_corners in zip(ids.flatten().tolist(), corners, strict=False):
            detected[int(marker_id)] = np.asarray(marker_corners[0], dtype=np.float32)
        return detected

    def calibrate_from_detected_markers(
        self,
        model: CalibrationModel,
        detected_markers: dict[int, np.ndarray],
    ) -> CalibrationModel:
        playable_width = self._config.grid.playable_width
        playable_height = self._config.grid.playable_height
        inset = self._config.grid.playable_inset

        outer = ((0.0, 0.0), (playable_width, 0.0), (playable_width, playable_height), (0.0, playable_height))
        inner = (
            (inset, inset),
            (playable_width - inset, inset),
            (playable_width - inset, playable_height - inset),
            (inset, playable_height - inset),
        )

        required_ids = tuple(self._config.marker_ids)
        missing_ids = [marker_id for marker_id in required_ids if marker_id not in detected_markers]
        if missing_ids:
            self._logger.warning("Calibration missing marker ids: %s", missing_ids)
            return replace(
                model,
                state=CalibrationState.INVALID,
                detected_marker_corners=detected_markers,
                homography=None,
                outer_bounds=None,
                playable_bounds=None,
                validation_message=(
                    f"Calibration requires all 4 markers visible using {self._dictionary_name()}. Missing marker IDs: "
                    + ", ".join(str(marker_id) for marker_id in missing_ids)
                ),
            )

        if not self._is_rectangle_valid(outer) or not self._is_rectangle_valid(inner):
            self._logger.error("Calibration rejected because configured playable rectangle is invalid")
            return replace(
                model,
                state=CalibrationState.INVALID,
                detected_marker_corners=detected_markers,
                homography=None,
                outer_bounds=None,
                playable_bounds=None,
                validation_message="Configured playable rectangle is invalid.",
            )

        image_points = np.array(
            [self._marker_center(detected_markers[marker_id]) for marker_id in required_ids],
            dtype=np.float32,
        )
        world_points = np.array(outer, dtype=np.float32)

        if not self._is_quadrilateral_convex(image_points):
            self._logger.warning("Calibration rejected because detected markers are not convex")
            return replace(
                model,
                state=CalibrationState.INVALID,
                detected_marker_corners=detected_markers,
                homography=None,
                outer_bounds=None,
                playable_bounds=None,
                validation_message="Detected marker arrangement is not a valid convex quadrilateral.",
            )

        homography, mask = cv2.findHomography(image_points, world_points, method=0)
        if homography is None or mask is None:
            self._logger.error("Calibration failed because homography computation returned no result")
            return replace(
                model,
                state=CalibrationState.INVALID,
                detected_marker_corners=detected_markers,
                homography=None,
                outer_bounds=None,
                playable_bounds=None,
                validation_message="Unable to compute homography from detected markers.",
            )

        reprojection_error = self._mean_reprojection_error(homography, image_points, world_points)
        if not np.isfinite(reprojection_error) or reprojection_error > 0.15:
            self._logger.warning("Calibration rejected due to reprojection error %.3f", reprojection_error)
            return replace(
                model,
                state=CalibrationState.INVALID,
                detected_marker_corners=detected_markers,
                homography=None,
                outer_bounds=None,
                playable_bounds=None,
                validation_message=f"Calibration rejected due to reprojection error ({reprojection_error:.3f}).",
            )

        self._logger.info("Calibration succeeded with dictionary %s", self._dictionary_name())
        return replace(
            model,
            state=CalibrationState.VALID,
            detected_marker_corners=detected_markers,
            homography=homography,
            outer_bounds=outer,
            playable_bounds=inner,
            validation_message=f"Calibration locked using {self._dictionary_name()} and all 4 ArUco markers.",
        )

    @staticmethod
    def _marker_center(corners: np.ndarray) -> tuple[float, float]:
        center = corners.mean(axis=0)
        return float(center[0]), float(center[1])

    @staticmethod
    def _polygon_area(points: Iterable[tuple[float, float]]) -> float:
        vertices = list(points)
        area = 0.0
        for index, point in enumerate(vertices):
            next_point = vertices[(index + 1) % len(vertices)]
            area += point[0] * next_point[1] - next_point[0] * point[1]
        return abs(area) / 2.0

    def _is_rectangle_valid(self, vertices: tuple[tuple[float, float], ...]) -> bool:
        if len(vertices) != 4:
            return False
        area = self._polygon_area(vertices)
        return area > 0.0

    @staticmethod
    def _is_quadrilateral_convex(points: np.ndarray) -> bool:
        signs: list[float] = []
        for index in range(4):
            p0 = points[index]
            p1 = points[(index + 1) % 4]
            p2 = points[(index + 2) % 4]
            edge_a = p1 - p0
            edge_b = p2 - p1
            cross_z = edge_a[0] * edge_b[1] - edge_a[1] * edge_b[0]
            signs.append(float(cross_z))
        return all(value > 0 for value in signs) or all(value < 0 for value in signs)

    @staticmethod
    def _mean_reprojection_error(homography: np.ndarray, image_points: np.ndarray, world_points: np.ndarray) -> float:
        transformed = cv2.perspectiveTransform(image_points.reshape(-1, 1, 2), homography).reshape(-1, 2)
        error = np.linalg.norm(transformed - world_points, axis=1)
        return float(error.mean())
