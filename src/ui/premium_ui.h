#pragma once

namespace acid::ui {
struct State {
    int page = 0;
    float radius = 110.0f;
    bool showCircle = true;
    float accent[3] = {0.38f, 0.34f, 0.95f};
    float selection[3] = {};
    float reveal = 0.0f;
};
struct WindowActions {
    bool minimize = false;
    bool maximize = false;
    bool close = false;
};
void ApplyTheme();
WindowActions Render(State& state, bool maximized);
}
