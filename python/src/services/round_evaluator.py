from __future__ import annotations

from dataclasses import dataclass, field

from src.models.contracts import MappedPlayerState
from src.models.enums import CellState, PlayerTrackingState
from src.models.game_session_model import GameSessionModel


@dataclass(slots=True)
class EvaluationResult:
    survivors: list[str] = field(default_factory=list)
    eliminated: list[str] = field(default_factory=list)
    reason_map: dict[str, str] = field(default_factory=dict)


class RoundEvaluator:
    def evaluate(
        self,
        session: GameSessionModel,
        mapped_players: list[MappedPlayerState],
    ) -> EvaluationResult:
        players_by_id = {player.player_id: player for player in mapped_players}
        result = EvaluationResult()

        for participant_id in session.round_state.participant_ids:
            mapped_player = players_by_id.get(participant_id)
            session_player = session.players[participant_id]

            eliminated, reason = self._check_elimination(
                mapped_player, session_player.tracking_state, session.grid.cell_states
            )

            if eliminated:
                result.eliminated.append(participant_id)
                result.reason_map[participant_id] = reason
            else:
                result.survivors.append(participant_id)

        return result

    @staticmethod
    def _check_elimination(
        mapped_player: MappedPlayerState | None,
        tracking_state: PlayerTrackingState,
        cell_states: dict[tuple[int, int], CellState],
    ) -> tuple[bool, str]:
        if mapped_player is None:
            return True, "not found in mapped players"
        if tracking_state is PlayerTrackingState.MISSING:
            return True, "player missing"
        if not mapped_player.in_bounds:
            return True, "out of bounds"
        if mapped_player.occupied_cell is None:
            return True, "no occupied cell"
        if mapped_player.ambiguous:
            return True, "ambiguous position"
        if cell_states.get(mapped_player.occupied_cell) is CellState.RED:
            return True, "standing on red cell"
        return False, ""
