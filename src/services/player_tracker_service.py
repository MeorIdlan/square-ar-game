from __future__ import annotations

from math import dist

from src.models.contracts import MappedPlayerState
from src.models.enums import PlayerTrackingState
from src.models.player_model import PlayerModel


class PlayerTrackerService:
    def __init__(self, max_match_distance: float = 1.25) -> None:
        self._max_match_distance = max_match_distance

    def update_players(
        self,
        players: dict[str, PlayerModel],
        detections: list[MappedPlayerState],
        timestamp: float,
        grace_period_seconds: float,
    ) -> None:
        unmatched_player_ids = {
            player_id
            for player_id, player in players.items()
            if player.tracking_state is not PlayerTrackingState.ELIMINATED
        }

        for detection in sorted(detections, key=self._sort_key):
            if detection.standing_point is None:
                continue
            matched_player_id = self._find_best_match(players, unmatched_player_ids, detection)
            if matched_player_id is None:
                matched_player_id = self._allocate_player_id(players)
                players[matched_player_id] = PlayerModel(player_id=matched_player_id)

            unmatched_player_ids.discard(matched_player_id)
            self._apply_detection(players[matched_player_id], detection, timestamp)

        for player_id in unmatched_player_ids:
            player = players[player_id]
            if player.standing_point is None:
                player.tracking_state = PlayerTrackingState.MISSING
                continue

            if timestamp - player.last_seen_at > grace_period_seconds:
                player.tracking_state = PlayerTrackingState.MISSING
                player.standing_point = None
                player.left_foot = None
                player.right_foot = None
                player.occupied_cell = None
                player.confidence = 0.0

    def _find_best_match(
        self,
        players: dict[str, PlayerModel],
        candidate_ids: set[str],
        detection: MappedPlayerState,
    ) -> str | None:
        best_player_id: str | None = None
        best_distance: float | None = None

        for player_id in candidate_ids:
            player = players[player_id]
            if player.standing_point is None:
                if player.tracking_state in (PlayerTrackingState.UNKNOWN, PlayerTrackingState.MISSING):
                    return player_id
                continue

            candidate_distance = dist(player.standing_point, detection.standing_point)
            if candidate_distance > self._max_match_distance:
                continue
            if best_distance is None or candidate_distance < best_distance:
                best_distance = candidate_distance
                best_player_id = player_id

        return best_player_id

    def _apply_detection(self, player: PlayerModel, detection: MappedPlayerState, timestamp: float) -> None:
        player.standing_point = detection.standing_point
        player.left_foot = detection.left_foot
        player.right_foot = detection.right_foot
        player.occupied_cell = detection.occupied_cell
        player.confidence = detection.confidence
        player.last_seen_at = timestamp
        player.tracking_state = PlayerTrackingState.ACTIVE

    def _allocate_player_id(self, players: dict[str, PlayerModel]) -> str:
        index = 1
        while f"P{index}" in players:
            index += 1
        return f"P{index}"

    @staticmethod
    def _sort_key(detection: MappedPlayerState) -> tuple[int, float, float]:
        if detection.standing_point is None:
            return (1, float("inf"), float("inf"))
        return (0, detection.standing_point[0], detection.standing_point[1])
