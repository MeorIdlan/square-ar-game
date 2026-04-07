from __future__ import annotations

from time import monotonic

from PyQt6.QtCore import QObject, QTimer, pyqtSignal

from src.models.enums import RoundPhase
from src.models.game_session_model import GameSessionModel
from src.services.protocols import GameEngineServiceProtocol
from src.utils.constants import GAME_TICK_INTERVAL_MS


class GameViewModel(QObject):
    session_updated = pyqtSignal(object)

    def __init__(
        self,
        session_model: GameSessionModel,
        game_engine_service: GameEngineServiceProtocol,
    ) -> None:
        super().__init__()
        self._session_model = session_model
        self._game_engine_service = game_engine_service
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._on_timer_tick)
        self._last_tick_at = monotonic()

    @property
    def session_model(self) -> GameSessionModel:
        return self._session_model

    def start_round(self) -> None:
        self._game_engine_service.start_round(self._session_model)
        self._ensure_timer_running()
        self.session_updated.emit(self._session_model)

    def lock_round(self) -> None:
        self._game_engine_service.lock_round(self._session_model)
        self._ensure_timer_running()
        self.session_updated.emit(self._session_model)

    def force_next_round(self) -> None:
        self._game_engine_service.force_next_round(self._session_model)
        self._ensure_timer_running()
        self.session_updated.emit(self._session_model)

    def revive_player(self, player_id: str) -> None:
        self._game_engine_service.revive_player(self._session_model, player_id)
        self.session_updated.emit(self._session_model)

    def eliminate_player(self, player_id: str) -> None:
        self._game_engine_service.eliminate_player(self._session_model, player_id)
        self.session_updated.emit(self._session_model)

    def remove_player(self, player_id: str) -> None:
        self._game_engine_service.remove_player(self._session_model, player_id)
        self.session_updated.emit(self._session_model)

    def reset_session(self) -> None:
        self._game_engine_service.reset_session(self._session_model)
        self._timer.stop()
        self.session_updated.emit(self._session_model)

    def _ensure_timer_running(self) -> None:
        if self._session_model.round_state.phase in (
            RoundPhase.IDLE,
            RoundPhase.FINISHED,
        ):
            return
        self._last_tick_at = monotonic()
        if not self._timer.isActive():
            self._timer.start(GAME_TICK_INTERVAL_MS)

    def _on_timer_tick(self) -> None:
        now = monotonic()
        delta_seconds = max(0.0, now - self._last_tick_at)
        self._last_tick_at = now
        self._game_engine_service.tick(self._session_model, delta_seconds)
        if self._session_model.round_state.phase in (
            RoundPhase.IDLE,
            RoundPhase.FINISHED,
        ):
            self._timer.stop()
        self.session_updated.emit(self._session_model)
