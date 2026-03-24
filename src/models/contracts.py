from __future__ import annotations

from dataclasses import dataclass, field
from time import monotonic

import numpy as np

from src.models.enums import CellState, RoundPhase


Point = tuple[float, float]
CellIndex = tuple[int, int]


@dataclass(slots=True)
class FramePacket:
    frame_id: int
    timestamp: float = field(default_factory=monotonic)
    frame: np.ndarray | None = None


@dataclass(slots=True)
class PoseFootState:
    left_foot: Point | None = None
    right_foot: Point | None = None
    resolved_point: Point | None = None
    confidence: float = 0.0


@dataclass(slots=True)
class PoseResult:
    frame_id: int
    timestamp: float = field(default_factory=monotonic)
    detections: list[PoseFootState] = field(default_factory=list)


@dataclass(slots=True)
class MappedPlayerState:
    player_id: str
    standing_point: Point | None
    left_foot: Point | None = None
    right_foot: Point | None = None
    occupied_cell: CellIndex | None = None
    in_bounds: bool = False
    ambiguous: bool = False
    confidence: float = 0.0
    last_seen_at: float = field(default_factory=monotonic)


@dataclass(slots=True)
class RenderPlayerState:
    player_id: str
    standing_point: Point | None
    left_foot: Point | None
    right_foot: Point | None
    occupied_cell: CellIndex | None
    status_text: str


@dataclass(slots=True)
class RenderState:
    phase: RoundPhase = RoundPhase.IDLE
    timer_text: str = "00.0"
    status_text: str = "Idle"
    grid_cells: dict[CellIndex, CellState] = field(default_factory=dict)
    players: list[RenderPlayerState] = field(default_factory=list)
