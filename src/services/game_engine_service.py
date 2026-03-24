from __future__ import annotations

from random import Random

from src.models.contracts import MappedPlayerState, RenderPlayerState, RenderState
from src.models.enums import AppState, CellState, PlayerTrackingState, RoundPhase
from src.models.game_session_model import GameSessionModel


class GameEngineService:
    def __init__(self, seed: int | None = None) -> None:
        self._random = Random(seed)

    def start_round(self, session: GameSessionModel) -> None:
        round_state = session.round_state
        round_state.round_number += 1
        round_state.phase = RoundPhase.FLASHING
        round_state.timer_remaining = round_state.timings.flashing_duration_seconds
        round_state.participant_ids = []
        round_state.survivor_ids = []
        round_state.eliminated_ids = []

        for cell_index in session.grid.cell_states:
            session.grid.cell_states[cell_index] = CellState.FLASHING

        for player in session.players.values():
            player.active_in_round = (
                player.tracking_state is PlayerTrackingState.ACTIVE and player.occupied_cell is not None
            )
            player.eliminated_in_round = False
            if player.active_in_round:
                round_state.participant_ids.append(player.player_id)

        session.app_state = AppState.RUNNING
        session.status_message = f"Round {round_state.round_number} flashing"

    def lock_round(self, session: GameSessionModel) -> None:
        session.round_state.phase = RoundPhase.LOCKED
        session.round_state.timer_remaining = session.round_state.timings.reaction_window_seconds
        for cell_index in session.grid.cell_states:
            session.grid.cell_states[cell_index] = self._random.choice((CellState.GREEN, CellState.RED))
        session.status_message = f"Round {session.round_state.round_number} locked"

    def evaluate_round(self, session: GameSessionModel, mapped_players: list[MappedPlayerState]) -> None:
        players_by_id = {player.player_id: player for player in mapped_players}
        round_state = session.round_state
        round_state.phase = RoundPhase.RESULTS
        round_state.survivor_ids = []
        round_state.eliminated_ids = []

        for participant_id in round_state.participant_ids:
            mapped_player = players_by_id.get(participant_id)
            session_player = session.players[participant_id]
            eliminated = (
                mapped_player is None
                or not mapped_player.in_bounds
                or mapped_player.occupied_cell is None
                or mapped_player.ambiguous
                or session.grid.cell_states.get(mapped_player.occupied_cell) is CellState.RED
            )
            session_player.eliminated_in_round = eliminated
            session_player.active_in_round = not eliminated
            session_player.tracking_state = (
                PlayerTrackingState.ELIMINATED if eliminated else PlayerTrackingState.ACTIVE
            )

            if eliminated:
                round_state.eliminated_ids.append(participant_id)
            else:
                round_state.survivor_ids.append(participant_id)

        if len(round_state.survivor_ids) == 1:
            session.winner_id = round_state.survivor_ids[0]
            session.app_state = AppState.FINISHED
            round_state.phase = RoundPhase.FINISHED
            session.status_message = f"Winner: {session.winner_id}"
        else:
            session.status_message = (
                f"Round {round_state.round_number} results: "
                f"{len(round_state.survivor_ids)} survivors, {len(round_state.eliminated_ids)} eliminated"
            )

    def build_render_state(self, session: GameSessionModel) -> RenderState:
        render_players = []
        for player in session.players.values():
            status_text = player.tracking_state.name.lower().replace("_", " ")
            render_players.append(
                RenderPlayerState(
                    player_id=player.player_id,
                    standing_point=player.standing_point,
                    left_foot=player.left_foot,
                    right_foot=player.right_foot,
                    occupied_cell=player.occupied_cell,
                    status_text=status_text,
                )
            )

        return RenderState(
            phase=session.round_state.phase,
            timer_text=f"{session.round_state.timer_remaining:0.1f}",
            status_text=session.status_message,
            grid_cells=dict(session.grid.cell_states),
            players=render_players,
        )
