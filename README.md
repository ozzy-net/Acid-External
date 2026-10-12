# Acid External V2

A standalone Windows diagnostics dashboard built with C++20, Dear ImGui and DirectX 11.

This foundation is **not** a game cheat, injector, process-memory reader, or anti-cheat bypass.

## Build (Windows)

Requires Visual Studio 2022 or newer with Desktop development with C++, CMake, and internet access on the first configure to download Dear ImGui.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
.\build\Release\AcidExternal.exe
```

## Features

- Standalone dark-themed ImGui window
- Dashboard with frame timing and FPS
- Adjustable on-screen FOV circle preview
- Settings panel with accent colors
- CMake build and Windows CI

## Cloud agent workstreams

Keep UI, diagnostics, configuration persistence and build automation in separate branches and PRs. Review and test changes before merging.
