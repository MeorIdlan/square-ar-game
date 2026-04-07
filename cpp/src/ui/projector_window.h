#pragma once
// Projector window — fullscreen D3D11 window on a selected monitor

#ifdef _WIN32

#include "d3d_window.h"
#include "../services/overlay_render_service.h"
#include "../models/contracts.h"
#include "../models/calibration_model.h"

namespace sag::ui {

class ProjectorWindow : public D3DWindow {
public:
    ProjectorWindow() = default;

    // Create on a specific monitor (0-based index)
    bool create_on_monitor(int monitor_index, HINSTANCE hInstance);

    // Move to a different monitor
    void set_target_monitor(int monitor_index);

    // Render the overlay
    void render_frame(OverlayRenderService& overlay_service,
                      const RenderState& state,
                      const FramePacket* frame_packet,
                      const CalibrationModel* calibration);

protected:
    LRESULT window_proc(UINT msg, WPARAM wParam, LPARAM lParam) override;
};

} // namespace sag::ui

#endif // _WIN32
