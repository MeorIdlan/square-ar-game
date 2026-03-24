from __future__ import annotations

from dataclasses import dataclass, field

from src.models.enums import RoundPhase


@dataclass(slots=True)
class RoundTimingSettings:
    flashing_duration_seconds: float = 3.0
    flash_interval_seconds: float = 0.25
    reaction_window_seconds: float = 1.5
    lock_delay_seconds: float = 0.25
    elimination_display_seconds: float = 2.5
    inter_round_delay_seconds: float = 1.5
    missed_detection_grace_seconds: float = 0.35


@dataclass(slots=True)
class RoundModel:
    round_number: int = 0
    phase: RoundPhase = RoundPhase.IDLE
    timer_remaining: float = 0.0
    timings: RoundTimingSettings = field(default_factory=RoundTimingSettings)
    participant_ids: list[str] = field(default_factory=list)
    survivor_ids: list[str] = field(default_factory=list)
    eliminated_ids: list[str] = field(default_factory=list)
