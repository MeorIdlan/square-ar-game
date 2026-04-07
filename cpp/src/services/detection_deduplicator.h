#pragma once
// Detection deduplicator — mirrors src/services/detection_deduplicator.py

#include "models/contracts.h"

#include <vector>

namespace sag {

std::vector<MappedPlayerState> deduplicate_detections(
    const std::vector<MappedPlayerState>& detections,
    float distance_threshold);

} // namespace sag
