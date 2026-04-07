#pragma once
// D3D11 helper utilities — device creation, texture management

#ifdef _WIN32

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <opencv2/core.hpp>

namespace sag::d3d {

using Microsoft::WRL::ComPtr;

struct D3DContext {
    ComPtr<ID3D11Device>        device;
    ComPtr<ID3D11DeviceContext>  context;
    ComPtr<IDXGISwapChain>      swapchain;
    ComPtr<ID3D11RenderTargetView> rtv;
    int width  = 0;
    int height = 0;
};

// Create a D3D11 device + swap chain for a given HWND
bool create_device_and_swapchain(
    HWND hwnd, int width, int height, D3DContext& out);

// Recreate the render target view (e.g. after resize)
void create_render_target(D3DContext& ctx);

// Upload a BGR cv::Mat as a D3D11 texture (BGRA SRV)
ComPtr<ID3D11ShaderResourceView> upload_texture(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    const cv::Mat& bgr_frame);

// Release and recreate swap chain buffers on resize
void resize_buffers(D3DContext& ctx, int width, int height);

} // namespace sag::d3d

#endif // _WIN32
