#include "services/player_tracker_service.h"
#include "utils/math_helpers.h"

#include <algorithm>
#include <limits>
#include <set>

namespace sag {

PlayerTrackerService::PlayerTrackerService(float max_match_distance)
    : max_match_distance_(max_match_distance) {}

void PlayerTrackerService::update_players(
    std::unordered_map<std::string, PlayerModel>& players,
    const std::vector<MappedPlayerState>& detections,
    double timestamp,
    float grace_period_seconds) const
{
    std::set<std::string> unmatched_player_ids;
    for (auto& [id, player] : players) {
        if (player.tracking_state != PlayerTrackingState::Eliminated)
            unmatched_player_ids.insert(id);
    }

    // Sort detections: prefer those with standing points, then by x,y
    auto sorted = detections;
    std::sort(sorted.begin(), sorted.end(),
        [](const MappedPlayerState& a, const MappedPlayerState& b) {
            bool a_has = a.standing_point.has_value();
            bool b_has = b.standing_point.has_value();
            if (a_has != b_has) return a_has > b_has;
            if (!a_has) return false;
            if (a.standing_point->x != b.standing_point->x)
                return a.standing_point->x < b.standing_point->x;
            return a.standing_point->y < b.standing_point->y;
        });

    for (auto& detection : sorted) {
        if (!detection.standing_point)
            continue;

        auto matched_id = find_best_match(players, unmatched_player_ids, detection);

        if (matched_id.empty()) {
            matched_id = allocate_player_id(players);
            players[matched_id] = PlayerModel{.player_id = matched_id};
        }

        unmatched_player_ids.erase(matched_id);
        apply_detection(players[matched_id], detection, timestamp);
    }

    // Mark unmatched players as MISSING after grace period
    for (auto& player_id : unmatched_player_ids) {
        auto& player = players[player_id];
        if (!player.standing_point) {
            player.tracking_state = PlayerTrackingState::Missing;
            continue;
        }

        if (timestamp - player.last_seen_at > grace_period_seconds) {
            player.tracking_state = PlayerTrackingState::Missing;
            player.standing_point = std::nullopt;
            player.left_foot      = std::nullopt;
            player.right_foot     = std::nullopt;
            player.occupied_cell  = std::nullopt;
            player.confidence     = 0.0f;
        }
    }
}

std::string PlayerTrackerService::find_best_match(
    const std::unordered_map<std::string, PlayerModel>& players,
    const std::set<std::string>& candidate_ids,
    const MappedPlayerState& detection) const
{
    std::string best_id;
    float best_distance = std::numeric_limits<float>::max();

    for (auto& pid : candidate_ids) {
        auto it = players.find(pid);
        if (it == players.end()) continue;
        auto& player = it->second;

        if (!player.standing_point) {
            if (player.tracking_state == PlayerTrackingState::Unknown
                || player.tracking_state == PlayerTrackingState::Missing)
                return pid;
            continue;
        }

        float d = distance(*player.standing_point, *detection.standing_point);
        if (d > max_match_distance_)
            continue;
        if (d < best_distance) {
            best_distance = d;
            best_id = pid;
        }
    }

    return best_id;
}

void PlayerTrackerService::apply_detection(
    PlayerModel& player,
    const MappedPlayerState& detection,
    double timestamp)
{
    player.standing_point  = detection.standing_point;
    player.left_foot       = detection.left_foot;
    player.right_foot      = detection.right_foot;
    player.occupied_cell   = detection.occupied_cell;
    player.confidence      = detection.confidence;
    player.last_seen_at    = timestamp;
    player.tracking_state  = PlayerTrackingState::Active;
}

std::string PlayerTrackerService::allocate_player_id(
    const std::unordered_map<std::string, PlayerModel>& players)
{
    int index = 1;
    while (players.contains("P" + std::to_string(index)))
        ++index;
    return "P" + std::to_string(index);
}

} // namespace sag
