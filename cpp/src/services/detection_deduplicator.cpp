#include "services/detection_deduplicator.h"
#include "utils/math_helpers.h"

#include <algorithm>

namespace sag {

std::vector<MappedPlayerState> deduplicate_detections(
    const std::vector<MappedPlayerState>& detections,
    float distance_threshold)
{
    // Sort by confidence descending
    auto sorted = detections;
    std::sort(sorted.begin(), sorted.end(),
        [](const MappedPlayerState& a, const MappedPlayerState& b) {
            return a.confidence > b.confidence;
        });

    std::vector<MappedPlayerState> unique;
    unique.reserve(sorted.size());

    for (auto& detection : sorted) {
        if (!detection.standing_point) {
            unique.push_back(detection);
            continue;
        }

        bool duplicate_found = false;
        for (auto& existing : unique) {
            if (!existing.standing_point)
                continue;

            // Same cell → duplicate
            if (detection.occupied_cell && existing.occupied_cell
                && *detection.occupied_cell == *existing.occupied_cell) {
                duplicate_found = true;
                break;
            }

            // Close distance → duplicate
            if (distance(*detection.standing_point, *existing.standing_point) <= distance_threshold) {
                duplicate_found = true;
                break;
            }
        }

        if (!duplicate_found)
            unique.push_back(detection);
    }

    return unique;
}

} // namespace sag
