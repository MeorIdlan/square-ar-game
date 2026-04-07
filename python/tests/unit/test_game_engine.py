from __future__ import annotations

import unittest

from src.models.contracts import MappedPlayerState
from src.models.enums import AppState, CellState, PlayerTrackingState, RoundPhase
from src.models.game_session_model import GameSessionModel
from src.models.player_model import PlayerModel
from src.services.game_engine_service import GameEngineService


class GameEngineServiceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.engine = GameEngineService(seed=7)
        self.session = GameSessionModel()
        self.session.round_state.timings.flashing_duration_seconds = 0.5
        self.session.round_state.timings.flash_interval_seconds = 0.1
        self.session.round_state.timings.reaction_window_seconds = 0.4
        self.session.round_state.timings.lock_delay_seconds = 0.2
        self.session.round_state.timings.elimination_display_seconds = 0.3
        self.session.round_state.timings.inter_round_delay_seconds = 0.25
        self.session.players = {
            "P1": PlayerModel(
                player_id="P1",
                tracking_state=PlayerTrackingState.ACTIVE,
                standing_point=(0.5, 0.5),
                occupied_cell=(0, 0),
            ),
            "P2": PlayerModel(
                player_id="P2",
                tracking_state=PlayerTrackingState.ACTIVE,
                standing_point=(1.5, 1.5),
                occupied_cell=(1, 1),
            ),
            "P3": PlayerModel(
                player_id="P3",
                tracking_state=PlayerTrackingState.MISSING,
                occupied_cell=None,
            ),
        }

    def test_start_round_freezes_active_in_bounds_participants(self) -> None:
        self.engine.start_round(self.session)
        self.assertEqual(self.session.round_state.phase, RoundPhase.FLASHING)
        self.assertEqual(self.session.round_state.participant_ids, ["P1", "P2"])
        states = self.session.grid.cell_states.values()
        self.assertTrue(all(s in (CellState.GREEN, CellState.RED) for s in states))

    def test_evaluate_round_eliminates_red_and_invalid_players(self) -> None:
        self.engine.start_round(self.session)
        self.session.grid.cell_states[(0, 0)] = CellState.GREEN
        self.session.grid.cell_states[(1, 1)] = CellState.RED
        mapped_players = [
            MappedPlayerState(
                player_id="P1",
                standing_point=(0.5, 0.5),
                occupied_cell=(0, 0),
                in_bounds=True,
            ),
            MappedPlayerState(
                player_id="P2",
                standing_point=(1.5, 1.5),
                occupied_cell=(1, 1),
                in_bounds=True,
            ),
        ]

        self.engine.evaluate_round(self.session, mapped_players)

        self.assertEqual(self.session.round_state.survivor_ids, ["P1"])
        self.assertEqual(self.session.round_state.eliminated_ids, ["P2"])
        self.assertEqual(self.session.winner_id, "P1")
        self.assertEqual(self.session.app_state, AppState.FINISHED)
        self.assertEqual(self.session.round_state.phase, RoundPhase.FINISHED)

    def test_tick_progresses_round_through_phases(self) -> None:
        self.engine.start_round(self.session)
        self.engine.tick(self.session, 0.5)
        self.assertEqual(self.session.round_state.phase, RoundPhase.LOCKED)

        self.engine.tick(self.session, 0.4)
        self.assertEqual(self.session.round_state.phase, RoundPhase.CHECKING)

    def test_missing_player_is_eliminated_at_check_time(self) -> None:
        self.engine.start_round(self.session)
        self.session.players["P2"].tracking_state = PlayerTrackingState.MISSING
        self.session.grid.cell_states[(0, 0)] = CellState.GREEN
        self.session.grid.cell_states[(1, 1)] = CellState.GREEN

        self.engine.evaluate_round(self.session)

        self.assertIn("P2", self.session.round_state.eliminated_ids)
        self.assertEqual(
            self.session.players["P2"].tracking_state, PlayerTrackingState.ELIMINATED
        )

    def test_operator_override_can_revive_player(self) -> None:
        self.engine.start_round(self.session)
        self.session.grid.cell_states[(0, 0)] = CellState.RED
        self.session.grid.cell_states[(1, 1)] = CellState.GREEN

        self.engine.evaluate_round(self.session)
        self.engine.revive_player(self.session, "P1")

        self.assertIn("P1", self.session.round_state.survivor_ids)
        self.assertNotIn("P1", self.session.round_state.eliminated_ids)

    def test_results_phase_auto_moves_to_preparing(self) -> None:
        self.engine.start_round(self.session)
        self.session.grid.cell_states[(0, 0)] = CellState.GREEN
        self.session.grid.cell_states[(1, 1)] = CellState.GREEN
        self.engine.evaluate_round(self.session)

        self.engine.tick(self.session, 0.3)

        self.assertEqual(self.session.round_state.phase, RoundPhase.PREPARING)

    def test_single_player_auto_win(self) -> None:
        self.session.players = {
            "P1": PlayerModel(
                player_id="P1",
                tracking_state=PlayerTrackingState.ACTIVE,
                standing_point=(0.5, 0.5),
                occupied_cell=(0, 0),
            ),
        }
        self.engine.start_round(self.session)
        self.assertEqual(self.session.winner_id, "P1")
        self.assertEqual(self.session.app_state, AppState.FINISHED)
        self.assertEqual(self.session.round_state.phase, RoundPhase.FINISHED)

    def test_flashing_randomization_reproducible_with_seed(self) -> None:
        engine1 = GameEngineService(seed=42)
        engine2 = GameEngineService(seed=42)
        session1 = GameSessionModel()
        session2 = GameSessionModel()
        session1.grid = self.session.grid
        session2.grid = self.session.grid
        session1.grid.reset_states()
        session2.grid.reset_states()
        engine1.randomize_flashing_pattern(session1)
        engine2.randomize_flashing_pattern(session2)
        self.assertEqual(session1.grid.cell_states, session2.grid.cell_states)

    def test_no_active_participants_rejects_start(self) -> None:
        self.session.players = {
            "P1": PlayerModel(
                player_id="P1",
                tracking_state=PlayerTrackingState.MISSING,
                occupied_cell=None,
            ),
        }
        self.engine.start_round(self.session)
        self.assertEqual(self.session.round_state.phase, RoundPhase.IDLE)


if __name__ == "__main__":
    unittest.main()
