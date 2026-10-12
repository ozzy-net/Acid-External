#include <windows.h>
#include <d3d11.h>
#include <algorithm>
#include <cmath>
#include "ui/premium_ui.h"
#include <windowsx.h>
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

static ID3D11Device* device = nullptr;
static ID3D11DeviceContext* context = nullptr;
static IDXGISwapChain* swapchain = nullptr;
static ID3D11RenderTargetView* target = nullptr;

static void CreateTarget() {
    ID3D11Texture2D* buffer = nullptr;
    if (SUCCEEDED(swapchain->GetBuffer(0, IID_PPV_ARGS(&buffer)))) {
        device->CreateRenderTargetView(buffer, nullptr, &target);
        buffer->Release();
    }
}
static void CleanupTarget() {
    if (target) { target->Release(); target = nullptr; }
}
static bool CreateDevice(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hwnd;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL level{};
    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};
    const HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 0, levels, 1, D3D11_SDK_VERSION, &desc, &swapchain,
        &device, &level, &context);
    if (FAILED(hr)) return false;
    CreateTarget();
    return target != nullptr;
}
static void CleanupDevice() {
    CleanupTarget();
    if (swapchain) swapchain->Release();
    if (context) context->Release();
    if (device) device->Release();
}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
static LRESULT WINAPI WindowProc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
    if (message == WM_NCHITTEST) {
        POINT point{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(hwnd, &point);
        RECT client{};
        GetClientRect(hwnd, &client);
        if (!IsZoomed(hwnd)) {
            const bool left = point.x < 7, right = point.x >= client.right - 7;
            const bool top = point.y < 7, bottom = point.y >= client.bottom - 7;
            if (top && left) return HTTOPLEFT;
            if (top && right) return HTTOPRIGHT;
            if (bottom && left) return HTBOTTOMLEFT;
            if (bottom && right) return HTBOTTOMRIGHT;
            if (left) return HTLEFT;
            if (right) return HTRIGHT;
            if (top) return HTTOP;
            if (bottom) return HTBOTTOM;
        }
        if (point.y < 54 && point.x < client.right - 156) return HTCAPTION;
        return HTCLIENT;
    }
    if (message == WM_NCCALCSIZE && wp) return 0;
    if (message == WM_GETMINMAXINFO) {
        auto* limits = reinterpret_cast<MINMAXINFO*>(lp);
        limits->ptMinTrackSize = POINT{600, 480};
        MONITORINFO monitor{sizeof(monitor)};
        if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor)) {
            limits->ptMaxPosition = POINT{monitor.rcWork.left - monitor.rcMonitor.left,
                monitor.rcWork.top - monitor.rcMonitor.top};
            limits->ptMaxSize = POINT{monitor.rcWork.right - monitor.rcWork.left,
                monitor.rcWork.bottom - monitor.rcWork.top};
        }
        return 0;
    }
    if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hwnd, message, wp, lp))
        return 1;
    switch (message) {
    case WM_SIZE:
        if (device && wp != SIZE_MINIMIZED && swapchain) {
            CleanupTarget();
            swapchain->ResizeBuffers(0, LOWORD(lp), HIWORD(lp), DXGI_FORMAT_UNKNOWN, 0);
            CreateTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wp & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wp, lp);
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    WNDCLASSEXW wc{sizeof(wc), CS_CLASSDC, WindowProc, 0, 0, instance,
        nullptr, nullptr, nullptr, nullptr, L"AcidExternalV2", nullptr};
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Acid External V2 - Diagnostics",
        WS_OVERLAPPEDWINDOW, 120, 120, 1100, 720, nullptr, nullptr, instance, nullptr);
    if (!hwnd) { UnregisterClassW(wc.lpszClassName, instance); return 1; }
    if (!CreateDevice(hwnd)) {
        CleanupDevice();
        DestroyWindow(hwnd);
        UnregisterClassW(wc.lpszClassName, instance);
        return 1;
    }
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    acid::ui::ApplyTheme();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device, context);

    acid::ui::State uiState;
    bool running = true;
    while (running) {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = false;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!running) break;
        if (!target) { Sleep(16); continue; }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        const auto actions = acid::ui::Render(uiState, IsZoomed(hwnd) != FALSE);
        if (actions.minimize) ShowWindow(hwnd, SW_MINIMIZE);
        if (actions.maximize) ShowWindow(hwnd, IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
        if (actions.close) PostMessageW(hwnd, WM_CLOSE, 0, 0);
        ImGui::Render();
        const float clear[4] = {0.055f, 0.061f, 0.085f, 1.0f};
        context->OMSetRenderTargets(1, &target, nullptr);
        context->ClearRenderTargetView(target, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        swapchain->Present(1, 0);
    }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDevice();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, instance);
    return 0;
}

