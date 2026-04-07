from __future__ import annotations

from src.models.contracts import MappedPlayerState
from src.services.detection_deduplicator import deduplicate_detections


class TestDeduplicateDetections:
    def test_two_far_apart_both_kept(self) -> None:
        detections = [
            MappedPlayerState(
                player_id="d1",
                standing_point=(0.0, 0.0),
                in_bounds=True,
                confidence=0.9,
            ),
            MappedPlayerState(
                player_id="d2",
                standing_point=(5.0, 5.0),
                in_bounds=True,
                confidence=0.8,
            ),
        ]
        result = deduplicate_detections(detections, distance_threshold=0.35)
        assert len(result) == 2

    def test_two_close_same_cell_one_removed(self) -> None:
        detections = [
            MappedPlayerState(
                player_id="d1",
                standing_point=(1.0, 1.0),
                occupied_cell=(0, 0),
                in_bounds=True,
                confidence=0.9,
            ),
            MappedPlayerState(
                player_id="d2",
                standing_point=(1.1, 1.1),
                occupied_cell=(0, 0),
                in_bounds=True,
                confidence=0.8,
            ),
        ]
        result = deduplicate_detections(detections, distance_threshold=0.35)
        assert len(result) == 1
        assert result[0].player_id == "d1"  # higher confidence kept

    def test_empty_list_returns_empty(self) -> None:
        result = deduplicate_detections([], distance_threshold=0.35)
        assert result == []

    def test_close_by_distance_different_cells(self) -> None:
        detections = [
            MappedPlayerState(
                player_id="d1",
                standing_point=(1.0, 1.0),
                occupied_cell=(0, 0),
                in_bounds=True,
                confidence=0.5,
            ),
            MappedPlayerState(
                player_id="d2",
                standing_point=(1.2, 1.0),
                occupied_cell=(0, 1),
                in_bounds=True,
                confidence=0.6,
            ),
        ]
        result = deduplicate_detections(detections, distance_threshold=0.35)
        assert len(result) == 1

    def test_none_standing_point_kept(self) -> None:
        detections = [
            MappedPlayerState(
                player_id="d1", standing_point=None, in_bounds=False, confidence=0.0
            ),
        ]
        result = deduplicate_detections(detections, distance_threshold=0.35)
        assert len(result) == 1
