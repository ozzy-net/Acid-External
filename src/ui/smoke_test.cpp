// Headless Dear ImGui smoke test; no Win32 renderer or application linkage required.
#include "premium_ui.h"
#include "imgui.h"
#include <cassert>
#include <cmath>
#include <initializer_list>

int main() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    acid::ui::ApplyTheme();
    acid::ui::State state;
    for (ImVec2 size : {ImVec2(600, 480), ImVec2(790, 600), ImVec2(1100, 720), ImVec2(1920, 1080)}) {
        io.DisplaySize = size;
        for (int page = 0; page < 3; ++page) {
            state.page = page;
            state.reveal = 0;
            for (int frame = 0; frame < 60; ++frame) {
                io.DeltaTime = 1.0f / 60.0f;
                state.radius = frame % 2 ? 20 : 300;
                state.showCircle = frame % 3 != 0;
                ImGui::NewFrame();
                const auto actions = acid::ui::Render(state, false);
                assert(!actions.close && !actions.minimize && !actions.maximize);
                ImGui::Render();
                const auto* data = ImGui::GetDrawData();
                assert(data && data->TotalVtxCount > 0);
                for (int list = 0; list < data->CmdListsCount; ++list)
                    for (const auto& vertex : data->CmdLists[list]->VtxBuffer)
                        assert(std::isfinite(vertex.pos.x) && std::isfinite(vertex.pos.y));
            }
            assert(state.reveal > 0.99f && state.reveal <= 1.0f);
        }
    }
    ImGui::DestroyContext();
}
