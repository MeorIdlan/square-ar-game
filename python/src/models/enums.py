from __future__ import annotations

from enum import Enum, auto


class AppState(Enum):
    BOOTING = auto()
    CAMERA_READY = auto()
    CALIBRATING = auto()
    CALIBRATED = auto()
    RUNNING = auto()
    PAUSED = auto()
    FINISHED = auto()
    ERROR = auto()


class RoundPhase(Enum):
    IDLE = auto()
    PREPARING = auto()
    FLASHING = auto()
    LOCKED = auto()
    CHECKING = auto()
    RESULTS = auto()
    FINISHED = auto()


class PlayerTrackingState(Enum):
    UNKNOWN = auto()
    ACTIVE = auto()
    MISSING = auto()
    ELIMINATED = auto()


class CellState(Enum):
    INACTIVE = auto()
    NEUTRAL = auto()
    FLASHING = auto()
    GREEN = auto()
    RED = auto()
    LOCKED = auto()


class CalibrationState(Enum):
    NOT_CALIBRATED = auto()
    DETECTING = auto()
    VALID = auto()
    INVALID = auto()
