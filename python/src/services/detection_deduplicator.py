from __future__ import annotations

from math import dist

from src.models.contracts import MappedPlayerState


def deduplicate_detections(
    detections: list[MappedPlayerState],
    distance_threshold: float,
) -> list[MappedPlayerState]:
    unique: list[MappedPlayerState] = []
    for detection in sorted(detections, key=lambda item: item.confidence, reverse=True):
        if detection.standing_point is None:
            unique.append(detection)
            continue

        duplicate_found = False
        for existing in unique:
            if existing.standing_point is None:
                continue
            if (
                detection.occupied_cell is not None
                and detection.occupied_cell == existing.occupied_cell
            ):
                duplicate_found = True
                break
            if (
                dist(detection.standing_point, existing.standing_point)
                <= distance_threshold
            ):
                duplicate_found = True
                break

        if not duplicate_found:
            unique.append(detection)

    return unique
