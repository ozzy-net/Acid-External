# Premium interface

`premium_ui.cpp` owns the theme, sidebar, diagnostics, FOV canvas, and appearance
controls. `State` preserves the existing radius, circle visibility, and accent
defaults. Selection highlights and page opacity use delta-time exponential
interpolation. Cards stack below 500 pixels of content width; the sidebar narrows
below 800 pixels of window width. Content scrolls on smaller windows.

The host in `src/main.cpp` handles native hit testing for the 54-pixel title region,
seven-pixel resize edges, a 600 x 480 minimum size, and monitor work-area maximize
bounds. Title buttons return window actions to the host. Dragging and title-bar
double-click use standard Windows behavior.

## Validation

The UI compiles against the pinned Dear ImGui v1.91.9b headers with GCC C++20,
`-Wall -Wextra -Wpedantic -Werror`. The headless smoke test renders all three pages
at 600 x 480, 790 x 600, 1100 x 720, and 1920 x 1080 over 720 frames, checks
finite draw geometry, exercises circle visibility and radius limits, and checks
animation convergence.

To run the test with the Windows build:

```powershell
cmake -S . -B build -A x64 -DACID_UI_SMOKE_TEST=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The editing environment has GCC but no CMake, Windows SDK, DirectX runtime, or
Windows desktop. Native build, visual appearance, mouse/keyboard interaction,
title dragging, edge resizing, minimize/restore, and multi-monitor maximize must
be checked on Windows. Headless tests do not verify those behaviors.

Manual checks: open each page at minimum and normal size; adjust radius to both
limits and toggle the circle; change and reset the accent; switch pages quickly;
drag and double-click the title; resize all edges and corners; minimize, restore,
maximize on each monitor, and close. Confirm FPS and timing remain live.
