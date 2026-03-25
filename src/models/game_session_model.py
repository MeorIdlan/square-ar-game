from __future__ import annotations

from dataclasses import dataclass, field

from src.models.calibration_model import CalibrationModel
from src.models.enums import AppState
from src.models.grid_model import GridModel
from src.models.player_model import PlayerModel
from src.models.round_model import RoundModel


@dataclass(slots=True)
class GameSessionModel:
    app_state: AppState = AppState.BOOTING
    calibration: CalibrationModel = field(default_factory=CalibrationModel)
    grid: GridModel = field(default_factory=GridModel)
    round_state: RoundModel = field(default_factory=RoundModel)
    players: dict[str, PlayerModel] = field(default_factory=dict)
    winner_id: str | None = None
    status_message: str = "Booting"
    camera_status_message: str = "Waiting for camera frames"
    display_status_message: str = "Projector not assigned"
