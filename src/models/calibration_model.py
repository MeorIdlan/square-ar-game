from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np

from src.models.enums import CalibrationState


Point = tuple[float, float]


@dataclass(slots=True)
class CalibrationMarkerLayout:
    marker_ids: tuple[int, int, int, int] = (0, 1, 2, 3)


@dataclass(slots=True)
class CalibrationModel:
    state: CalibrationState = CalibrationState.NOT_CALIBRATED
    marker_layout: CalibrationMarkerLayout = field(
        default_factory=CalibrationMarkerLayout
    )
    detected_marker_corners: dict[int, np.ndarray] = field(default_factory=dict)
    homography: np.ndarray | None = None
    inverse_homography: np.ndarray | None = None
    outer_bounds: tuple[Point, Point, Point, Point] | None = None
    playable_bounds: tuple[Point, Point, Point, Point] | None = None
    validation_message: str = "Not calibrated"

    @property
    def is_valid(self) -> bool:
        return self.state is CalibrationState.VALID and self.homography is not None

    @property
    def detected_marker_ids(self) -> list[int]:
        return sorted(self.detected_marker_corners.keys())

    @property
    def detected_marker_count(self) -> int:
        return len(self.detected_marker_corners)

    @property
    def missing_marker_ids(self) -> list[int]:
        return [
            marker_id
            for marker_id in self.marker_layout.marker_ids
            if marker_id not in self.detected_marker_corners
        ]
