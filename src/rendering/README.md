The root application now uses this renderer. Build and test the complete application
with the root CMake project; the standalone project below remains available.
Hardware initialization falls back to WARP if unavailable.

# DirectX 11 rendering foundation

This module is opt-in. `src/main.cpp` and the root build retain the current UI
and rendering path until the following integration is applied.

## Review of the existing path

The current implementation uses a discard swapchain and raw COM pointers,
ignores `ResizeBuffers`, RTV creation and `Present` errors, releases the RTV
without unbinding it, and performs GPU resize work in `WM_SIZE`. Minimized and
occluded windows continue generating frames. Cleanup leaves dangling globals.

`Dx11Renderer` owns COM objects and the DX11 ImGui backend. It prefers double
buffered flip sequential, tries creation without the latency flag if necessary,
and falls back to discard for older systems. Vsync remains enabled. A waitable
swapchain limits queued frames to one and wakes for window input. The fallback
uses device frame latency and blocking vsync present. No tearing or busy spin
is introduced. Debug device creation is explicit; a missing debug layer is an
error, not silently ignored.

Resize messages coalesce until the next frame. Zero dimensions and minimized
windows suspend rendering. Resizing unbinds and releases the backbuffer RTV,
preserves swapchain flags, checks errors, and recreates the RTV. Application code
must not hold other references/views to the swapchain backbuffer or recorded
command lists referring to it. ImGui objects do not need rebuilding on resize.

`Present` reports occlusion separately and probes at up to 10 Hz while idle.
Removal/reset/hang errors record `GetDeviceRemovedReason` before teardown.
Recovery rebuilds the device, swapchain, target and DX11 backend; it preserves
the ImGui context and application UI state. Each failing operation reports its
HRESULT and removal reason through `LastError()` and `OutputDebugStringA`.
`LastError` retains the last failure even after successful recovery.

## Integration (no UI rewrite)

1. Add `src/rendering/dx11_renderer.cpp` to the existing executable sources.
   Existing ImGui sources, include paths and `d3d11 dxgi` libraries suffice.
   Do not also link the standalone test library: that would duplicate ImGui.
2. Create the ImGui context and initialize the Win32 backend, then call
   `renderer.Initialize(hwnd)`. Remove the old device helpers and the explicit
   `ImGui_ImplDX11_Init`; the renderer owns that backend. Check initialization
   and Win32 backend return values before entering the loop.
3. In `WM_SIZE`, call `renderer.QueueResize(LOWORD(lp), HIWORD(lp),
   wp == SIZE_MINIMIZED)` and return zero. Process this message even if the
   ImGui message handler consumes other input. Call it on the render thread.
4. After pumping all messages and checking `WM_QUIT`, call `BeginFrame()`.
   For `Skipped`, continue the message loop without starting an ImGui frame.
   For `Ready`, call `ImGui_ImplWin32_NewFrame()` then `ImGui::NewFrame()`;
   keep the existing UI body, call `ImGui::Render()`, and then
   `RenderAndPresent(ImGui::GetDrawData(), clear)`. Do not call
   `ImGui_ImplDX11_NewFrame()` or `Present()` separately.
5. Handle `DeviceLost` from either frame method: release all application device
   resources (including ImGui texture SRVs), call `Recover()`, then rebuild
   those resources with `Device()`/`Context()`. Retry failed recovery using a
   bounded backoff and message pumping, or exit with `LastError().message`.
   Treat other `Error` results as actionable failures, not successful frames;
   avoid an unbounded retry loop. For driver hangs, recovery may keep failing.
6. Before destroying the ImGui context or HWND, call `Shutdown()`, then shut
   down the Win32 backend and destroy the context. Shutdown is idempotent.
   Keep the renderer on the same thread throughout its lifetime and do not
   switch ImGui contexts while its backend is initialized. Multi-viewport
   swapchains and exclusive fullscreen are outside this module's scope.

## Validation

Independent build; Linux runs the portable resize-state tests, Windows also
compiles the actual renderer and runs its lifecycle smoke test (requires a
hardware D3D11 adapter). The smoke test covers initialization, deferred resize,
minimize, drawing/present, explicit recovery, repeated initialization and repeated shutdown.
It does not simulate actual driver removal or validate displayed pixels.

```sh
cmake -S src/rendering -B build-rendering
cmake --build build-rendering --config Debug
ctest --test-dir build-rendering -C Debug --output-on-failure
```

On Windows, use `-A x64` for Visual Studio. Enable the optional D3D debug layer
via `Initialize(hwnd, true)` when integrating; verify resize storms and shutdown
produce no live backbuffer/RTV warnings. The root CI does not yet build this
opt-in module: run the standalone commands above in Windows CI as well.

## Frame pacing benchmark procedure

Measure the original and integrated builds in Release on the same machine,
resolution, refresh rate and UI tab. Warm up for 10 seconds, capture 60 seconds
with PresentMon and Windows Performance Recorder, repeat three times. Record
CPU utilization, presents/sec, median/p95/p99 frame intervals, present blocking
time and input-to-display latency if the capture supports it. Record
`UsesFlipModel()` and `UsesLatencyWait()` so fallback runs are identifiable.
Repeat minimized, fully covered, restored and during continuous window resize.
Expect negligible rendering while minimized, occlusion probes at at most 10 Hz
when no input arrives, and no busy polling. Latency flags and flip support vary
by OS/driver; do not claim a speedup from the portable state test.

For device loss, use the Windows Graphics Tools `dxcap -forcetdr` on a test
machine, check the reported removal reason and recover with textures rebuilt.
Also test missing debug-layer startup, unsupported hardware, and repeated
recover/shutdown cycles. TDR affects other graphics applications: save work
before this manual test. No GPU performance measurements were made in Linux.
