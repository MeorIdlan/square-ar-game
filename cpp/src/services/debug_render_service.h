#pragma once
// Debug render service — draws a top-down grid view with player positions.
// Uses D3D11 through the Renderer2D helper.

#ifdef _WIN32

#include "../models/contracts.h"
#include "../models/enums.h"
#include "../utils/d3d_helpers.h"
#include "../utils/d3d_renderer.h"

namespace sag {

class DebugRenderService {
public:
    void render(d3d::D3DContext& ctx,
                d3d::Renderer2D& renderer,
                const RenderState& state);

private:
    static d3d::Color4 cell_debug_color(CellState state);
};

} // namespace sag

#endif // _WIN32
