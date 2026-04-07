#include "services/floor_mapping_service.h"

#include <algorithm>
#include <cmath>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>

namespace sag {

FloorMappingService::FloorMappingService(float boundary_tolerance)
    : boundary_tolerance_(boundary_tolerance) {}

std::optional<Point> FloorMappingService::map_image_point_to_floor(
    const std::optional<Point>& point,
    const CalibrationModel& calibration) const
{
    if (!point || calibration.homography.empty())
        return std::nullopt;

    std::vector<cv::Point2f> src = {{point->x, point->y}};
    std::vector<cv::Point2f> dst;
    cv::perspectiveTransform(src, dst, calibration.homography);
    return Point{dst[0].x, dst[0].y};
}

bool FloorMappingService::is_inside_playable_area(
    const Point& point,
    const CalibrationModel& calibration) const
{
    if (!calibration.playable_bounds)
        return false;

    auto& bounds = *calibration.playable_bounds;
    float min_x = bounds[0].x, max_x = bounds[0].x;
    float min_y = bounds[0].y, max_y = bounds[0].y;
    for (int i = 1; i < 4; ++i) {
        min_x = std::min(min_x, bounds[i].x);
        max_x = std::max(max_x, bounds[i].x);
        min_y = std::min(min_y, bounds[i].y);
        max_y = std::max(max_y, bounds[i].y);
    }

    return point.x >= min_x && point.x <= max_x
        && point.y >= min_y && point.y <= max_y;
}

std::optional<CellIndex> FloorMappingService::resolve_cell(
    const std::optional<Point>& point,
    const GridModel& grid,
    const CalibrationModel& calibration) const
{
    if (!point || !is_inside_playable_area(*point, calibration))
        return std::nullopt;

    if (!calibration.playable_bounds)
        return std::nullopt;

    auto& bounds = *calibration.playable_bounds;
    float min_x = bounds[0].x, max_x = bounds[0].x;
    float min_y = bounds[0].y, max_y = bounds[0].y;
    for (int i = 1; i < 4; ++i) {
        min_x = std::min(min_x, bounds[i].x);
        max_x = std::max(max_x, bounds[i].x);
        min_y = std::min(min_y, bounds[i].y);
        max_y = std::max(max_y, bounds[i].y);
    }

    float playable_width  = max_x - min_x;
    float playable_height = max_y - min_y;
    float cell_width  = playable_width / static_cast<float>(grid.columns);
    float cell_height = playable_height / static_cast<float>(grid.rows);

    float rel_x = point->x - min_x;
    float rel_y = point->y - min_y;

    // Boundary check — points exactly on cell edges are ambiguous
    float mod_x = std::fmod(rel_x, cell_width);
    float mod_y = std::fmod(rel_y, cell_height);
    if (std::abs(mod_x) <= boundary_tolerance_ || std::abs(mod_x - cell_width) <= boundary_tolerance_)
        return std::nullopt;
    if (std::abs(mod_y) <= boundary_tolerance_ || std::abs(mod_y - cell_height) <= boundary_tolerance_)
        return std::nullopt;

    int column = static_cast<int>(rel_x / cell_width);
    int row    = static_cast<int>(rel_y / cell_height);

    if (row < 0 || row >= grid.rows || column < 0 || column >= grid.columns)
        return std::nullopt;

    return CellIndex{row, column};
}

MappedPlayerState FloorMappingService::map_detection(
    const std::string& player_id,
    const PoseFootState& pose_state,
    const GridModel& grid,
    const CalibrationModel& calibration) const
{
    auto left_foot  = map_image_point_to_floor(pose_state.left_foot, calibration);
    auto right_foot = map_image_point_to_floor(pose_state.right_foot, calibration);
    auto standing   = resolve_feet_midpoint(left_foot, right_foot);
    auto cell       = resolve_cell(standing, grid, calibration);
    bool in_bounds  = standing.has_value() && is_inside_playable_area(*standing, calibration);
    bool ambiguous  = in_bounds && !cell.has_value();

    return MappedPlayerState{
        .player_id     = player_id,
        .standing_point = standing,
        .left_foot     = left_foot,
        .right_foot    = right_foot,
        .occupied_cell = cell,
        .in_bounds     = in_bounds,
        .ambiguous     = ambiguous,
        .confidence    = pose_state.confidence,
    };
}

} // namespace sag
