#ifdef _WIN32

#include "d3d_window.h"
#include "../utils/logger.h"
#include <format>

namespace sag::ui
{

    D3DWindow::~D3DWindow()
    {
        if (hwnd_)
        {
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
        }
    }

    LRESULT CALLBACK D3DWindow::static_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        D3DWindow *self = nullptr;

        if (msg == WM_NCCREATE)
        {
            auto *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
            self = static_cast<D3DWindow *>(cs->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->hwnd_ = hwnd;
        }
        else
        {
            self = reinterpret_cast<D3DWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (self)
        {
            return self->window_proc(msg, wParam, lParam);
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    LRESULT D3DWindow::window_proc(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                handle_resize(LOWORD(lParam), HIWORD(lParam));
            }
            return 0;

        case WM_DESTROY:
            return 0;

        default:
            return DefWindowProc(hwnd_, msg, wParam, lParam);
        }
    }

    bool D3DWindow::create(const std::wstring &title, int x, int y, int width, int height,
                           HINSTANCE hInstance, bool borderless)
    {
        // Register window class
        std::wstring class_name = L"SAG_" + title;
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = static_wnd_proc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = class_name.c_str();
        RegisterClassExW(&wc);

        DWORD style = borderless ? (WS_POPUP) : (WS_OVERLAPPEDWINDOW);

        RECT rc = {0, 0, width, height};
        AdjustWindowRect(&rc, style, FALSE);

        hwnd_ = CreateWindowExW(
            0, class_name.c_str(), title.c_str(), style,
            x, y, rc.right - rc.left, rc.bottom - rc.top,
            nullptr, nullptr, hInstance, this);

        if (!hwnd_)
        {
            Logger::error(std::format("CreateWindowExW failed for '{}'", std::string(title.begin(), title.end())));
            return false;
        }

        if (!d3d::create_device_and_swapchain(hwnd_, width, height, d3d_ctx_))
        {
            Logger::error(std::format("create_device_and_swapchain failed for '{}' {}x{}", std::string(title.begin(), title.end()), width, height));
            return false;
        }
        Logger::info(std::format("D3D device+swapchain created for '{}' {}x{}", std::string(title.begin(), title.end()), width, height));

        if (!renderer_.initialize(d3d_ctx_.device.Get(), d3d_ctx_.context.Get()))
        {
            Logger::error(std::format("Renderer2D init failed for '{}'", std::string(title.begin(), title.end())));
            return false;
        }
        Logger::info(std::format("Renderer2D initialized for '{}'", std::string(title.begin(), title.end())));

        return true;
    }

    void D3DWindow::show()
    {
        if (hwnd_)
        {
            ShowWindow(hwnd_, SW_SHOW);
            UpdateWindow(hwnd_);
            visible_ = true;
        }
    }

    void D3DWindow::hide()
    {
        if (hwnd_)
        {
            ShowWindow(hwnd_, SW_HIDE);
            visible_ = false;
        }
    }

    bool D3DWindow::is_visible() const
    {
        return visible_ && hwnd_ && IsWindowVisible(hwnd_);
    }

    void D3DWindow::resize(int width, int height)
    {
        if (hwnd_)
        {
            SetWindowPos(hwnd_, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
        }
    }

    void D3DWindow::present(int sync_interval)
    {
        if (d3d_ctx_.swapchain)
        {
            d3d_ctx_.swapchain->Present(sync_interval, 0);
        }
    }

    void D3DWindow::clear(float r, float g, float b, float a)
    {
        if (!d3d_ctx_.rtv)
            return;

        float color[4] = {r, g, b, a};
        d3d_ctx_.context->OMSetRenderTargets(1, d3d_ctx_.rtv.GetAddressOf(), nullptr);

        D3D11_VIEWPORT vp{};
        vp.Width = static_cast<float>(d3d_ctx_.width);
        vp.Height = static_cast<float>(d3d_ctx_.height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        d3d_ctx_.context->RSSetViewports(1, &vp);

        d3d_ctx_.context->ClearRenderTargetView(d3d_ctx_.rtv.Get(), color);
    }

    void D3DWindow::handle_resize(int new_width, int new_height)
    {
        if (new_width > 0 && new_height > 0)
        {
            d3d::resize_buffers(d3d_ctx_, new_width, new_height);
        }
    }

} // namespace sag::ui

#endif // _WIN32
