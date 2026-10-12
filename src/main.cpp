#include <windows.h>
#include <d3d11.h>
#include <algorithm>
#include <cmath>
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
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 10.0f;
    style.FrameRounding = 7.0f;
    style.GrabRounding = 7.0f;
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device, context);

    int tab = 0;
    float fov = 110.0f;
    bool showCircle = true;
    float accent[3] = {0.38f, 0.34f, 0.95f};
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
        style.Colors[ImGuiCol_Button] = ImVec4(accent[0], accent[1], accent[2], 1.0f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(accent[0], accent[1], accent[2], 1.0f);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
        ImGui::Begin("Acid External", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextColored(ImVec4(accent[0], accent[1], accent[2], 1), "ACID / EXTERNAL V2");
        ImGui::Separator();
        ImGui::BeginChild("Navigation", ImVec2(190, 0), ImGuiChildFlags_Borders);
        if (ImGui::Selectable("Dashboard", tab == 0)) tab = 0;
        if (ImGui::Selectable("FOV Preview", tab == 1)) tab = 1;
        if (ImGui::Selectable("Appearance", tab == 2)) tab = 2;
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("Content", ImVec2(0, 0), ImGuiChildFlags_Borders);
        if (tab == 0) {
            ImGui::Text("Diagnostics");
            ImGui::Separator();
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("Frame time: %.2f ms", io.Framerate > 0 ? 1000.0f / io.Framerate : 0.0f);
            ImGui::TextWrapped("Standalone local UI. No game attachment or process access.");
        } else if (tab == 1) {
            ImGui::Text("FOV Visualization");
            ImGui::SliderFloat("Radius", &fov, 20.0f, 300.0f, "%.0f px");
            ImGui::Checkbox("Show circle", &showCircle);
            ImVec2 origin = ImGui::GetCursorScreenPos();
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::InvisibleButton("preview", ImVec2(std::max(1.0f, avail.x), std::max(1.0f, avail.y)));
            if (showCircle) {
                ImVec2 center(origin.x + avail.x * 0.5f, origin.y + avail.y * 0.5f);
                ImGui::GetWindowDrawList()->AddCircle(center, fov,
                    ImGui::ColorConvertFloat4ToU32(ImVec4(accent[0], accent[1], accent[2], 1)), 96, 2.0f);
            }
        } else {
            ImGui::Text("Appearance");
            ImGui::ColorEdit3("Accent color", accent);
            if (ImGui::Button("Reset theme")) {
                accent[0] = 0.38f; accent[1] = 0.34f; accent[2] = 0.95f;
            }
        }
        ImGui::EndChild();
        ImGui::End();
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
