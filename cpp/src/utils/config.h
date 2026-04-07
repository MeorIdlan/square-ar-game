#pragma once
// Application configuration — mirrors src/utils/config.py

#include "models/round_model.h"
#include "utils/constants.h"

#include <string>
#include <vector>

namespace sag {

struct CameraProfile {
    int width  = 1280;
    int height = 720;
    int fps    = 30;

    std::string label() const {
        if (width >= 3840 && height >= 2160)
            return std::to_string(fps) + " FPS 4K";
        return std::to_string(height) + "p " + std::to_string(fps) + " FPS";
    }

    bool operator==(const CameraProfile&) const = default;
};

struct CameraSettings {
    int camera_index  = constants::defaults::CAMERA_INDEX;
    int frame_width   = constants::defaults::FRAME_WIDTH;
    int frame_height  = constants::defaults::FRAME_HEIGHT;
    int target_fps    = constants::defaults::TARGET_FPS;

    CameraProfile profile() const {
        return {frame_width, frame_height, target_fps};
    }

    void apply_profile(const CameraProfile& p) {
        frame_width  = p.width;
        frame_height = p.height;
        target_fps   = p.fps;
    }
};

struct DisplaySettings {
    int projector_screen_index = 0;
    bool show_debug_window     = true;
    bool show_player_markers   = true;
};

struct GridSettings {
    int rows               = constants::defaults::GRID_ROWS;
    int columns            = constants::defaults::GRID_COLUMNS;
    float playable_width   = constants::defaults::PLAYABLE_WIDTH;
    float playable_height  = constants::defaults::PLAYABLE_HEIGHT;
    float playable_inset   = constants::defaults::PLAYABLE_INSET;
};

struct PoseSettings {
    std::string model_asset_path = "assets/models/pose_landmarker_lite.onnx";
    int num_poses                   = 4;
    float min_pose_detection_confidence = 0.35f;
    float min_pose_presence_confidence  = 0.35f;
    float min_tracking_confidence       = 0.35f;
    float min_landmark_visibility       = 0.2f;
    bool use_gpu_if_available           = true;
};

struct AppConfig {
    CameraSettings camera;
    DisplaySettings display;
    PoseSettings pose;
    GridSettings grid;
    RoundTimingSettings timings;
    std::string aruco_dictionary = "DICT_6X6_1000";
    std::vector<int> marker_ids  = {0, 1, 2, 3};
};

// Load configuration from %APPDATA%/square-ar-game/settings.json (or defaults)
AppConfig load_config();

// Save configuration to %APPDATA%/square-ar-game/settings.json
void save_config(const AppConfig& config);

} // namespace sag
