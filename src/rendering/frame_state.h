#pragma once
#include <cstdint>

namespace acid::rendering {
// Window messages only queue state; GPU work happens between frames.
struct FrameState {
    std::uint32_t width = 0, height = 0;
    bool minimized = false, resizePending = false, occluded = false;
    void QueueResize(std::uint32_t w, std::uint32_t h, bool isMinimized) noexcept {
        minimized = isMinimized || w == 0 || h == 0;
        if (!minimized) {
            width = w;
            height = h;
            resizePending = true;
        }
    }
    bool CanRender() const noexcept { return !minimized && !occluded; }
};
} // namespace acid::rendering
