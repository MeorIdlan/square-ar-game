#pragma once
// FloorMappingService — mirrors src/services/floor_mapping_service.py

#include "models/calibration_model.h"
#include "models/contracts.h"
#include "models/grid_model.h"
#include "utils/constants.h"

#include <optional>
#include <string>

namespace sag {

class FloorMappingService {
public:
    explicit FloorMappingService(float boundary_tolerance = constants::BOUNDARY_TOLERANCE);

    MappedPlayerState map_detection(
        const std::string& player_id,
        const PoseFootState& pose_state,
        const GridModel& grid,
        const CalibrationModel& calibration) const;

    std::optional<Point> map_image_point_to_floor(
        const std::optional<Point>& point,
        const CalibrationModel& calibration) const;

    bool is_inside_playable_area(
        const Point& point,
        const CalibrationModel& calibration) const;

    std::optional<CellIndex> resolve_cell(
        const std::optional<Point>& point,
        const GridModel& grid,
        const CalibrationModel& calibration) const;

private:
    float boundary_tolerance_;
};

} // namespace sag
