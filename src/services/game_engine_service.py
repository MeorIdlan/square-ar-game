from __future__ import annotations

import logging
from random import Random

from src.models.contracts import MappedPlayerState, RenderPlayerState, RenderState
from src.models.enums import AppState, CellState, PlayerTrackingState, RoundPhase
from src.models.game_session_model import GameSessionModel


class GameEngineService:
    def __init__(self, seed: int | None = None) -> None:
        self._random = Random(seed)
        self._logger = logging.getLogger(__name__)

    def start_round(self, session: GameSessionModel) -> None:
        active_candidates = [
            player.player_id
            for player in session.players.values()
            if player.tracking_state is PlayerTrackingState.ACTIVE and player.occupied_cell is not None
        ]
        if not active_candidates:
            session.round_state.phase = RoundPhase.IDLE
            session.round_state.timer_remaining = 0.0
            session.status_message = "No active participants in valid cells"
            self._logger.warning("Round start rejected because there are no active participants in valid cells")
            return

        if len(active_candidates) == 1:
            session.winner_id = active_candidates[0]
            session.app_state = AppState.FINISHED
            session.round_state.phase = RoundPhase.FINISHED
            session.round_state.timer_remaining = 0.0
            session.status_message = f"Winner: {session.winner_id}"
            self._logger.info("Round start immediately finished with winner %s", session.winner_id)
            return

        round_state = session.round_state
        round_state.round_number += 1
        round_state.phase = RoundPhase.FLASHING
        round_state.timer_remaining = round_state.timings.flashing_duration_seconds
        round_state.flash_elapsed_seconds = 0.0
        round_state.participant_ids = []
        round_state.survivor_ids = []
        round_state.eliminated_ids = []

        for player in session.players.values():
            player.active_in_round = (
                player.tracking_state is PlayerTrackingState.ACTIVE and player.occupied_cell is not None
            )
            player.eliminated_in_round = False
            if player.active_in_round:
                round_state.participant_ids.append(player.player_id)

        session.app_state = AppState.RUNNING
        self.randomize_flashing_pattern(session)
        session.status_message = f"Round {round_state.round_number} flashing"
        self._logger.info("Round %s started with participants=%s", round_state.round_number, round_state.participant_ids)

    def randomize_flashing_pattern(self, session: GameSessionModel) -> None:
        for cell_index in session.grid.cell_states:
            session.grid.cell_states[cell_index] = self._random.choice((CellState.GREEN, CellState.RED))

    def lock_round(self, session: GameSessionModel) -> None:
        session.round_state.phase = RoundPhase.LOCKED
        session.round_state.timer_remaining = session.round_state.timings.reaction_window_seconds
        for cell_index in session.grid.cell_states:
            session.grid.cell_states[cell_index] = self._random.choice((CellState.GREEN, CellState.RED))
        session.status_message = f"Round {session.round_state.round_number} locked"
        self._logger.info("Round %s locked", session.round_state.round_number)

    def begin_check(self, session: GameSessionModel) -> None:
        session.round_state.phase = RoundPhase.CHECKING
        session.round_state.timer_remaining = session.round_state.timings.lock_delay_seconds
        session.status_message = f"Round {session.round_state.round_number} checking positions"
        self._logger.info("Round %s checking positions", session.round_state.round_number)

    def begin_preparing_next_round(self, session: GameSessionModel) -> None:
        session.round_state.phase = RoundPhase.PREPARING
        session.round_state.timer_remaining = session.round_state.timings.inter_round_delay_seconds
        session.status_message = f"Next round in {session.round_state.timer_remaining:0.1f}s"
        self._logger.info("Preparing next round after round %s", session.round_state.round_number)

    def tick(self, session: GameSessionModel, delta_seconds: float) -> None:
        round_state = session.round_state
        if round_state.phase in (RoundPhase.IDLE, RoundPhase.FINISHED):
            return

        round_state.timer_remaining = max(0.0, round_state.timer_remaining - delta_seconds)

        if round_state.phase is RoundPhase.FLASHING:
            round_state.flash_elapsed_seconds += delta_seconds
            interval = max(round_state.timings.flash_interval_seconds, 0.05)
            while round_state.flash_elapsed_seconds >= interval and round_state.timer_remaining > 0.0:
                round_state.flash_elapsed_seconds -= interval
                self.randomize_flashing_pattern(session)

            if round_state.timer_remaining <= 0.0:
                self.lock_round(session)
            else:
                session.status_message = f"Round {round_state.round_number} flashing"

        elif round_state.phase is RoundPhase.LOCKED:
            if round_state.timer_remaining <= 0.0:
                self.begin_check(session)
            else:
                session.status_message = f"Round {round_state.round_number} locked"

        elif round_state.phase is RoundPhase.CHECKING:
            if round_state.timer_remaining <= 0.0:
                self.evaluate_round(session)
            else:
                session.status_message = f"Round {round_state.round_number} checking positions"

        elif round_state.phase is RoundPhase.RESULTS:
            if round_state.timer_remaining <= 0.0:
                if session.app_state is AppState.FINISHED:
                    round_state.phase = RoundPhase.FINISHED
                else:
                    self.begin_preparing_next_round(session)

        elif round_state.phase is RoundPhase.PREPARING:
            if round_state.timer_remaining <= 0.0:
                self.start_round(session)
            else:
                session.status_message = f"Next round in {round_state.timer_remaining:0.1f}s"

    def current_mapped_players(self, session: GameSessionModel) -> list[MappedPlayerState]:
        return [
            MappedPlayerState(
                player_id=player.player_id,
                standing_point=player.standing_point,
                left_foot=player.left_foot,
                right_foot=player.right_foot,
                occupied_cell=player.occupied_cell,
                in_bounds=player.occupied_cell is not None,
                ambiguous=player.occupied_cell is None and player.standing_point is not None,
                confidence=player.confidence,
                last_seen_at=player.last_seen_at,
            )
            for player in session.players.values()
        ]

    def evaluate_round(self, session: GameSessionModel, mapped_players: list[MappedPlayerState] | None = None) -> None:
        players_by_id = {
            player.player_id: player for player in (mapped_players if mapped_players is not None else self.current_mapped_players(session))
        }
        round_state = session.round_state
        round_state.phase = RoundPhase.RESULTS
        round_state.timer_remaining = round_state.timings.elimination_display_seconds
        round_state.survivor_ids = []
        round_state.eliminated_ids = []

        for participant_id in round_state.participant_ids:
            mapped_player = players_by_id.get(participant_id)
            session_player = session.players[participant_id]
            eliminated = (
                mapped_player is None
                or session_player.tracking_state is PlayerTrackingState.MISSING
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

        self._refresh_round_outcome(session)
        self._logger.info(
            "Round %s evaluated survivors=%s eliminated=%s",
            round_state.round_number,
            round_state.survivor_ids,
            round_state.eliminated_ids,
        )

    def revive_player(self, session: GameSessionModel, player_id: str) -> None:
        player = session.players.get(player_id)
        if player is None:
            return
        player.eliminated_in_round = False
        player.active_in_round = True
        player.tracking_state = PlayerTrackingState.ACTIVE if player.standing_point is not None else PlayerTrackingState.MISSING
        self._ensure_membership(session.round_state.survivor_ids, player_id)
        self._remove_membership(session.round_state.eliminated_ids, player_id)
        self._ensure_membership(session.round_state.participant_ids, player_id)
        self._refresh_round_outcome(session)

    def eliminate_player(self, session: GameSessionModel, player_id: str) -> None:
        player = session.players.get(player_id)
        if player is None:
            return
        player.eliminated_in_round = True
        player.active_in_round = False
        player.tracking_state = PlayerTrackingState.ELIMINATED
        self._ensure_membership(session.round_state.eliminated_ids, player_id)
        self._remove_membership(session.round_state.survivor_ids, player_id)
        self._ensure_membership(session.round_state.participant_ids, player_id)
        self._refresh_round_outcome(session)

    def remove_player(self, session: GameSessionModel, player_id: str) -> None:
        if player_id not in session.players:
            return
        del session.players[player_id]
        self._remove_membership(session.round_state.participant_ids, player_id)
        self._remove_membership(session.round_state.survivor_ids, player_id)
        self._remove_membership(session.round_state.eliminated_ids, player_id)
        if session.winner_id == player_id:
            session.winner_id = None
        self._refresh_round_outcome(session)

    def reset_session(self, session: GameSessionModel) -> None:
        session.winner_id = None
        session.app_state = AppState.CALIBRATED if session.calibration.is_valid else AppState.CAMERA_READY
        session.status_message = "Session reset"
        session.grid.reset_states(cell_state=CellState.NEUTRAL)
        session.round_state.phase = RoundPhase.IDLE
        session.round_state.timer_remaining = 0.0
        session.round_state.flash_elapsed_seconds = 0.0
        session.round_state.participant_ids = []
        session.round_state.survivor_ids = []
        session.round_state.eliminated_ids = []
        for player in session.players.values():
            player.active_in_round = False
            player.eliminated_in_round = False
            if player.tracking_state is PlayerTrackingState.ELIMINATED:
                player.tracking_state = PlayerTrackingState.MISSING
        self._logger.info("Session reset")

    def force_next_round(self, session: GameSessionModel) -> None:
        self.start_round(session)

    def _refresh_round_outcome(self, session: GameSessionModel) -> None:
        round_state = session.round_state
        round_state.survivor_ids = [
            player_id for player_id in round_state.participant_ids if player_id not in round_state.eliminated_ids
        ]

        if len(round_state.survivor_ids) == 1:
            session.winner_id = round_state.survivor_ids[0]
            session.app_state = AppState.FINISHED
            round_state.phase = RoundPhase.FINISHED
            round_state.timer_remaining = 0.0
            session.status_message = f"Winner: {session.winner_id}"
        else:
            session.winner_id = None
            if round_state.phase is RoundPhase.FINISHED:
                round_state.phase = RoundPhase.RESULTS
            session.status_message = (
                f"Round {round_state.round_number} results: "
                f"{len(round_state.survivor_ids)} survivors, {len(round_state.eliminated_ids)} eliminated"
            )

    @staticmethod
    def _ensure_membership(values: list[str], player_id: str) -> None:
        if player_id not in values:
            values.append(player_id)

    @staticmethod
    def _remove_membership(values: list[str], player_id: str) -> None:
        while player_id in values:
            values.remove(player_id)

    def build_render_state(self, session: GameSessionModel) -> RenderState:
        render_players = []
        for player in session.players.values():
            if player.tracking_state is PlayerTrackingState.MISSING and player.standing_point is None and not player.active_in_round:
                continue
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
            camera_status_text=session.camera_status_message,
            pose_status_text=session.pose_status_message,
            calibration_status_text=session.calibration.validation_message,
            display_status_text=session.display_status_message,
            grid_cells=dict(session.grid.cell_states),
            players=render_players,
        )
