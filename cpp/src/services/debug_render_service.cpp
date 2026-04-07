#ifdef _WIN32

#include "debug_render_service.h"
#include <algorithm>
#include <string>

namespace sag {

void DebugRenderService::render(
    d3d::D3DContext& ctx,
    d3d::Renderer2D& renderer,
    const RenderState& state)
{
    auto width  = static_cast<float>(ctx.width);
    auto height = static_cast<float>(ctx.height);

    renderer.begin_frame(width, height);

    // Background
    renderer.draw_filled_rect(0, 0, width, height, {0.067f, 0.067f, 0.067f, 1.0f}); // #111111

    constexpr float margin = 30.0f;
    float grid_w = width  - margin * 2;
    float grid_h = height - margin * 2;

    int rows = 0, cols = 0;
    for (const auto& [idx, _] : state.grid_cells) {
        rows = std::max(rows, idx.row + 1);
        cols = std::max(cols, idx.col + 1);
    }
    rows = std::max(rows, 1);
    cols = std::max(cols, 1);

    float cell_w = grid_w / static_cast<float>(cols);
    float cell_h = grid_h / static_cast<float>(rows);

    // Draw cells
    for (const auto& [idx, cell_state] : state.grid_cells) {
        auto color = cell_debug_color(cell_state);
        float cx = margin + static_cast<float>(idx.col) * cell_w;
        float cy = margin + static_cast<float>(idx.row) * cell_h;
        renderer.draw_filled_rect(cx, cy, cell_w - 2, cell_h - 2, color);
    }

    // Draw player markers
    constexpr d3d::Color4 player_color = {0.0f, 0.75f, 1.0f, 1.0f}; // #00bfff
    constexpr d3d::Color4 white = {1, 1, 1, 1};

    for (const auto& player : state.players) {
        if (!player.standing_point) continue;

        float px = margin + player.standing_point->x / static_cast<float>(std::max(cols, 1)) * grid_w;
        float py = margin + player.standing_point->y / static_cast<float>(std::max(rows, 1)) * grid_h;

        renderer.draw_circle(px, py, 4.0f, player_color);

        std::string cell_str = "-";
        if (player.occupied_cell) {
            cell_str = std::to_string(player.occupied_cell->row) + ","
                     + std::to_string(player.occupied_cell->col);
        }
        std::string label = player.player_id + " [" + cell_str + "] " + player.status_text;
        std::wstring wlabel(label.begin(), label.end());
        renderer.draw_text(px + 8, py - 12, wlabel, white);
    }

    // HUD text
    float y = 4.0f;
    constexpr float line_h = 22.0f;

    auto to_wide = [](const std::string& s) { return std::wstring(s.begin(), s.end()); };

    renderer.draw_text(16, y, to_wide(state.status_text), white);
    y += line_h;
    renderer.draw_text(16, y, L"Phase: " + to_wide(std::string(to_string(state.phase))), white);
    y += line_h;
    renderer.draw_text(16, y, L"Timer: " + to_wide(state.timer_text), white);
    y += line_h;
    renderer.draw_text(16, y, L"Camera: " + to_wide(state.camera_status_text), white);
    y += line_h;
    renderer.draw_text(16, y, L"Pose: " + to_wide(state.pose_status_text), white);
    y += line_h;
    renderer.draw_text(16, y, L"Display: " + to_wide(state.display_status_text), white);
    y += line_h;
    renderer.draw_text(16, y, L"Calibration: " + to_wide(state.calibration_status_text), white);

    renderer.end_frame();
}

d3d::Color4 DebugRenderService::cell_debug_color(CellState state)
{
    switch (state) {
        case CellState::GREEN:    return {0.18f, 0.545f, 0.341f, 1.0f}; // #2e8b57
        case CellState::RED:      return {0.647f, 0.165f, 0.165f, 1.0f}; // #a52a2a
        case CellState::FLASHING: return {0.722f, 0.525f, 0.043f, 1.0f}; // #b8860b
        default:                  return {0.251f, 0.251f, 0.251f, 1.0f}; // #404040
    }
}

} // namespace sag

#endif // _WIN32
