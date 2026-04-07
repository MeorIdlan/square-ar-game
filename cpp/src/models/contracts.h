#pragma once
// Data transfer contracts — mirrors src/models/contracts.py

#include "models/enums.h"
#include "utils/math_helpers.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <opencv2/core.hpp>

namespace sag {

// ── Camera → Vision pipeline ─────────────────────────────────────────

struct FramePacket {
    int frame_id          = 0;
    double timestamp      = 0.0;
    cv::Mat frame;                        // BGR or empty
    int camera_index      = -1;
    bool is_live          = false;
    std::string source_name = "fallback";
    std::string error_message;
    float actual_fps      = 0.0f;
};

// ── Vision → Tracking pipeline ───────────────────────────────────────

struct PoseFootState {
    std::optional<Point> left_foot;
    std::optional<Point> right_foot;
    std::optional<Point> resolved_point;
    float confidence = 0.0f;
};

struct PoseResult {
    int frame_id        = 0;
    double timestamp    = 0.0;
    std::vector<PoseFootState> detections;
    int raw_pose_count  = 0;
    std::string status_text = "Pose tracking idle";
};

// ── Floor mapping result ─────────────────────────────────────────────

struct MappedPlayerState {
    std::string player_id;
    std::optional<Point> standing_point;
    std::optional<Point> left_foot;
    std::optional<Point> right_foot;
    std::optional<CellIndex> occupied_cell;
    bool in_bounds   = false;
    bool ambiguous   = false;
    float confidence = 0.0f;
    double last_seen_at = 0.0;
};

// ── Render DTO ───────────────────────────────────────────────────────

struct RenderPlayerState {
    std::string player_id;
    std::optional<Point> standing_point;
    std::optional<Point> left_foot;
    std::optional<Point> right_foot;
    std::optional<CellIndex> occupied_cell;
    std::string status_text;
};

struct RenderState {
    RoundPhase phase = RoundPhase::Idle;
    std::string timer_text            = "00.0";
    std::string status_text           = "Idle";
    std::string camera_status_text    = "Camera status unknown";
    std::string pose_status_text      = "Pose tracking idle";
    std::string calibration_status_text = "Not calibrated";
    std::string display_status_text   = "Projector not assigned";

    std::unordered_map<CellIndex, CellState, CellIndexHash> grid_cells;
    std::vector<RenderPlayerState> players;
};

} // namespace sag
