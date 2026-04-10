#pragma once
// Operator window — Dear ImGui control panel matching the Python MainWindow layout.
// Runs in its own Win32+D3D11 window with ImGui context.

#ifdef _WIN32

#include "d3d_window.h"
#include "../models/game_session_model.h"
#include "../utils/config.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <functional>

namespace sag::ui
{

    // Callbacks from operator UI to the application
    struct OperatorCallbacks
    {
        std::function<void()> calibrate;
        std::function<void()> reset_calibration;
        std::function<void()> start_round;
        std::function<void()> lock_round;
        std::function<void()> force_next_round;
        std::function<void()> reset_session;
        std::function<void()> save_settings;
        std::function<void()> reconnect_camera;
        std::function<void(int)> set_camera_index;
        std::function<void(const CameraProfile &)> set_camera_format;
        std::function<void(int)> set_projector_monitor;
        std::function<void(const std::string &)> set_aruco_dictionary;
        std::function<void(const std::string &)> set_pose_model;
        std::function<void(int)> set_grid_rows;
        std::function<void(int)> set_grid_columns;
        std::function<void(const std::string &, float)> set_timing;
        std::function<void(const std::string &)> revive_player;
        std::function<void(const std::string &)> eliminate_player;
        std::function<void(const std::string &)> remove_player;
    };

    class OperatorWindow
    {
    public:
        OperatorWindow() = default;
        ~OperatorWindow();

        bool create(HINSTANCE hInstance, AppConfig &config, OperatorCallbacks callbacks);
        void destroy();

        // Call each frame from the message loop
        void begin_frame();
        void render(const GameSessionModel &session);
        void end_frame();

        // Update enumerated camera device names (index = position in list)
        void set_camera_devices(std::vector<std::string> device_names);

        // Update available formats for the currently selected camera
        void set_camera_formats(std::vector<CameraProfile> formats);

        // Forward Win32 messages to ImGui
        LRESULT handle_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        HWND hwnd() const { return hwnd_; }
        bool wants_close() const { return wants_close_; }

    private:
        void draw_controls(const GameSessionModel &session);
        void draw_timing_controls();
        void draw_overrides(const GameSessionModel &session);
        void draw_log();

        HWND hwnd_ = nullptr;
        d3d::D3DContext d3d_ctx_;
        bool imgui_initialized_ = false;
        bool wants_close_ = false;

        AppConfig *config_ = nullptr;
        OperatorCallbacks callbacks_;
        std::vector<std::string> log_messages_;
        int selected_player_index_ = 0;

        // Camera device / format lists populated by Application
        std::vector<std::string> camera_device_names_;
        std::vector<CameraProfile> camera_formats_;
        int selected_device_index_ = 0;
        int selected_format_index_ = 0;

        static LRESULT CALLBACK static_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    };

} // namespace sag::ui

#endif // _WIN32
