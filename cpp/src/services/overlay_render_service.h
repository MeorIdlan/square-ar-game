#pragma once
// Overlay render service — draws camera feed + grid overlay + player markers + HUD
// using D3D11 through the Renderer2D helper.

#ifdef _WIN32

#include "../models/contracts.h"
#include "../models/calibration_model.h"
#include "../models/enums.h"
#include "../utils/d3d_helpers.h"
#include "../utils/d3d_renderer.h"
#include <opencv2/core.hpp>
#include <optional>
#include <string>

namespace sag
{

    class OverlayRenderService
    {
    public:
        void render(d3d::D3DContext &ctx,
                    d3d::Renderer2D &renderer,
                    const RenderState &state,
                    const FramePacket *frame_packet,
                    const CalibrationModel *calibration);

    private:
        void draw_camera_background(d3d::D3DContext &ctx,
                                    d3d::Renderer2D &renderer,
                                    const FramePacket *frame_packet,
                                    float width, float height);

        void draw_grid_overlay(d3d::Renderer2D &renderer,
                               const RenderState &state,
                               const CalibrationModel *calibration,
                               float scale_x, float scale_y);

        void draw_player_markers(d3d::Renderer2D &renderer,
                                 const RenderState &state,
                                 const CalibrationModel *calibration,
                                 float scale_x, float scale_y);

        void draw_floor_point(d3d::Renderer2D &renderer,
                              const cv::Mat &inverse_homography,
                              const std::optional<Point> &point,
                              d3d::Color4 color, float radius,
                              float scale_x, float scale_y);

        void draw_hud(d3d::Renderer2D &renderer,
                      const RenderState &state,
                      float width, float height);

        std::array<DirectX::XMFLOAT2, 4> project_floor_cell(
            const cv::Mat &inverse_homography,
            float x, float y, float w, float h,
            float scale_x, float scale_y) const;

        static d3d::Color4 cell_fill_color(CellState state);
    };

} // namespace sag

#endif // _WIN32
