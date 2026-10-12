#include "premium_ui.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cfloat>

namespace acid::ui {
namespace {
const ImVec4 muted(0.52f, 0.59f, 0.73f, 1.0f);
ImVec4 Accent(const State& s, float alpha = 1.0f) {
    return ImVec4(s.accent[0], s.accent[1], s.accent[2], alpha);
}
float Ease(float current, float target, float dt) {
    return current + (target - current) * (1.0f - std::exp(-12.0f * dt));
}
void Label(const char* text) { ImGui::TextColored(muted, "%s", text); }
void Card(const char* id, float width, const char* title, float value, const char* unit) {
    ImGui::BeginChild(id, ImVec2(width, 110), ImGuiChildFlags_Borders);
    Label(title);
    ImGui::Spacing();
    ImGui::Text("%.1f %s", value, unit);
    ImGui::EndChild();
}
void Preview(const State& s) {
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec2 size(std::max(1.0f, avail.x), std::max(180.0f, avail.y));
    ImGui::InvisibleButton("##canvas", size);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true);
    draw->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y),
        IM_COL32(11, 17, 31, 255), 12);
    for (float x = 0; x < size.x; x += 28)
        draw->AddLine(ImVec2(origin.x + x, origin.y), ImVec2(origin.x + x, origin.y + size.y),
            IM_COL32(28, 37, 58, 120));
    for (float y = 0; y < size.y; y += 28)
        draw->AddLine(ImVec2(origin.x, origin.y + y), ImVec2(origin.x + size.x, origin.y + y),
            IM_COL32(28, 37, 58, 120));
    const ImVec2 center(origin.x + size.x * 0.5f, origin.y + size.y * 0.5f);
    if (s.circle) {
        draw->AddCircle(center, s.radius, ImGui::ColorConvertFloat4ToU32(Accent(s, 0.12f)), 96, 10);
        draw->AddCircle(center, s.radius, ImGui::ColorConvertFloat4ToU32(Accent(s)), 96, 2);
    }
    draw->AddLine(ImVec2(center.x - 6, center.y), ImVec2(center.x + 6, center.y), IM_COL32(180, 193, 220, 255));
    draw->AddLine(ImVec2(center.x, center.y - 6), ImVec2(center.x, center.y + 6), IM_COL32(180, 193, 220, 255));
    draw->PopClipRect();
}
}

void ApplyTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding = ImVec2(18, 16);
    s.FramePadding = ImVec2(12, 9);
    s.ItemSpacing = ImVec2(12, 12);
    s.WindowRounding = 0;
    s.ChildRounding = 12;
    s.FrameRounding = 7;
    s.GrabRounding = 7;
    s.ScrollbarRounding = 8;
    s.WindowBorderSize = 0;
    s.ChildBorderSize = 1;
    s.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.93f, 1, 1);
    s.Colors[ImGuiCol_TextDisabled] = muted;
    s.Colors[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.05f, 0.09f, 1);
    s.Colors[ImGuiCol_ChildBg] = ImVec4(0.055f, 0.075f, 0.13f, 1);
    s.Colors[ImGuiCol_Border] = ImVec4(0.14f, 0.18f, 0.28f, 1);
    s.Colors[ImGuiCol_FrameBg] = ImVec4(0.09f, 0.12f, 0.20f, 1);
    s.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.18f, 0.29f, 1);
    s.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.21f, 0.34f, 1);
    s.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.43f, 0.40f, 0.86f, 1);
    s.Colors[ImGuiCol_ButtonActive] = ImVec4(0.31f, 0.28f, 0.68f, 1);
}

WindowActions Render(State& state, bool maximized, const diagnostics::Snapshot& snapshot, const std::string& status) {
    WindowActions actions;
    const ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Button] = Accent(state, 0.75f);
    style.Colors[ImGuiCol_SliderGrab] = Accent(state);
    style.Colors[ImGuiCol_SliderGrabActive] = Accent(state);
    style.Colors[ImGuiCol_CheckMark] = Accent(state);
    style.Colors[ImGuiCol_Header] = Accent(state, 0.22f);
    style.Colors[ImGuiCol_HeaderHovered] = Accent(state, 0.35f);
    style.Colors[ImGuiCol_HeaderActive] = Accent(state, 0.5f);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("Acid External", nullptr, ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    // The native title drag region is the top 54 pixels; buttons occupy its right edge.
    ImGui::TextColored(Accent(state), "ACID");
    ImGui::SameLine();
    Label("/ EXTERNAL V2");
    ImGui::SameLine(std::max(240.0f, io.DisplaySize.x - 150.0f));
    actions.minimize = ImGui::Button("-##minimize", ImVec2(32, 24));
    ImGui::SameLine(0, 8);
    actions.maximize = ImGui::Button(maximized ? "[]##restore" : "[]##maximize", ImVec2(32, 24));
    ImGui::SameLine(0, 8);
    actions.close = ImGui::Button("X##close", ImVec2(32, 24));
    ImGui::SetCursorPosY(54);
    ImGui::Separator();
    const bool compact = io.DisplaySize.x < 800;
    const float sidebarWidth = compact ? 140.0f : 210.0f;
    ImGui::BeginChild("Navigation", ImVec2(sidebarWidth, 0), ImGuiChildFlags_Borders);
    Label("WORKSPACE");
    ImGui::Spacing();
    const char* pages[] = {"Dashboard", "FOV Preview", "Appearance"};
    for (int i = 0; i < 3; ++i) {
        state.selection[i] = Ease(state.selection[i], state.page == i ? 1.0f : 0.0f, io.DeltaTime);
        ImGui::PushStyleColor(ImGuiCol_Header, Accent(state, 0.12f + 0.20f * state.selection[i]));
        if (ImGui::Selectable(pages[i], state.page == i, 0, ImVec2(0, 38))) {
            if (state.page != i) state.reveal = 0;
            state.page = i;
        }
        ImGui::PopStyleColor();
    }
    ImGui::Spacing();
    ImGui::Separator();
    Label("LOCAL SESSION");
    ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.70f, 1), "Ready");
    ImGui::TextWrapped("Standalone diagnostics");
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Content", ImVec2(0, 0));
    state.reveal = Ease(state.reveal, 1, io.DeltaTime);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.4f + 0.6f * state.reveal);
    ImGui::Text("%s", pages[state.page]);
    Label(state.page == 0 ? "Your workspace, at a glance." :
        state.page == 1 ? "Tune the local visualization." : "Make the workspace yours.");
    ImGui::Spacing();
    if (state.page == 0) {
        const float width = ImGui::GetContentRegionAvail().x;
        const bool stacked = width < 500;
        Card("FPS", stacked ? width : (width - style.ItemSpacing.x) / 2, "FRAME RATE", io.Framerate, "FPS");
        if (!stacked) ImGui::SameLine();
        Card("Timing", stacked ? width : (width - style.ItemSpacing.x) / 2, "FRAME TIME",
            io.Framerate > 0 ? 1000 / io.Framerate : 0, "ms");
        ImGui::Spacing();
        ImGui::BeginChild("Session", ImVec2(0, 0), ImGuiChildFlags_Borders);
        ImGui::Text("Session overview");
        ImGui::TextWrapped("Standalone local UI. No game attachment or process access.");
        Label("Dear ImGui / DirectX 11");
        if (snapshot.process.cpu_percent) ImGui::Text("Process CPU: %.1f%% (100%% per core)", *snapshot.process.cpu_percent);
        else ImGui::TextUnformatted("Process CPU: unavailable / warming up");
        if (snapshot.process.memory) ImGui::Text("Working set: %.1f MiB | Private: %.1f MiB",
            snapshot.process.memory->working_set_bytes / 1048576.0, snapshot.process.memory->private_bytes / 1048576.0);
        else ImGui::TextUnformatted("Process memory: unavailable");
        if (snapshot.process.uptime_seconds) ImGui::Text("Process uptime: %.1f seconds", *snapshot.process.uptime_seconds);
        if (!snapshot.frame_times_ms.empty()) ImGui::PlotLines("Frame ms", snapshot.frame_times_ms.data(),
            static_cast<int>(snapshot.frame_times_ms.size()), 0, nullptr, 0, FLT_MAX, ImVec2(0, 70));
        actions.exportReport = ImGui::Button("Export diagnostics report");
        if (!status.empty()) ImGui::TextWrapped("%s", status.c_str());
        for (const auto& entry : snapshot.logs) ImGui::TextWrapped("%s", entry.message.c_str());
        ImGui::EndChild();
    } else if (state.page == 1) {
        ImGui::SetNextItemWidth(std::max(100.0f, ImGui::GetContentRegionAvail().x - 85));
        ImGui::SliderFloat("Radius", &state.radius, 20, 300, "%.0f px");
        ImGui::Checkbox("Show circle", &state.circle);
        Label("Preview is clipped to the canvas at smaller window sizes.");
        Preview(state);
    } else {
        ImGui::BeginChild("Theme", ImVec2(0, 260), ImGuiChildFlags_Borders);
        ImGui::Text("Accent palette");
        ImGui::TextWrapped("A dark blue foundation with your choice of accent.");
        ImGui::SetNextItemWidth(std::min(320.0f, ImGui::GetContentRegionAvail().x));
        ImGui::ColorEdit3("##accent", state.accent.data());
        if (ImGui::Button("Reset theme")) {
            state.accent[0] = 0.38f; state.accent[1] = 0.34f; state.accent[2] = 0.95f;
        }
        Label("Settings are saved when the application closes.");
        ImGui::EndChild();
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::End();
    return actions;
}
}
