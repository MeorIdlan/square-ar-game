#ifdef _WIN32

#include "projector_window.h"
#include <vector>

namespace sag::ui {

struct MonitorInfo {
    HMONITOR handle;
    RECT     rect;
};

static BOOL CALLBACK enum_monitors_callback(HMONITOR hMonitor, HDC, LPRECT lpRect, LPARAM lParam)
{
    auto* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(lParam);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);
    monitors->push_back({hMonitor, mi.rcMonitor});
    return TRUE;
}

static std::vector<MonitorInfo> enumerate_monitors()
{
    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(nullptr, nullptr, enum_monitors_callback,
                        reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}

bool ProjectorWindow::create_on_monitor(int monitor_index, HINSTANCE hInstance)
{
    auto monitors = enumerate_monitors();
    if (monitors.empty()) return false;

    int idx = std::clamp(monitor_index, 0, static_cast<int>(monitors.size()) - 1);
    const auto& rect = monitors[idx].rect;
    int w = rect.right  - rect.left;
    int h = rect.bottom - rect.top;

    if (!create(L"Projector View", rect.left, rect.top, w, h, hInstance, true))
        return false;

    // Go borderless fullscreen on the target monitor
    SetWindowPos(hwnd(), HWND_TOP, rect.left, rect.top, w, h, SWP_FRAMECHANGED);
    return true;
}

void ProjectorWindow::set_target_monitor(int monitor_index)
{
    auto monitors = enumerate_monitors();
    if (monitors.empty()) return;

    int idx = std::clamp(monitor_index, 0, static_cast<int>(monitors.size()) - 1);
    const auto& rect = monitors[idx].rect;
    int w = rect.right  - rect.left;
    int h = rect.bottom - rect.top;

    SetWindowPos(hwnd(), HWND_TOP, rect.left, rect.top, w, h, SWP_FRAMECHANGED);
    handle_resize(w, h);
}

void ProjectorWindow::render_frame(
    OverlayRenderService& overlay_service,
    const RenderState& state,
    const FramePacket* frame_packet,
    const CalibrationModel* calibration)
{
    clear(0, 0, 0, 1);
    overlay_service.render(d3d_ctx(), renderer(), state, frame_packet, calibration);
    present();
}

LRESULT ProjectorWindow::window_proc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                hide();
                return 0;
            }
            break;
    }
    return D3DWindow::window_proc(msg, wParam, lParam);
}

} // namespace sag::ui

#endif // _WIN32
