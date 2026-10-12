#pragma once
#include "config/settings.h"
#include "diagnostics/diagnostics.hpp"

namespace acid::ui {
struct State : config::Settings {
    int page = 0;
    float selection[3] = {};
    float reveal = 0.0f;
};
struct WindowActions {
    bool minimize = false;
    bool maximize = false;
    bool close = false;
    bool exportReport = false;
};
void ApplyTheme();
WindowActions Render(State& state, bool maximized, const diagnostics::Snapshot& snapshot = {}, const std::string& status = {});
}
