from __future__ import annotations

import unittest

from src.models.contracts import MappedPlayerState
from src.models.enums import PlayerTrackingState
from src.models.player_model import PlayerModel
from src.services.player_tracker_service import PlayerTrackerService


class PlayerTrackerServiceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.service = PlayerTrackerService(max_match_distance=0.75)

    def test_creates_stable_ids_for_new_players(self) -> None:
        players: dict[str, PlayerModel] = {}
        detections = [
            MappedPlayerState(player_id="d1", standing_point=(0.5, 0.5), in_bounds=True),
            MappedPlayerState(player_id="d2", standing_point=(2.0, 2.0), in_bounds=True),
        ]

        self.service.update_players(players, detections, timestamp=10.0, grace_period_seconds=0.35)

        self.assertEqual(set(players.keys()), {"P1", "P2"})
        self.assertEqual(players["P1"].tracking_state, PlayerTrackingState.ACTIVE)
        self.assertEqual(players["P2"].tracking_state, PlayerTrackingState.ACTIVE)

    def test_matches_nearest_existing_players(self) -> None:
        players = {
            "P1": PlayerModel(player_id="P1", standing_point=(1.0, 1.0), tracking_state=PlayerTrackingState.ACTIVE),
            "P2": PlayerModel(player_id="P2", standing_point=(3.0, 3.0), tracking_state=PlayerTrackingState.ACTIVE),
        }
        detections = [
            MappedPlayerState(player_id="d1", standing_point=(1.2, 1.1), in_bounds=True),
            MappedPlayerState(player_id="d2", standing_point=(3.2, 2.9), in_bounds=True),
        ]

        self.service.update_players(players, detections, timestamp=12.0, grace_period_seconds=0.35)

        self.assertAlmostEqual(players["P1"].standing_point[0], 1.2)
        self.assertAlmostEqual(players["P2"].standing_point[0], 3.2)

    def test_keeps_last_known_position_during_grace_period(self) -> None:
        players = {
            "P1": PlayerModel(
                player_id="P1",
                standing_point=(1.0, 1.0),
                tracking_state=PlayerTrackingState.ACTIVE,
                last_seen_at=10.0,
            )
        }

        self.service.update_players(players, [], timestamp=10.2, grace_period_seconds=0.35)

        self.assertEqual(players["P1"].tracking_state, PlayerTrackingState.ACTIVE)
        self.assertEqual(players["P1"].standing_point, (1.0, 1.0))

    def test_marks_player_missing_after_grace_period(self) -> None:
        players = {
            "P1": PlayerModel(
                player_id="P1",
                standing_point=(1.0, 1.0),
                tracking_state=PlayerTrackingState.ACTIVE,
                last_seen_at=10.0,
            )
        }

        self.service.update_players(players, [], timestamp=10.5, grace_period_seconds=0.35)

        self.assertEqual(players["P1"].tracking_state, PlayerTrackingState.MISSING)
        self.assertIsNone(players["P1"].standing_point)

    def test_reuses_missing_player_id_for_new_detection(self) -> None:
        players = {
            "P1": PlayerModel(
                player_id="P1",
                standing_point=None,
                tracking_state=PlayerTrackingState.MISSING,
                last_seen_at=10.0,
            )
        }
        detections = [MappedPlayerState(player_id="d1", standing_point=(1.5, 1.5), in_bounds=True)]

        self.service.update_players(players, detections, timestamp=10.6, grace_period_seconds=0.35)

        self.assertEqual(set(players.keys()), {"P1"})
        self.assertEqual(players["P1"].tracking_state, PlayerTrackingState.ACTIVE)
        self.assertEqual(players["P1"].standing_point, (1.5, 1.5))

    def test_does_not_allocate_player_for_unmappable_detection(self) -> None:
        players: dict[str, PlayerModel] = {}
        detections = [MappedPlayerState(player_id="d1", standing_point=None, in_bounds=False)]

        self.service.update_players(players, detections, timestamp=10.0, grace_period_seconds=0.35)

        self.assertEqual(players, {})


if __name__ == "__main__":
    unittest.main()