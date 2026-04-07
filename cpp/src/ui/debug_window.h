#pragma once
// Debug window — shows the top-down grid view in a resizable window

#ifdef _WIN32

#include "d3d_window.h"
#include "../services/debug_render_service.h"
#include "../models/contracts.h"

namespace sag::ui {

class DebugWindow : public D3DWindow {
public:
    DebugWindow() = default;

    bool create_default(HINSTANCE hInstance);

    void render_frame(DebugRenderService& debug_service,
                      const RenderState& state);
};

} // namespace sag::ui

#endif // _WIN32
