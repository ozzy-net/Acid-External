#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <wrl/client.h>
#include <string>
#include "frame_state.h"

struct ImDrawData;
namespace acid::rendering {
enum class FrameResult { Ready, Skipped, DeviceLost, Error };
struct RenderError {
    HRESULT code = S_OK;
    HRESULT removedReason = S_OK;
    std::string operation;
    std::string message;
};

// Single render thread, one existing ImGui context. Does not own HWND, the
// ImGui context, Win32 backend, or application texture resources.
class Dx11Renderer final {
public:
    Dx11Renderer() = default;
    ~Dx11Renderer();
    Dx11Renderer(const Dx11Renderer&) = delete;
    Dx11Renderer& operator=(const Dx11Renderer&) = delete;
    HRESULT Initialize(HWND window, bool debug = false);
    void Shutdown() noexcept;
    void QueueResize(UINT width, UINT height, bool minimized) noexcept;
    // Call after pumping messages, before any ImGui NewFrame. Waits at most
    // 100ms and wakes for input; Skipped means pump messages and retry.
    FrameResult BeginFrame();
    FrameResult RenderAndPresent(ImDrawData* data, const float clear[4]);
    // Caller releases its device resources before recovery and recreates
    // them afterward. The ImGui context and Win32 backend survive.
    HRESULT Recover();
    const RenderError& LastError() const noexcept { return error_; }
    ID3D11Device* Device() const noexcept { return device_.Get(); }
    ID3D11DeviceContext* Context() const noexcept { return context_.Get(); }
    bool UsesFlipModel() const noexcept { return flip_; }
    bool UsesLatencyWait() const noexcept { return latency_ != nullptr; }
private:
    HRESULT CreateDeviceAndSwapChain();
    HRESULT CreateTarget();
    HRESULT Fail(const char* operation, HRESULT hr);
    FrameResult Classify(const char* operation, HRESULT hr);
    void ReleaseGpu() noexcept;
    HWND window_ = nullptr;
    bool debug_ = false, flip_ = false, imguiInitialized_ = false;
    bool lost_ = false, frameReady_ = false;
    UINT swapFlags_ = 0;
    HANDLE latency_ = nullptr;
    FrameState state_;
    RenderError error_;
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target_;
};
} // namespace acid::rendering
