#include "dx11_renderer.h"
#include "imgui.h"
#include <iostream>
using namespace acid::rendering;
int main() {
    HWND window = CreateWindowExW(0, L"STATIC", L"Renderer smoke", WS_OVERLAPPEDWINDOW,
        0, 0, 640, 480, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!window) return 1;
    ShowWindow(window, SW_SHOW);
    ImGui::CreateContext();
    int result = 0;
    {
        Dx11Renderer renderer;
        auto check = [&](HRESULT hr) {
            if (FAILED(hr)) { std::cerr << renderer.LastError().message << '\n'; result = 1; }
        };
        check(renderer.Initialize(window));
        if (!result) {
            renderer.QueueResize(0, 0, true);
            if (renderer.BeginFrame() != FrameResult::Skipped) result = 1;
            renderer.QueueResize(800, 600, false);
            FrameResult frame = FrameResult::Skipped;
            // Fixed test UI needs no Win32 backend.
            bool presented = false;
            for (int attempt = 0; attempt < 50 && !presented && !result; ++attempt) {
                MSG message{};
                while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
                frame = renderer.BeginFrame();
                if (frame == FrameResult::Skipped) continue;
                if (frame != FrameResult::Ready) { result = 1; break; }
                ImGui::GetIO().DisplaySize = ImVec2(800, 600);
                ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
                ImGui::NewFrame();
                ImGui::Begin("Smoke");
                ImGui::TextUnformatted("Resize and present");
                ImGui::End();
                ImGui::Render();
                const float clear[] = {0.05f, 0.05f, 0.05f, 1.0f};
                frame = renderer.RenderAndPresent(ImGui::GetDrawData(), clear);
                if (frame == FrameResult::Ready) presented = true;
                else if (frame != FrameResult::Skipped) result = 1;
            }
            if (!presented) { std::cerr << "No visible frame presented\n"; result = 1; }
            // Recover exercises backend detach/reattach while preserving context.
            check(renderer.Recover());
            check(renderer.Initialize(window));
        }
        renderer.Shutdown();
        renderer.Shutdown();
    }
    ImGui::DestroyContext();
    DestroyWindow(window);
    return result;
}
