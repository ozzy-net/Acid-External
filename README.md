# Acid External V2

A standalone Windows 10/11 diagnostics application using C++20, Dear ImGui and DirectX 11. The custom dark workspace provides a sidebar, animated navigation, rounded panels, Segoe UI typography and a draggable title bar with native resizing, snapping and window controls. Runs as a standard user without a console window.

## Build with Visual Studio

Install **Desktop development with C++**, the Windows 10 or 11 SDK, Git and CMake. Visual Studio 2022 needs CMake 3.21 or newer; Visual Studio 2026 needs CMake 4.2 or newer. The first configuration needs network access to fetch the pinned Dear ImGui `v1.91.9b` release. No other third-party libraries are required.

From PowerShell in the repository root, choose **one** generator:

```powershell
# Visual Studio 2022
cmake -S . -B build-vs2022 -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build build-vs2022 --config Release --parallel
ctest --test-dir build-vs2022 -C Release --output-on-failure
.\build-vs2022\Release\AcidExternal.exe
```

```powershell
# Visual Studio 2026 (CMake 4.2+)
cmake -S . -B build-vs2026 -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON
cmake --build build-vs2026 --config Release --parallel
ctest --test-dir build-vs2026 -C Release --output-on-failure
.\build-vs2026\Release\AcidExternal.exe
```

Use separate build directories for each generator. Replace `Release` with `Debug` in build, test and executable paths to build Debug. The generated solution can also be opened in Visual Studio. When offline, provide an existing ImGui checkout using `-DFETCHCONTENT_SOURCE_DIR_IMGUI=C:/path/to/imgui` during configuration.

## Workspace

- **Dashboard:** application FPS and mean frame interval, process CPU utilization, working set, session uptime, renderer adapter and system summary.
- **Visualization:** circle visibility, radius, stroke, RGBA color, crosshair visibility and size. All drawing stays inside an independent preview canvas; it is never an external overlay.
- **Diagnostics:** last 240 frame intervals; CPU and working-set history sampled every 0.5 seconds; private committed memory; a bounded event console; clear controls and JSON report export.
- **Settings:** indigo/blue/purple presets, custom accent, background brightness, animation and VSync controls; save, load and reset.

Settings live at `%LOCALAPPDATA%\AcidExternal\config.json`. Current preferences save on normal exit. Loading is transactional: malformed JSON, unknown/duplicate fields, unsupported versions, oversized files and out-of-range values are rejected without changing the live configuration. Saving validates values and atomically replaces the existing file. Reset applies defaults immediately; save them to persist immediately. Reports are written to `%LOCALAPPDATA%\AcidExternal\reports\diagnostics-<timestamp>.json`; the full path appears in the UI and console.

Frame intervals include application processing and presentation wait, and are **not GPU execution timings**. FPS uses the mean of the available active frame samples. Process CPU is normalized across all logical processors (100% means all cores busy). Working set is resident process memory; private commit is a different measurement. Failed system queries are shown as unavailable or awaiting samples, and exported as `null`. Minimized/occluded periods are excluded from frame sampling. CPU and memory history cover up to 120 seconds; frame history covers 240 frames. Session uptime includes paused periods.

VSync is enabled by default. Disabling it uses a coarse near-240-FPS pacing limit. Hardware initialization falls back to the DirectX software renderer (WARP); the adapter name indicates the selected renderer. Resizing is deferred until rendering, checks all HRESULTs and unbinds the old render target. Device loss triggers renderer recreation; unrecoverable failures report an error and exit. COM resources, ImGui backends, windows and window classes are cleaned up during shutdown.

## Architecture

```text
src/core/          Win32 lifetime, event loop, DPI and window behavior
src/ui/            Theme, sidebar, tabs and preview drawing
src/diagnostics/   Measured telemetry, bounded histories, event log and reports
src/config/        Versioned JSON parsing and transactional persistence
src/rendering/     DirectX 11 resources and presentation
assets/            Application manifest and asset notes
tests/            Configuration and bounded-history regression tests
```

There is no injection, game attachment, external process-memory access or anti-cheat functionality. Measurements use only the current application process and standard Windows system information APIs.

## Verification

Portable tests can run on Linux without fetching ImGui:

```sh
cmake -S . -B build-core -DACID_BUILD_APP=OFF -DBUILD_TESTING=ON
cmake --build build-core --parallel
ctest --test-dir build-core --output-on-failure
```

Without CMake, compile the same tests directly:

```sh
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Isrc src/config/Config.cpptests/CoreTests.cpp -o /tmp/acid-core-tests
/tmp/acid-core-tests
```

`AcidExternal --smoke-test` initializes the window, DirectX and ImGui, draws 12 frames, resizes after frame 2, and exits without saving preferences. CTest imposes a 30-second timeout. This requires a Windows session capable of creating a DirectX rendering window. CI builds/tests Debug and Release with VS 2022 and VS 2026 and uploads Release executables, plus runs portable tests on Linux.

Before release, manually check Windows 10 and 11 with hardware rendering and WARP: repeated resize/minimize/restore/maximize; title dragging, double-click and snap; mixed-DPI monitors; keyboard navigation; settings persistence/rejection; report output; prolonged idle and device-loss recovery. Automated startup tests do not establish visual quality, GPU-driver compatibility or leak-free behavior under every driver. Reports include system/adapter descriptions and application events; inspect them before sharing.
