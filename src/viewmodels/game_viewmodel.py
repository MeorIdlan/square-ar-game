from __future__ import annotations

from PyQt6.QtCore import QObject, pyqtSignal

from src.models.game_session_model import GameSessionModel
from src.models.player_model import PlayerModel
from src.services.game_engine_service import GameEngineService


class GameViewModel(QObject):
    session_updated = pyqtSignal(object)

    def __init__(self, session_model: GameSessionModel, game_engine_service: GameEngineService) -> None:
        super().__init__()
        self._session_model = session_model
        self._game_engine_service = game_engine_service

    @property
    def session_model(self) -> GameSessionModel:
        return self._session_model

    def ensure_demo_players(self) -> None:
        if self._session_model.players:
            return
        for player_id in ("P1", "P2"):
            self._session_model.players[player_id] = PlayerModel(player_id=player_id)
        self.session_updated.emit(self._session_model)

    def start_round(self) -> None:
        self._game_engine_service.start_round(self._session_model)
        self.session_updated.emit(self._session_model)

    def lock_round(self) -> None:
        self._game_engine_service.lock_round(self._session_model)
        self.session_updated.emit(self._session_model)
