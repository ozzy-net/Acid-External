#include <windows.h>
#include <d3d11.h>
#include <algorithm>
#include <cmath>
#include "ui/premium_ui.h"
#include <windowsx.h>
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include "rendering/dx11_renderer.h"
#include "config/settings.h"
#include "diagnostics/diagnostics.hpp"
#include <shlobj.h>
#include <chrono>
#include <stdexcept>
#include <string_view>

static acid::rendering::Dx11Renderer* renderer = nullptr;
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
        if (renderer) renderer->QueueResize(LOWORD(lp), HIWORD(lp), wp == SIZE_MINIMIZED);
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
namespace {
struct Session {
    HINSTANCE instance;
    HWND window = nullptr;
    bool registered = false, win32 = false;
    acid::rendering::Dx11Renderer graphics;
    ~Session() {
        renderer = nullptr;
        graphics.Shutdown();
        if (win32) ImGui_ImplWin32_Shutdown();
        if (ImGui::GetCurrentContext()) ImGui::DestroyContext();
        if (window && IsWindow(window)) DestroyWindow(window);
        if (registered) UnregisterClassW(L"AcidExternalV2", instance);
    }
};
int Run(HINSTANCE instance, int show, bool smoke) {
    Session session{instance};
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc{}; wc.cbSize = sizeof(wc); wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance; wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"AcidExternalV2";
    session.registered = RegisterClassExW(&wc) != 0;
    if (!session.registered) throw std::runtime_error("Window registration failed");
    session.window = CreateWindowW(wc.lpszClassName, L"Acid External V2 - Diagnostics",
        WS_OVERLAPPEDWINDOW, 120, 120, 1100, 720, nullptr, nullptr, instance, nullptr);
    if (!session.window) throw std::runtime_error("Window creation failed");
    HWND hwnd = session.window;
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    acid::ui::ApplyTheme();
    session.win32 = ImGui_ImplWin32_Init(hwnd);
    if (!session.win32) throw std::runtime_error("Win32 UI initialization failed");
    renderer = &session.graphics;
    if (FAILED(renderer->Initialize(hwnd))) throw std::runtime_error(renderer->LastError().message);
    acid::ui::State state;
    acid::diagnostics::Diagnostics diagnostics;
    PWSTR local = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &local)))
        throw std::runtime_error("LocalAppData unavailable");
    std::filesystem::path directory;
    try { directory = std::filesystem::path(local) / L"AcidExternal"; }
    catch (...) { CoTaskMemFree(local); throw; }
    CoTaskMemFree(local);
    const auto settings = directory / L"config.json";
    std::string status;
    if (!smoke && std::filesystem::exists(settings) && !acid::config::Load(settings, state, status))
        diagnostics.log(acid::diagnostics::LogLevel::warning, status);
    diagnostics.log(acid::diagnostics::LogLevel::info, "Diagnostics measure this application only.");
    ShowWindow(hwnd, show); UpdateWindow(hwnd);
    auto sampleTime = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    auto previous = std::chrono::steady_clock::now();
    const auto started = previous;
    bool paused = true; unsigned frames = 0;
    for (;;) {
        MSG msg{}; bool quit = false;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) quit = true;
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        if (quit) break;
        if (smoke && std::chrono::steady_clock::now() - started > std::chrono::seconds(20))
            throw std::runtime_error("Desktop smoke test timed out");
        const auto result = renderer->BeginFrame();
        if (result == acid::rendering::FrameResult::Skipped) { paused = true; continue; }
        if (result == acid::rendering::FrameResult::DeviceLost) {
            diagnostics.log(acid::diagnostics::LogLevel::warning, renderer->LastError().message);
            if (FAILED(renderer->Recover())) throw std::runtime_error(renderer->LastError().message);
            paused = true; continue;
        }
        if (result == acid::rendering::FrameResult::Error) throw std::runtime_error(renderer->LastError().message);
        const auto now = std::chrono::steady_clock::now();
        if (!paused) diagnostics.record_frame(now - previous);
        previous = now; paused = false;
        if (now - sampleTime >= std::chrono::milliseconds(500)) { diagnostics.sample_process(); sampleTime = now; }
        ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
        const auto actions = acid::ui::Render(state, IsZoomed(hwnd) != FALSE, diagnostics.snapshot(), status);
        if (actions.minimize) ShowWindow(hwnd, SW_MINIMIZE);
        if (actions.maximize) ShowWindow(hwnd, IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
        if (actions.close) PostMessageW(hwnd, WM_CLOSE, 0, 0);
        if (actions.exportReport) {
            try {
                std::filesystem::create_directories(directory);
                acid::diagnostics::export_report(diagnostics.snapshot(), directory / L"diagnostics-report.json");
                status = "Report saved in LocalAppData/AcidExternal/diagnostics-report.json";
            } catch (const std::exception& e) { status = e.what(); }
            diagnostics.log(acid::diagnostics::LogLevel::info, status);
        }
        ImGui::Render();
        const float clear[4] = {0.055f, 0.061f, 0.085f, 1.0f};
        const auto presented = renderer->RenderAndPresent(ImGui::GetDrawData(), clear);
        if (presented == acid::rendering::FrameResult::Error) throw std::runtime_error(renderer->LastError().message);
        if (presented == acid::rendering::FrameResult::DeviceLost) {
            diagnostics.log(acid::diagnostics::LogLevel::warning, renderer->LastError().message);
            if (FAILED(renderer->Recover())) throw std::runtime_error(renderer->LastError().message);
            paused = true;
        }
        if (smoke) {
            ++frames;
            if (frames == 2) SetWindowPos(hwnd, nullptr, 0, 0, 800, 600, SWP_NOMOVE | SWP_NOZORDER);
            if (frames >= 12) break;
        }
    }
    if (!smoke && !acid::config::Save(settings, state, status))
        MessageBoxA(nullptr, status.c_str(), "Settings could not be saved", MB_OK | MB_ICONWARNING);
    return 0;
}
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR arguments, int show) {
    const bool smoke = std::wstring_view(arguments) == L"--smoke-test";
    try { return Run(instance, show, smoke); }
    catch (const std::exception& e) {
        OutputDebugStringA(e.what());
        if (!smoke) MessageBoxA(nullptr, e.what(), "Acid External error", MB_OK | MB_ICONERROR);
        return 1;
    }
}
