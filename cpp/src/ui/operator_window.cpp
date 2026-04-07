#ifdef _WIN32

#include "operator_window.h"
#include "../utils/constants.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <algorithm>
#include <format>

// Forward declare the ImGui Win32 handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace sag::ui {

LRESULT CALLBACK OperatorWindow::static_wnd_proc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    OperatorWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = static_cast<OperatorWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<OperatorWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->handle_message(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

OperatorWindow::~OperatorWindow()
{
    destroy();
}

bool OperatorWindow::create(HINSTANCE hInstance, AppConfig& config, OperatorCallbacks callbacks)
{
    config_    = &config;
    callbacks_ = std::move(callbacks);

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = static_wnd_proc;
    wc.hInstance      = hInstance;
    wc.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName  = L"SAG_Operator";
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(
        0, L"SAG_Operator", L"Square AR Game Control",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 760, 640,
        nullptr, nullptr, hInstance, this);

    if (!hwnd_) return false;

    if (!d3d::create_device_and_swapchain(hwnd_, 760, 640, d3d_ctx_))
        return false;

    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd_);
    ImGui_ImplDX11_Init(d3d_ctx_.device.Get(), d3d_ctx_.context.Get());
    imgui_initialized_ = true;

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);

    return true;
}

void OperatorWindow::destroy()
{
    if (imgui_initialized_) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imgui_initialized_ = false;
    }
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

LRESULT OperatorWindow::handle_message(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (imgui_initialized_) {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            return 1;
    }

    switch (msg) {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                d3d::resize_buffers(d3d_ctx_, LOWORD(lParam), HIWORD(lParam));
            }
            return 0;

        case WM_CLOSE:
            wants_close_ = true;
            return 0;

        case WM_DESTROY:
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void OperatorWindow::begin_frame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void OperatorWindow::render(const GameSessionModel& session)
{
    // Main operator panel in a full-window ImGui window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Operator Panel", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTable("MainLayout", 2, ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthFixed, 340.0f);
        ImGui::TableSetupColumn("Log");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        draw_controls(session);
        draw_timing_controls();
        draw_overrides(session);

        ImGui::TableNextColumn();
        draw_log();

        ImGui::EndTable();
    }

    ImGui::End();
}

void OperatorWindow::end_frame()
{
    ImGui::Render();

    float clear_color[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    d3d_ctx_.context->OMSetRenderTargets(1, d3d_ctx_.rtv.GetAddressOf(), nullptr);
    d3d_ctx_.context->ClearRenderTargetView(d3d_ctx_.rtv.Get(), clear_color);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    d3d_ctx_.swapchain->Present(1, 0);
}

void OperatorWindow::draw_controls(const GameSessionModel& session)
{
    if (ImGui::CollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Grid settings
        int rows = config_->grid.rows;
        if (ImGui::SliderInt("Rows", &rows, 2, 12)) {
            config_->grid.rows = rows;
            if (callbacks_.set_grid_rows) callbacks_.set_grid_rows(rows);
        }

        int cols = config_->grid.columns;
        if (ImGui::SliderInt("Columns", &cols, 2, 12)) {
            config_->grid.columns = cols;
            if (callbacks_.set_grid_columns) callbacks_.set_grid_columns(cols);
        }

        ImGui::Separator();

        // Camera
        int cam_idx = config_->camera.camera_index;
        if (ImGui::InputInt("Camera Index", &cam_idx)) {
            cam_idx = std::max(0, cam_idx);
            config_->camera.camera_index = cam_idx;
            if (callbacks_.set_camera_index) callbacks_.set_camera_index(cam_idx);
        }

        if (ImGui::Button("Reconnect Camera")) {
            if (callbacks_.reconnect_camera) callbacks_.reconnect_camera();
        }

        // Projector monitor
        int monitor = config_->display.projector_screen_index;
        if (ImGui::InputInt("Projector Monitor", &monitor)) {
            monitor = std::max(0, monitor);
            config_->display.projector_screen_index = monitor;
            if (callbacks_.set_projector_monitor) callbacks_.set_projector_monitor(monitor);
        }

        // ArUco dictionary
        static const char* aruco_dicts[] = {
            "DICT_4X4_50", "DICT_4X4_100", "DICT_4X4_250", "DICT_4X4_1000",
            "DICT_5X5_50", "DICT_5X5_100", "DICT_5X5_250", "DICT_5X5_1000",
            "DICT_6X6_50", "DICT_6X6_100", "DICT_6X6_250", "DICT_6X6_1000",
            "DICT_7X7_50", "DICT_7X7_100", "DICT_7X7_250", "DICT_7X7_1000",
        };
        int aruco_idx = 0;
        for (int i = 0; i < IM_ARRAYSIZE(aruco_dicts); ++i) {
            if (config_->aruco_dictionary == aruco_dicts[i]) { aruco_idx = i; break; }
        }
        if (ImGui::Combo("ArUco Dictionary", &aruco_idx, aruco_dicts, IM_ARRAYSIZE(aruco_dicts))) {
            config_->aruco_dictionary = aruco_dicts[aruco_idx];
            if (callbacks_.set_aruco_dictionary) callbacks_.set_aruco_dictionary(aruco_dicts[aruco_idx]);
        }

        ImGui::Separator();

        // Action buttons
        if (ImGui::Button("Calibrate")) {
            if (callbacks_.calibrate) callbacks_.calibrate();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Calibration")) {
            if (callbacks_.reset_calibration) callbacks_.reset_calibration();
        }

        if (ImGui::Button("Start Round")) {
            if (callbacks_.start_round) callbacks_.start_round();
        }
        ImGui::SameLine();
        if (ImGui::Button("Lock Round")) {
            if (callbacks_.lock_round) callbacks_.lock_round();
        }
        ImGui::SameLine();
        if (ImGui::Button("Force Next Round")) {
            if (callbacks_.force_next_round) callbacks_.force_next_round();
        }

        if (ImGui::Button("Reset Session")) {
            if (callbacks_.reset_session) callbacks_.reset_session();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Settings")) {
            if (callbacks_.save_settings) callbacks_.save_settings();
        }

        ImGui::Separator();

        // Status display
        ImGui::Text("Status: %s", session.status_message.c_str());
        ImGui::Text("Phase: %s", to_string(session.round_state.phase).data());
        ImGui::Text("Timer: %.1f", session.round_state.timer_remaining);
        ImGui::Text("Calibration: %s", to_string(session.calibration.state).data());
        ImGui::TextWrapped("Camera: %s", session.camera_status_message.c_str());
        ImGui::TextWrapped("Pose: %s", session.pose_status_message.c_str());
        ImGui::TextWrapped("Display: %s", session.display_status_message.c_str());

        // Calibration details
        auto detected = session.calibration.detected_marker_ids();
        auto missing  = session.calibration.missing_marker_ids();
        std::string visible_str;
        for (int id : detected) {
            if (!visible_str.empty()) visible_str += ", ";
            visible_str += std::to_string(id);
        }
        if (visible_str.empty()) visible_str = "none";

        std::string missing_str;
        for (int id : missing) {
            if (!missing_str.empty()) missing_str += ", ";
            missing_str += std::to_string(id);
        }
        if (missing_str.empty()) missing_str = "none";

        ImGui::Text("Markers: %d visible | visible: %s | missing: %s",
                     static_cast<int>(detected.size()), visible_str.c_str(), missing_str.c_str());
        ImGui::TextWrapped("Diagnostics: %s", session.calibration.validation_message.c_str());
    }
}

void OperatorWindow::draw_timing_controls()
{
    if (ImGui::CollapsingHeader("Timing", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto timing_slider = [this](const char* label, const char* field, float& value) {
            if (ImGui::SliderFloat(label, &value, 0.05f, 30.0f, "%.2f")) {
                if (callbacks_.set_timing) callbacks_.set_timing(field, value);
            }
        };

        timing_slider("Flashing",       "flashing_duration_seconds",      config_->timings.flashing_duration_seconds);
        timing_slider("Flash Interval", "flash_interval_seconds",         config_->timings.flash_interval_seconds);
        timing_slider("Reaction",       "reaction_window_seconds",        config_->timings.reaction_window_seconds);
        timing_slider("Check Delay",    "lock_delay_seconds",             config_->timings.lock_delay_seconds);
        timing_slider("Results",        "elimination_display_seconds",    config_->timings.elimination_display_seconds);
        timing_slider("Inter-Round",    "inter_round_delay_seconds",      config_->timings.inter_round_delay_seconds);
        timing_slider("Missing Grace",  "missed_detection_grace_seconds", config_->timings.missed_detection_grace_seconds);
    }
}

void OperatorWindow::draw_overrides(const GameSessionModel& session)
{
    if (ImGui::CollapsingHeader("Player Overrides")) {
        // Build player list
        std::vector<std::string> player_ids;
        for (const auto& [id, _] : session.players) {
            player_ids.push_back(id);
        }
        std::sort(player_ids.begin(), player_ids.end());

        if (!player_ids.empty()) {
            selected_player_index_ = std::clamp(selected_player_index_, 0,
                                                 static_cast<int>(player_ids.size()) - 1);
            if (ImGui::BeginCombo("Player", player_ids[selected_player_index_].c_str())) {
                for (int i = 0; i < static_cast<int>(player_ids.size()); ++i) {
                    bool selected = (i == selected_player_index_);
                    if (ImGui::Selectable(player_ids[i].c_str(), selected)) {
                        selected_player_index_ = i;
                    }
                }
                ImGui::EndCombo();
            }

            const auto& pid = player_ids[selected_player_index_];
            if (ImGui::Button("Revive")) {
                if (callbacks_.revive_player) callbacks_.revive_player(pid);
            }
            ImGui::SameLine();
            if (ImGui::Button("Eliminate")) {
                if (callbacks_.eliminate_player) callbacks_.eliminate_player(pid);
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove")) {
                if (callbacks_.remove_player) callbacks_.remove_player(pid);
            }
        } else {
            ImGui::TextDisabled("No players tracked");
        }
    }
}

void OperatorWindow::draw_log()
{
    ImGui::Text("Event Log");
    ImGui::Separator();

    ImGui::BeginChild("LogRegion", ImVec2(0, 0), ImGuiChildFlags_None,
                       ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& msg : log_messages_) {
        ImGui::TextUnformatted(msg.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}

} // namespace sag::ui

#endif // _WIN32
