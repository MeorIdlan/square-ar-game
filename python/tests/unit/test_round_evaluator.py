from __future__ import annotations

from src.models.contracts import MappedPlayerState
from src.models.enums import CellState, PlayerTrackingState
from src.models.game_session_model import GameSessionModel
from src.models.player_model import PlayerModel
from src.services.round_evaluator import RoundEvaluator


def _session_with_grid() -> GameSessionModel:
    session = GameSessionModel()
    session.grid.cell_states[(0, 0)] = CellState.GREEN
    session.grid.cell_states[(0, 1)] = CellState.RED
    session.grid.cell_states[(1, 0)] = CellState.GREEN
    session.grid.cell_states[(1, 1)] = CellState.RED
    return session


class TestRoundEvaluator:
    def test_player_on_red_eliminated(self) -> None:
        session = _session_with_grid()
        session.round_state.participant_ids = ["P1"]
        session.players = {
            "P1": PlayerModel(player_id="P1", tracking_state=PlayerTrackingState.ACTIVE)
        }
        mapped = [
            MappedPlayerState(
                player_id="P1",
                standing_point=(1.5, 0.5),
                occupied_cell=(0, 1),
                in_bounds=True,
            )
        ]
        evaluator = RoundEvaluator()
        result = evaluator.evaluate(session, mapped)
        assert "P1" in result.eliminated
        assert "standing on red cell" in result.reason_map["P1"]

    def test_player_on_green_survives(self) -> None:
        session = _session_with_grid()
        session.round_state.participant_ids = ["P1"]
        session.players = {
            "P1": PlayerModel(player_id="P1", tracking_state=PlayerTrackingState.ACTIVE)
        }
        mapped = [
            MappedPlayerState(
                player_id="P1",
                standing_point=(0.5, 0.5),
                occupied_cell=(0, 0),
                in_bounds=True,
            )
        ]
        evaluator = RoundEvaluator()
        result = evaluator.evaluate(session, mapped)
        assert "P1" in result.survivors
        assert "P1" not in result.eliminated

    def test_player_out_of_bounds_eliminated(self) -> None:
        session = _session_with_grid()
        session.round_state.participant_ids = ["P1"]
        session.players = {
            "P1": PlayerModel(player_id="P1", tracking_state=PlayerTrackingState.ACTIVE)
        }
        mapped = [
            MappedPlayerState(
                player_id="P1", standing_point=(5.0, 5.0), in_bounds=False
            )
        ]
        evaluator = RoundEvaluator()
        result = evaluator.evaluate(session, mapped)
        assert "P1" in result.eliminated
        assert "out of bounds" in result.reason_map["P1"]

    def test_ambiguous_position_eliminated(self) -> None:
        session = _session_with_grid()
        session.round_state.participant_ids = ["P1"]
        session.players = {
            "P1": PlayerModel(player_id="P1", tracking_state=PlayerTrackingState.ACTIVE)
        }
        mapped = [
            MappedPlayerState(
                player_id="P1",
                standing_point=(1.0, 1.0),
                occupied_cell=None,
                in_bounds=True,
                ambiguous=True,
            )
        ]
        evaluator = RoundEvaluator()
        result = evaluator.evaluate(session, mapped)
        assert "P1" in result.eliminated

    def test_missing_player_eliminated(self) -> None:
        session = _session_with_grid()
        session.round_state.participant_ids = ["P1"]
        session.players = {
            "P1": PlayerModel(
                player_id="P1", tracking_state=PlayerTrackingState.MISSING
            )
        }
        mapped = [
            MappedPlayerState(player_id="P1", standing_point=None, in_bounds=False)
        ]
        evaluator = RoundEvaluator()
        result = evaluator.evaluate(session, mapped)
        assert "P1" in result.eliminated

    def test_player_not_in_mapped_eliminated(self) -> None:
        session = _session_with_grid()
        session.round_state.participant_ids = ["P1"]
        session.players = {
            "P1": PlayerModel(player_id="P1", tracking_state=PlayerTrackingState.ACTIVE)
        }
        evaluator = RoundEvaluator()
        result = evaluator.evaluate(session, [])
        assert "P1" in result.eliminated
        assert "not found" in result.reason_map["P1"]

    def test_all_players_eliminated(self) -> None:
        session = _session_with_grid()
        session.round_state.participant_ids = ["P1", "P2"]
        session.players = {
            "P1": PlayerModel(
                player_id="P1", tracking_state=PlayerTrackingState.ACTIVE
            ),
            "P2": PlayerModel(
                player_id="P2", tracking_state=PlayerTrackingState.ACTIVE
            ),
        }
        mapped = [
            MappedPlayerState(
                player_id="P1",
                standing_point=(1.5, 0.5),
                occupied_cell=(0, 1),
                in_bounds=True,
            ),
            MappedPlayerState(
                player_id="P2",
                standing_point=(1.5, 1.5),
                occupied_cell=(1, 1),
                in_bounds=True,
            ),
        ]
        evaluator = RoundEvaluator()
        result = evaluator.evaluate(session, mapped)
        assert len(result.eliminated) == 2
        assert len(result.survivors) == 0
