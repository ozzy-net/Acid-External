#include "frame_state.h"
#include <cstdlib>
#include <iostream>
using acid::rendering::FrameState;
static void Check(bool condition, const char* name) {
    if (!condition) { std::cerr << name << '\n'; std::exit(1); }
}
int main() {
    FrameState state;
    state.QueueResize(800, 600, false);
    Check(state.resizePending && state.CanRender(), "initial resize");
    state.QueueResize(1920, 1080, false);
    Check(state.width == 1920 && state.height == 1080, "coalesce resize storm");
    state.QueueResize(0, 0, true);
    Check(!state.CanRender() && state.width == 1920, "minimize preserves pending dimensions");
    state.QueueResize(0, 100, false);
    Check(!state.CanRender(), "zero width suspends rendering");
    state.QueueResize(100, 0, false);
    Check(!state.CanRender(), "zero height suspends rendering");
    state.QueueResize(640, 480, false);
    Check(state.CanRender() && state.resizePending && state.width == 640, "restore queues resize");
    state.occluded = true;
    Check(!state.CanRender(), "occlusion suspends rendering");
    state.QueueResize(1280, 720, false);
    Check(state.occluded, "resize must not assume occlusion ended");
    state.occluded = false;
    Check(state.CanRender(), "successful present probe resumes rendering");
    std::cout << "frame state tests passed\n";
}
