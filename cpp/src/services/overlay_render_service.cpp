#ifdef _WIN32

#include "overlay_render_service.h"
#include "../utils/constants.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <format>

namespace sag
{

    void OverlayRenderService::render(
        d3d::D3DContext &ctx,
        d3d::Renderer2D &renderer,
        const RenderState &state,
        const FramePacket *frame_packet,
        const CalibrationModel *calibration)
    {
        auto width = static_cast<float>(ctx.width);
        auto height = static_cast<float>(ctx.height);

        renderer.begin_frame(width, height);

        // Compute scale from camera-pixel space to viewport space so the AR overlay
        // aligns with the camera background regardless of camera vs monitor resolution.
        float frame_w = width;
        float frame_h = height;
        if (frame_packet && !frame_packet->frame.empty())
        {
            frame_w = static_cast<float>(frame_packet->frame.cols);
            frame_h = static_cast<float>(frame_packet->frame.rows);
        }
        const float scale_x = (frame_w > 0.0f) ? (width / frame_w) : 1.0f;
        const float scale_y = (frame_h > 0.0f) ? (height / frame_h) : 1.0f;

        draw_camera_background(ctx, renderer, frame_packet, width, height);
        draw_grid_overlay(renderer, state, calibration, scale_x, scale_y);
        draw_player_markers(renderer, state, calibration, scale_x, scale_y);
        draw_hud(renderer, state, width, height);

        renderer.end_frame();
    }

    void OverlayRenderService::draw_camera_background(
        d3d::D3DContext &ctx,
        d3d::Renderer2D &renderer,
        const FramePacket *frame_packet,
        float width, float height)
    {
        if (!frame_packet || frame_packet->frame.empty())
        {
            // Black background
            renderer.draw_filled_rect(0, 0, width, height, {0, 0, 0, 1});
            return;
        }

        auto srv = d3d::upload_texture(ctx.device.Get(), ctx.context.Get(), frame_packet->frame);
        if (srv)
        {
            renderer.draw_texture(srv.Get(), 0, 0, width, height);
        }
    }

    void OverlayRenderService::draw_grid_overlay(
        d3d::Renderer2D &renderer,
        const RenderState &state,
        const CalibrationModel *calibration,
        float scale_x, float scale_y)
    {
        if (!calibration || calibration->homography.empty() || !calibration->playable_bounds)
            return;

        if (calibration->inverse_homography.empty())
            return;
        const auto &inv_h = calibration->inverse_homography;

        if (state.grid_cells.empty())
            return;

        int rows = 0, cols = 0;
        for (const auto &[idx, _] : state.grid_cells)
        {
            rows = std::max(rows, idx.row + 1);
            cols = std::max(cols, idx.col + 1);
        }
        if (rows <= 0 || cols <= 0)
            return;

        const auto &bounds = *calibration->playable_bounds;
        float min_x = bounds[0].x, max_x = bounds[0].x;
        float min_y = bounds[0].y, max_y = bounds[0].y;
        for (const auto &v : bounds)
        {
            min_x = std::min(min_x, v.x);
            max_x = std::max(max_x, v.x);
            min_y = std::min(min_y, v.y);
            max_y = std::max(max_y, v.y);
        }

        float cell_w = (max_x - min_x) / static_cast<float>(cols);
        float cell_h = (max_y - min_y) / static_cast<float>(rows);

        for (const auto &[idx, cell_state] : state.grid_cells)
        {
            float fx = min_x + static_cast<float>(idx.col) * cell_w;
            float fy = min_y + static_cast<float>(idx.row) * cell_h;

            auto corners = project_floor_cell(inv_h, fx, fy, cell_w, cell_h, scale_x, scale_y);
            auto fill = cell_fill_color(cell_state);
            renderer.draw_filled_quad(corners, fill);
        }
    }

    void OverlayRenderService::draw_player_markers(
        d3d::Renderer2D &renderer,
        const RenderState &state,
        const CalibrationModel *calibration,
        float scale_x, float scale_y)
    {
        if (!calibration || calibration->homography.empty())
            return;
        if (calibration->inverse_homography.empty())
            return;
        const auto &inv_h = calibration->inverse_homography;

        for (const auto &player : state.players)
        {
            draw_floor_point(renderer, inv_h, player.left_foot,
                             {1.0f, 0.82f, 0.4f, 1.0f}, 8.0f, scale_x, scale_y);
            draw_floor_point(renderer, inv_h, player.right_foot,
                             {0.94f, 0.28f, 0.44f, 1.0f}, 8.0f, scale_x, scale_y);
            draw_floor_point(renderer, inv_h, player.standing_point,
                             {0.0f, 0.82f, 1.0f, 1.0f}, 11.0f, scale_x, scale_y);
        }
    }

    void OverlayRenderService::draw_floor_point(
        d3d::Renderer2D &renderer,
        const cv::Mat &inverse_homography,
        const std::optional<Point> &point,
        d3d::Color4 color, float radius,
        float scale_x, float scale_y)
    {
        if (!point)
            return;

        std::vector<cv::Point2f> src = {{point->x, point->y}};
        std::vector<cv::Point2f> dst;
        cv::perspectiveTransform(src, dst, inverse_homography);

        // Scale both position and radius from camera-pixel space to viewport space
        const float avg_scale = (scale_x + scale_y) * 0.5f;
        renderer.draw_circle(dst[0].x * scale_x, dst[0].y * scale_y, radius * avg_scale, color);
    }

    void OverlayRenderService::draw_hud(
        d3d::Renderer2D &renderer,
        const RenderState &state,
        float width, float height)
    {
        constexpr d3d::Color4 white = {1, 1, 1, 1};
        constexpr d3d::Color4 yellow = {1.0f, 0.82f, 0.4f, 1.0f};

        float y = 16.0f;
        constexpr float line_h = 28.0f;

        renderer.draw_outlined_text(24, y, L"Status: " + std::wstring(state.status_text.begin(), state.status_text.end()), white);
        y += line_h;
        auto phase_str = std::string(to_string(state.phase));
        renderer.draw_outlined_text(24, y, L"Phase: " + std::wstring(phase_str.begin(), phase_str.end()), white);
        y += line_h;
        renderer.draw_outlined_text(24, y, L"Timer: " + std::wstring(state.timer_text.begin(), state.timer_text.end()), white);
        y += line_h;
        renderer.draw_outlined_text(24, y, L"Camera: " + std::wstring(state.camera_status_text.begin(), state.camera_status_text.end()), white);
        y += line_h;
        renderer.draw_outlined_text(24, y, L"Pose: " + std::wstring(state.pose_status_text.begin(), state.pose_status_text.end()), white);
        y += line_h;
        renderer.draw_outlined_text(24, y, L"Display: " + std::wstring(state.display_status_text.begin(), state.display_status_text.end()), white);
        y += line_h;
        renderer.draw_outlined_text(24, y, L"Calibration: " + std::wstring(state.calibration_status_text.begin(), state.calibration_status_text.end()), yellow);

        // Player list at bottom
        float py = height - 24.0f;
        for (const auto &player : state.players)
        {
            auto label = std::wstring(player.player_id.begin(), player.player_id.end()) + L": " + std::wstring(player.status_text.begin(), player.status_text.end());
            renderer.draw_outlined_text(24, py, label, white);
            py -= 20.0f;
        }
    }

    std::array<DirectX::XMFLOAT2, 4> OverlayRenderService::project_floor_cell(
        const cv::Mat &inverse_homography,
        float x, float y, float w, float h,
        float scale_x, float scale_y) const
    {
        std::vector<cv::Point2f> floor_pts = {
            {x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}};
        std::vector<cv::Point2f> img_pts;
        cv::perspectiveTransform(floor_pts, img_pts, inverse_homography);

        // Scale from camera-pixel space to viewport space
        return {{{img_pts[0].x * scale_x, img_pts[0].y * scale_y},
                 {img_pts[1].x * scale_x, img_pts[1].y * scale_y},
                 {img_pts[2].x * scale_x, img_pts[2].y * scale_y},
                 {img_pts[3].x * scale_x, img_pts[3].y * scale_y}}};
    }

    d3d::Color4 OverlayRenderService::cell_fill_color(CellState state)
    {
        switch (state)
        {
        case CellState::Green:
            return {46 / 255.f, 204 / 255.f, 113 / 255.f, 110 / 255.f};
        case CellState::Red:
            return {231 / 255.f, 76 / 255.f, 60 / 255.f, 110 / 255.f};
        case CellState::Flashing:
            return {constants::FLASHING_CELL_COLOR_R / 255.f,
                    constants::FLASHING_CELL_COLOR_G / 255.f,
                    constants::FLASHING_CELL_COLOR_B / 255.f,
                    constants::FLASHING_CELL_ALPHA / 255.f};
        default:
            return {127 / 255.f, 140 / 255.f, 141 / 255.f, 70 / 255.f};
        }
    }

} // namespace sag

#endif // _WIN32
