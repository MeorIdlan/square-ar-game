#ifdef _WIN32

#include "debug_window.h"
#include "../utils/constants.h"

namespace sag::ui {

bool DebugWindow::create_default(HINSTANCE hInstance)
{
    return create(L"Debug View",
                  CW_USEDEFAULT, CW_USEDEFAULT,
                  constants::DEBUG_CANVAS_WIDTH, constants::DEBUG_CANVAS_HEIGHT,
                  hInstance, false);
}

void DebugWindow::render_frame(DebugRenderService& debug_service,
                               const RenderState& state)
{
    clear(0.067f, 0.067f, 0.067f, 1.0f);
    debug_service.render(d3d_ctx(), renderer(), state);
    present();
}

} // namespace sag::ui

#endif // _WIN32
