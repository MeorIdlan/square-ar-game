#ifdef _WIN32

#include "d3d_helpers.h"
#include <opencv2/imgproc.hpp>
#include <stdexcept>

namespace sag::d3d {

bool create_device_and_swapchain(
    HWND hwnd, int width, int height, D3DContext& out)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount        = 2;
    sd.BufferDesc.Width   = static_cast<UINT>(width);
    sd.BufferDesc.Height  = static_cast<UINT>(height);
    sd.BufferDesc.Format  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count   = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed     = TRUE;
    sd.SwapEffect   = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags        = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    D3D_FEATURE_LEVEL feature_level;
    constexpr D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        flags, levels, 2,
        D3D11_SDK_VERSION, &sd,
        out.swapchain.GetAddressOf(),
        out.device.GetAddressOf(),
        &feature_level,
        out.context.GetAddressOf());

    if (FAILED(hr)) return false;

    out.width  = width;
    out.height = height;
    create_render_target(out);
    return true;
}

void create_render_target(D3DContext& ctx)
{
    ComPtr<ID3D11Texture2D> back_buffer;
    ctx.swapchain->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf()));
    ctx.device->CreateRenderTargetView(
        back_buffer.Get(), nullptr, ctx.rtv.ReleaseAndGetAddressOf());
}

ComPtr<ID3D11ShaderResourceView> upload_texture(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    const cv::Mat& bgr_frame)
{
    if (bgr_frame.empty()) return nullptr;

    cv::Mat bgra;
    cv::cvtColor(bgr_frame, bgra, cv::COLOR_BGR2BGRA);

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width     = static_cast<UINT>(bgra.cols);
    desc.Height    = static_cast<UINT>(bgra.rows);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format    = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage     = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem     = bgra.data;
    init.SysMemPitch  = static_cast<UINT>(bgra.step);

    ComPtr<ID3D11Texture2D> tex;
    HRESULT hr = device->CreateTexture2D(&desc, &init, tex.GetAddressOf());
    if (FAILED(hr)) return nullptr;

    ComPtr<ID3D11ShaderResourceView> srv;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format = desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(tex.Get(), &srv_desc, srv.GetAddressOf());
    if (FAILED(hr)) return nullptr;

    return srv;
}

void resize_buffers(D3DContext& ctx, int width, int height)
{
    if (!ctx.swapchain) return;

    ctx.rtv.Reset();
    ctx.swapchain->ResizeBuffers(
        0,
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        DXGI_FORMAT_UNKNOWN, 0);
    ctx.width  = width;
    ctx.height = height;
    create_render_target(ctx);
}

} // namespace sag::d3d

#endif // _WIN32
