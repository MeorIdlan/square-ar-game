#ifdef _WIN32

#include "d3d_renderer.h"
#include <cmath>

namespace sag::d3d {

bool Renderer2D::initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    device_  = device;
    context_ = context;

    sprite_batch_ = std::make_unique<DirectX::SpriteBatch>(context);

    prim_batch_ = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);

    basic_effect_ = std::make_unique<DirectX::BasicEffect>(device);
    basic_effect_->SetVertexColorEnabled(true);

    common_states_ = std::make_unique<DirectX::CommonStates>(device);

    // Create input layout for VertexPositionColor
    const void* shader_bytecode = nullptr;
    size_t bytecode_length = 0;
    basic_effect_->GetVertexShaderBytecode(&shader_bytecode, &bytecode_length);

    HRESULT hr = device->CreateInputLayout(
        DirectX::VertexPositionColor::InputElements,
        DirectX::VertexPositionColor::InputElementCount,
        shader_bytecode, bytecode_length,
        input_layout_.ReleaseAndGetAddressOf());

    return SUCCEEDED(hr);
}

void Renderer2D::begin_frame(float viewport_width, float viewport_height)
{
    vp_width_  = viewport_width;
    vp_height_ = viewport_height;

    // Set up orthographic projection for 2D rendering (pixel coordinates)
    auto projection = DirectX::XMMatrixOrthographicOffCenterLH(
        0.0f, viewport_width, viewport_height, 0.0f, 0.0f, 1.0f);
    basic_effect_->SetProjection(projection);
    basic_effect_->SetView(DirectX::XMMatrixIdentity());
    basic_effect_->SetWorld(DirectX::XMMatrixIdentity());

    // Apply states for alpha blending
    context_->OMSetBlendState(common_states_->AlphaBlend(), nullptr, 0xFFFFFFFF);
    context_->RSSetState(common_states_->CullNone());
    context_->OMSetDepthStencilState(common_states_->DepthNone(), 0);

    basic_effect_->Apply(context_);
    context_->IASetInputLayout(input_layout_.Get());
}

void Renderer2D::end_frame()
{
    // Nothing special needed; flush happens at present
}

DirectX::VertexPositionColor Renderer2D::make_vertex(float x, float y, Color4 c) const
{
    return DirectX::VertexPositionColor(
        DirectX::XMFLOAT3(x, y, 0.0f),
        DirectX::XMFLOAT4(c.r, c.g, c.b, c.a));
}

void Renderer2D::draw_filled_rect(float x, float y, float w, float h, Color4 color)
{
    auto v0 = make_vertex(x,     y,     color);
    auto v1 = make_vertex(x + w, y,     color);
    auto v2 = make_vertex(x + w, y + h, color);
    auto v3 = make_vertex(x,     y + h, color);

    prim_batch_->Begin();
    prim_batch_->DrawTriangle(v0, v1, v2);
    prim_batch_->DrawTriangle(v0, v2, v3);
    prim_batch_->End();
}

void Renderer2D::draw_filled_quad(const std::array<XMFLOAT2, 4>& corners, Color4 color)
{
    auto v0 = make_vertex(corners[0].x, corners[0].y, color);
    auto v1 = make_vertex(corners[1].x, corners[1].y, color);
    auto v2 = make_vertex(corners[2].x, corners[2].y, color);
    auto v3 = make_vertex(corners[3].x, corners[3].y, color);

    prim_batch_->Begin();
    prim_batch_->DrawTriangle(v0, v1, v2);
    prim_batch_->DrawTriangle(v0, v2, v3);
    prim_batch_->End();
}

void Renderer2D::draw_circle(float cx, float cy, float radius, Color4 color, int segments)
{
    if (segments < 3) segments = 3;

    prim_batch_->Begin();
    auto center = make_vertex(cx, cy, color);

    for (int i = 0; i < segments; ++i) {
        float a0 = 2.0f * 3.14159265f * static_cast<float>(i)     / static_cast<float>(segments);
        float a1 = 2.0f * 3.14159265f * static_cast<float>(i + 1) / static_cast<float>(segments);

        auto p0 = make_vertex(cx + radius * std::cos(a0), cy + radius * std::sin(a0), color);
        auto p1 = make_vertex(cx + radius * std::cos(a1), cy + radius * std::sin(a1), color);

        prim_batch_->DrawTriangle(center, p0, p1);
    }
    prim_batch_->End();
}

void Renderer2D::draw_rect_outline(float x, float y, float w, float h, Color4 color, float thickness)
{
    draw_filled_rect(x, y, w, thickness, color);                 // top
    draw_filled_rect(x, y + h - thickness, w, thickness, color); // bottom
    draw_filled_rect(x, y, thickness, h, color);                 // left
    draw_filled_rect(x + w - thickness, y, thickness, h, color); // right
}

void Renderer2D::draw_texture(ID3D11ShaderResourceView* srv,
                              float x, float y, float w, float h)
{
    if (!srv) return;

    sprite_batch_->Begin(DirectX::SpriteSortMode_Deferred,
                         common_states_->NonPremultiplied());

    RECT dest;
    dest.left   = static_cast<LONG>(x);
    dest.top    = static_cast<LONG>(y);
    dest.right  = static_cast<LONG>(x + w);
    dest.bottom = static_cast<LONG>(y + h);

    sprite_batch_->Draw(srv, dest);
    sprite_batch_->End();

    // Restore primitive batch state after sprite batch
    basic_effect_->Apply(context_);
    context_->IASetInputLayout(input_layout_.Get());
}

void Renderer2D::draw_text(float x, float y, const std::wstring& text, Color4 color)
{
    if (!font_) return;

    sprite_batch_->Begin(DirectX::SpriteSortMode_Deferred,
                         common_states_->NonPremultiplied());
    DirectX::XMFLOAT4 color4(color.r, color.g, color.b, color.a);
    font_->DrawString(sprite_batch_.get(), text.c_str(),
                      DirectX::XMFLOAT2(x, y),
                      DirectX::XMLoadFloat4(&color4));
    sprite_batch_->End();

    // Restore primitive batch state
    basic_effect_->Apply(context_);
    context_->IASetInputLayout(input_layout_.Get());
}

void Renderer2D::draw_outlined_text(float x, float y, const std::wstring& text,
                                    Color4 fill_color, Color4 outline_color)
{
    if (!font_) return;

    sprite_batch_->Begin(DirectX::SpriteSortMode_Deferred,
                         common_states_->NonPremultiplied());

    DirectX::XMFLOAT4 outline4(outline_color.r, outline_color.g, outline_color.b, outline_color.a);
    DirectX::XMFLOAT4 fill4(fill_color.r, fill_color.g, fill_color.b, fill_color.a);
    auto outline = DirectX::XMLoadFloat4(&outline4);
    auto fill = DirectX::XMLoadFloat4(&fill4);

    constexpr float offsets[][2] = {
        {-1,-1}, {-1,0}, {-1,1}, {0,-1}, {0,1}, {1,-1}, {1,0}, {1,1}
    };
    for (auto& [ox, oy] : offsets) {
        font_->DrawString(sprite_batch_.get(), text.c_str(),
                          DirectX::XMFLOAT2(x + ox, y + oy), outline);
    }
    font_->DrawString(sprite_batch_.get(), text.c_str(),
                      DirectX::XMFLOAT2(x, y), fill);

    sprite_batch_->End();

    basic_effect_->Apply(context_);
    context_->IASetInputLayout(input_layout_.Get());
}

bool Renderer2D::load_font(const std::wstring& spritefont_path)
{
    try {
        font_ = std::make_unique<DirectX::SpriteFont>(device_, spritefont_path.c_str());
        return true;
    } catch (...) {
        font_.reset();
        return false;
    }
}

} // namespace sag::d3d

#endif // _WIN32
