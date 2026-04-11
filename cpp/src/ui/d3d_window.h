#pragma once
// Base Win32 window with D3D11 swap chain

#ifdef _WIN32

#include "../utils/d3d_helpers.h"
#include "../utils/d3d_renderer.h"
#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace sag::ui
{

    class D3DWindow
    {
    public:
        D3DWindow() = default;
        virtual ~D3DWindow();

        bool create(const std::wstring &title, int x, int y, int width, int height,
                    HINSTANCE hInstance, bool borderless = false);
        void show();
        void hide();
        bool is_visible() const;
        void resize(int width, int height);

        HWND hwnd() const { return hwnd_; }
        d3d::D3DContext &d3d_ctx() { return d3d_ctx_; }
        d3d::Renderer2D &renderer() { return renderer_; }

        // Present the swap chain (sync_interval: 1=vsync, 0=immediate)
        void present(int sync_interval = 1);

        // Clear the render target
        void clear(float r = 0, float g = 0, float b = 0, float a = 1);

        // Call after WM_SIZE to resize the swap chain
        void handle_resize(int new_width, int new_height);

    protected:
        virtual LRESULT window_proc(UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        static LRESULT CALLBACK static_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        HWND hwnd_ = nullptr;
        d3d::D3DContext d3d_ctx_;
        d3d::Renderer2D renderer_;
        bool visible_ = false;
    };

} // namespace sag::ui

#endif // _WIN32
