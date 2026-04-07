#pragma once
// 2D rendering primitives using DirectXTK (SpriteBatch, PrimitiveBatch, SpriteFont)

#ifdef _WIN32

#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <Effects.h>
#include <CommonStates.h>
#include <memory>
#include <string>
#include <array>

namespace sag::d3d {

using Microsoft::WRL::ComPtr;
using DirectX::XMFLOAT2;
using DirectX::XMFLOAT4;

struct Color4 {
    float r, g, b, a;
    DirectX::XMVECTORF32 to_xm() const { return {{{r, g, b, a}}}; }
};

class Renderer2D {
public:
    bool initialize(ID3D11Device* device, ID3D11DeviceContext* context);

    void begin_frame(float viewport_width, float viewport_height);
    void end_frame();

    // Primitives
    void draw_filled_rect(float x, float y, float w, float h, Color4 color);
    void draw_filled_quad(const std::array<XMFLOAT2, 4>& corners, Color4 color);
    void draw_circle(float cx, float cy, float radius, Color4 color, int segments = 24);
    void draw_rect_outline(float x, float y, float w, float h, Color4 color, float thickness = 1.0f);

    // Texture (camera frame etc.)
    void draw_texture(ID3D11ShaderResourceView* srv,
                      float x, float y, float w, float h);

    // Text
    void draw_text(float x, float y, const std::wstring& text, Color4 color);
    void draw_outlined_text(float x, float y, const std::wstring& text,
                            Color4 fill_color, Color4 outline_color = {0, 0, 0, 1});

    bool load_font(const std::wstring& spritefont_path);
    bool has_font() const { return font_ != nullptr; }

private:
    ID3D11Device*        device_  = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    float vp_width_  = 0;
    float vp_height_ = 0;

    std::unique_ptr<DirectX::SpriteBatch>   sprite_batch_;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> prim_batch_;
    std::unique_ptr<DirectX::BasicEffect>   basic_effect_;
    std::unique_ptr<DirectX::CommonStates>  common_states_;
    std::unique_ptr<DirectX::SpriteFont>    font_;
    ComPtr<ID3D11InputLayout>               input_layout_;

    DirectX::VertexPositionColor make_vertex(float x, float y, Color4 c) const;
};

} // namespace sag::d3d

#endif // _WIN32
