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
        self.session.players = {
            "P1": PlayerModel(
                player_id="P1",
                tracking_state=PlayerTrackingState.ACTIVE,
                occupied_cell=(0, 0),
            ),
            "P2": PlayerModel(
                player_id="P2",
                tracking_state=PlayerTrackingState.ACTIVE,
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
        self.assertTrue(all(state is CellState.FLASHING for state in self.session.grid.cell_states.values()))

    def test_evaluate_round_eliminates_red_and_invalid_players(self) -> None:
        self.engine.start_round(self.session)
        self.session.grid.cell_states[(0, 0)] = CellState.GREEN
        self.session.grid.cell_states[(1, 1)] = CellState.RED
        mapped_players = [
            MappedPlayerState(player_id="P1", standing_point=(0.5, 0.5), occupied_cell=(0, 0), in_bounds=True),
            MappedPlayerState(player_id="P2", standing_point=(1.5, 1.5), occupied_cell=(1, 1), in_bounds=True),
        ]

        self.engine.evaluate_round(self.session, mapped_players)

        self.assertEqual(self.session.round_state.survivor_ids, ["P1"])
        self.assertEqual(self.session.round_state.eliminated_ids, ["P2"])
        self.assertEqual(self.session.winner_id, "P1")
        self.assertEqual(self.session.app_state, AppState.FINISHED)
        self.assertEqual(self.session.round_state.phase, RoundPhase.FINISHED)


if __name__ == "__main__":
    unittest.main()
