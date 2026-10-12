# Acid External V2

A standalone Windows diagnostics desktop application using C++20, DirectX 11,
and Dear ImGui v1.91.9b. Measurements cover the application itself.

## Build and test

Install Visual Studio 2022 or newer with Desktop development with C++, the Windows
SDK, CMake 3.21+, and Git. First configuration requires access to GitHub for ImGui.

```powershell
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
.\build\Release\AcidExternal.exe
```

Windows CI runs both Debug and Release builds, diagnostics/settings/UI tests,
renderer lifecycle tests, and application startup/resize smoke tests. The software
WARP renderer provides a fallback when hardware rendering is unavailable.

Portable tests can run on Linux with CMake and a C++20 compiler:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Functionality

- Responsive dashboard, local circle preview, accent palette, and native window controls.
- Current-process CPU usage (100% per logical core), working set, private memory,
  process uptime, bounded frame history, and bounded diagnostic logs.
- Report export to `%LOCALAPPDATA%\AcidExternal\diagnostics-report.json`.
- Radius, circle visibility, and accent saved on normal close to
  `%LOCALAPPDATA%\AcidExternal\config.json`. Versioned JSON settings are validated
  before application and replaced through a temporary file. Invalid settings retain
  defaults and appear in the diagnostics log. Desktop smoke tests do not save settings.
- Checked initialization, deferred resizing, input-aware pacing, minimized/occluded
  suspension, device-loss recovery, and deterministic graphics/backend cleanup.

## Architecture

`src/main.cpp` owns the window and coordinates the modules. `src/ui` only draws UI
and returns actions. `src/config` validates and persists settings. `src/diagnostics`
collects current-process telemetry and exports reports without ImGui dependencies.
`src/rendering` owns DirectX resources and the ImGui DX11 backend. The application
owns the ImGui context and Win32 backend; all work stays on one thread. CMake compiles
ImGui once and shares it across targets.

Standalone diagnostics and rendering build instructions remain in their module
READMEs. The root CMake build is the integrated application build.

See [integration review and testing limitations](docs/integration.md).
