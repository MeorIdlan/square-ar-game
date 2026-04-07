// Square AR Game — C++ reimplementation
// Entry point: WinMain

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mfapi.h>
#include <objbase.h>

#include "app/application.h"

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPSTR /*lpCmdLine*/,
    _In_ int /*nCmdShow*/)
{
    // Initialize COM (required for Media Foundation and DXGI)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize COM.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Initialize Media Foundation
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize Media Foundation.", L"Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    int exit_code = 0;
    {
        sag::Application app;

        if (!app.initialize(hInstance)) {
            MessageBoxW(nullptr, L"Failed to initialize application.", L"Error", MB_OK | MB_ICONERROR);
            exit_code = 1;
        } else {
            exit_code = app.run();
        }

        app.shutdown();
    }

    MFShutdown();
    CoUninitialize();

    return exit_code;
}

#else

// Non-Windows stub so the project structure compiles
#include <cstdio>
int main()
{
    std::printf("Square AR Game requires Windows (Media Foundation + Direct3D 11).\n");
    return 1;
}

#endif // _WIN32
