from __future__ import annotations

from dataclasses import dataclass, field
from time import monotonic

from src.models.enums import PlayerTrackingState


Point = tuple[float, float]
CellIndex = tuple[int, int]


@dataclass(slots=True)
class PlayerModel:
    player_id: str
    tracking_state: PlayerTrackingState = PlayerTrackingState.UNKNOWN
    standing_point: Point | None = None
    left_foot: Point | None = None
    right_foot: Point | None = None
    occupied_cell: CellIndex | None = None
    confidence: float = 0.0
    last_seen_at: float = field(default_factory=monotonic)
    active_in_round: bool = False
    eliminated_in_round: bool = False
