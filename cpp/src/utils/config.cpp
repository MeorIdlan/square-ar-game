#include "utils/config.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace sag
{

    namespace fs = std::filesystem;
    using json = nlohmann::json;

    // ── JSON serialization helpers ───────────────────────────────────────

    static fs::path config_dir()
    {
        const char *appdata = std::getenv("APPDATA");
        if (appdata)
            return fs::path(appdata) / "square-ar-game";
        // Fallback for non-Windows dev environments
        const char *home = std::getenv("HOME");
        if (home)
            return fs::path(home) / ".square-ar-game";
        return fs::path(".square-ar-game");
    }

    static fs::path config_path()
    {
        return config_dir() / "settings.json";
    }

    // ── Load ─────────────────────────────────────────────────────────────

    AppConfig load_config()
    {
        auto path = config_path();
        if (!fs::exists(path))
            return AppConfig{};

        std::ifstream ifs(path);
        if (!ifs.is_open())
            return AppConfig{};

        json j;
        try
        {
            ifs >> j;
        }
        catch (...)
        {
            return AppConfig{};
        }

        AppConfig cfg;

        if (j.contains("camera"))
        {
            auto &c = j["camera"];
            if (c.contains("camera_index"))
                cfg.camera.camera_index = c["camera_index"];
            if (c.contains("frame_width"))
                cfg.camera.frame_width = c["frame_width"];
            if (c.contains("frame_height"))
                cfg.camera.frame_height = c["frame_height"];
            if (c.contains("target_fps"))
                cfg.camera.target_fps = c["target_fps"];
        }

        if (j.contains("display"))
        {
            auto &d = j["display"];
            if (d.contains("projector_screen_index"))
                cfg.display.projector_screen_index = d["projector_screen_index"];
            if (d.contains("show_debug_window"))
                cfg.display.show_debug_window = d["show_debug_window"];
            if (d.contains("show_player_markers"))
                cfg.display.show_player_markers = d["show_player_markers"];
        }

        if (j.contains("pose"))
        {
            auto &p = j["pose"];
            if (p.contains("model_asset_path"))
            {
                cfg.pose.model_asset_path = p["model_asset_path"];
                // Migrate legacy model filenames to the current default
                static constexpr std::string_view kLegacyModels[] = {
                    "pose_landmarker_lite.onnx",
                    "pose_landmarker_full.onnx",
                    "yolov8n-pose.onnx",
                };
                const std::filesystem::path loaded(cfg.pose.model_asset_path);
                for (auto legacy : kLegacyModels)
                {
                    if (loaded.filename().string() == legacy)
                    {
                        cfg.pose.model_asset_path = PoseSettings{}.model_asset_path;
                        break;
                    }
                }
            }
            if (p.contains("num_poses"))
                cfg.pose.num_poses = p["num_poses"];
            if (p.contains("min_pose_detection_confidence"))
                cfg.pose.min_pose_detection_confidence = p["min_pose_detection_confidence"];
            if (p.contains("min_pose_presence_confidence"))
                cfg.pose.min_pose_presence_confidence = p["min_pose_presence_confidence"];
            if (p.contains("min_tracking_confidence"))
                cfg.pose.min_tracking_confidence = p["min_tracking_confidence"];
            if (p.contains("min_landmark_visibility"))
                cfg.pose.min_landmark_visibility = p["min_landmark_visibility"];
            if (p.contains("use_gpu_if_available"))
                cfg.pose.use_gpu_if_available = p["use_gpu_if_available"];
        }

        if (j.contains("grid"))
        {
            auto &g = j["grid"];
            if (g.contains("rows"))
                cfg.grid.rows = g["rows"];
            if (g.contains("columns"))
                cfg.grid.columns = g["columns"];
            if (g.contains("playable_width"))
                cfg.grid.playable_width = g["playable_width"];
            if (g.contains("playable_height"))
                cfg.grid.playable_height = g["playable_height"];
            if (g.contains("playable_inset"))
                cfg.grid.playable_inset = g["playable_inset"];
        }

        if (j.contains("timings"))
        {
            auto &t = j["timings"];
            if (t.contains("flashing_duration_seconds"))
                cfg.timings.flashing_duration_seconds = t["flashing_duration_seconds"];
            if (t.contains("flash_interval_seconds"))
                cfg.timings.flash_interval_seconds = t["flash_interval_seconds"];
            if (t.contains("reaction_window_seconds"))
                cfg.timings.reaction_window_seconds = t["reaction_window_seconds"];
            if (t.contains("lock_delay_seconds"))
                cfg.timings.lock_delay_seconds = t["lock_delay_seconds"];
            if (t.contains("elimination_display_seconds"))
                cfg.timings.elimination_display_seconds = t["elimination_display_seconds"];
            if (t.contains("inter_round_delay_seconds"))
                cfg.timings.inter_round_delay_seconds = t["inter_round_delay_seconds"];
            if (t.contains("missed_detection_grace_seconds"))
                cfg.timings.missed_detection_grace_seconds = t["missed_detection_grace_seconds"];
        }

        if (j.contains("aruco_dictionary"))
            cfg.aruco_dictionary = j["aruco_dictionary"];

        if (j.contains("marker_ids"))
            cfg.marker_ids = j["marker_ids"].get<std::vector<int>>();

        return cfg;
    }

    // ── Save ─────────────────────────────────────────────────────────────

    void save_config(const AppConfig &cfg)
    {
        auto dir = config_dir();
        fs::create_directories(dir);
        auto path = config_path();

        json j;

        j["camera"] = {
            {"camera_index", cfg.camera.camera_index},
            {"frame_width", cfg.camera.frame_width},
            {"frame_height", cfg.camera.frame_height},
            {"target_fps", cfg.camera.target_fps},
        };

        j["display"] = {
            {"projector_screen_index", cfg.display.projector_screen_index},
            {"show_debug_window", cfg.display.show_debug_window},
            {"show_player_markers", cfg.display.show_player_markers},
        };

        j["pose"] = {
            {"model_asset_path", cfg.pose.model_asset_path},
            {"num_poses", cfg.pose.num_poses},
            {"min_pose_detection_confidence", cfg.pose.min_pose_detection_confidence},
            {"min_pose_presence_confidence", cfg.pose.min_pose_presence_confidence},
            {"min_tracking_confidence", cfg.pose.min_tracking_confidence},
            {"min_landmark_visibility", cfg.pose.min_landmark_visibility},
            {"use_gpu_if_available", cfg.pose.use_gpu_if_available},
        };

        j["grid"] = {
            {"rows", cfg.grid.rows},
            {"columns", cfg.grid.columns},
            {"playable_width", cfg.grid.playable_width},
            {"playable_height", cfg.grid.playable_height},
            {"playable_inset", cfg.grid.playable_inset},
        };

        j["timings"] = {
            {"flashing_duration_seconds", cfg.timings.flashing_duration_seconds},
            {"flash_interval_seconds", cfg.timings.flash_interval_seconds},
            {"reaction_window_seconds", cfg.timings.reaction_window_seconds},
            {"lock_delay_seconds", cfg.timings.lock_delay_seconds},
            {"elimination_display_seconds", cfg.timings.elimination_display_seconds},
            {"inter_round_delay_seconds", cfg.timings.inter_round_delay_seconds},
            {"missed_detection_grace_seconds", cfg.timings.missed_detection_grace_seconds},
        };

        j["aruco_dictionary"] = cfg.aruco_dictionary;
        j["marker_ids"] = cfg.marker_ids;

        std::ofstream ofs(path);
        ofs << j.dump(2);
    }

} // namespace sag
