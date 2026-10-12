#include "dx11_renderer.h"
#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include <cstdio>

namespace acid::rendering {
using Microsoft::WRL::ComPtr;
Dx11Renderer::~Dx11Renderer() { Shutdown(); }

HRESULT Dx11Renderer::Fail(const char* operation, HRESULT hr) {
    error_.code = hr;
    error_.operation = operation;
    error_.removedReason = device_ ? device_->GetDeviceRemovedReason() : S_OK;
    char code[96];
    std::snprintf(code, sizeof(code), " (HRESULT 0x%08lX, removal 0x%08lX)",
        static_cast<unsigned long>(hr), static_cast<unsigned long>(error_.removedReason));
    error_.message = error_.operation + code;
    OutputDebugStringA((error_.message + "\n").c_str());
    return hr;
}
FrameResult Dx11Renderer::Classify(const char* operation, HRESULT hr) {
    Fail(operation, hr);
    lost_ = hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET ||
        hr == DXGI_ERROR_DEVICE_HUNG || FAILED(error_.removedReason);
    return lost_ ? FrameResult::DeviceLost : FrameResult::Error;
}
HRESULT Dx11Renderer::Initialize(HWND window, bool debug) {
    Shutdown();
    window_ = window;
    debug_ = debug;
    if (!IsWindow(window_) || !ImGui::GetCurrentContext())
        return Fail("Initialize: valid HWND and ImGui context required", E_INVALIDARG);
    RECT client{};
    if (!GetClientRect(window_, &client)) return Fail("GetClientRect", HRESULT_FROM_WIN32(GetLastError()));
    state_.QueueResize(static_cast<UINT>(client.right), static_cast<UINT>(client.bottom), IsIconic(window_) != FALSE);
    const HRESULT hr = CreateDeviceAndSwapChain();
    if (FAILED(hr)) ReleaseGpu();
    return hr;
}
HRESULT Dx11Renderer::CreateDeviceAndSwapChain() {
    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    if (debug_) flags |= D3D11_CREATE_DEVICE_DEBUG;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        flags, levels, 1, D3D11_SDK_VERSION, device_.GetAddressOf(), nullptr, context_.GetAddressOf());
    if (FAILED(hr)) {
        device_.Reset(); context_.Reset();
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            flags & ~D3D11_CREATE_DEVICE_DEBUG, levels, 1, D3D11_SDK_VERSION,
            device_.GetAddressOf(), nullptr, context_.GetAddressOf());
    }
    if (FAILED(hr)) return Fail("D3D11CreateDevice (hardware and WARP)", hr);
    ComPtr<IDXGIDevice> dxgiDevice;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory1> factory;
    if (FAILED(hr = device_.As(&dxgiDevice))) return Fail("Query IDXGIDevice", hr);
    if (FAILED(hr = dxgiDevice->GetAdapter(adapter.GetAddressOf()))) return Fail("GetAdapter", hr);
    if (FAILED(hr = adapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf())))) return Fail("GetFactory", hr);
    ComPtr<IDXGIFactory2> factory2;
    flip_ = false;
    swapFlags_ = 0;
    // Win 8.1+ supports a message-aware frame-latency wait. Prefer flip
    // sequential for compatibility; fall back without the optional flag,
    // then to the legacy blt model if flip creation is unavailable.
    if (SUCCEEDED(factory.As(&factory2))) {
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width = state_.width ? state_.width : 1;
        desc.Height = state_.height ? state_.height : 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        ComPtr<IDXGISwapChain1> chain;
        hr = factory2->CreateSwapChainForHwnd(device_.Get(), window_, &desc, nullptr, nullptr, chain.GetAddressOf());
        if (FAILED(hr)) {
            chain.Reset();
            desc.Flags = 0;
            hr = factory2->CreateSwapChainForHwnd(device_.Get(), window_, &desc, nullptr, nullptr, chain.GetAddressOf());
        }
        if (SUCCEEDED(hr)) {
            hr = chain.As(&swapchain_);
            if (FAILED(hr)) return Fail("Query IDXGISwapChain", hr);
            flip_ = true;
            swapFlags_ = desc.Flags;
        }
    }
    if (!swapchain_) {
        DXGI_SWAP_CHAIN_DESC desc{};
        desc.BufferDesc.Width = state_.width ? state_.width : 1;
        desc.BufferDesc.Height = state_.height ? state_.height : 1;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.OutputWindow = window_;
        desc.Windowed = TRUE;
        desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        hr = factory->CreateSwapChain(device_.Get(), &desc, swapchain_.GetAddressOf());
        if (FAILED(hr)) return Fail("CreateSwapChain", hr);
    }
    hr = factory->MakeWindowAssociation(window_, DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(hr)) return Fail("MakeWindowAssociation", hr);
    if (swapFlags_ & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT) {
        ComPtr<IDXGISwapChain2> chain2;
        if (FAILED(hr = swapchain_.As(&chain2))) return Fail("Query IDXGISwapChain2", hr);
        if (FAILED(hr = chain2->SetMaximumFrameLatency(1))) return Fail("SetMaximumFrameLatency", hr);
        latency_ = chain2->GetFrameLatencyWaitableObject();
        if (!latency_) return Fail("GetFrameLatencyWaitableObject", E_FAIL);
    } else {
        ComPtr<IDXGIDevice1> dxgiDevice1;
        if (FAILED(hr = dxgiDevice.As(&dxgiDevice1))) return Fail("Query IDXGIDevice1", hr);
        hr = dxgiDevice1->SetMaximumFrameLatency(1);
        if (FAILED(hr)) return Fail("SetMaximumFrameLatency (device)", hr);
    }
    if (FAILED(hr = CreateTarget())) return hr;
    if (!ImGui_ImplDX11_Init(device_.Get(), context_.Get())) return Fail("ImGui_ImplDX11_Init", E_FAIL);
    imguiInitialized_ = true;
    if (!ImGui_ImplDX11_CreateDeviceObjects()) return Fail("ImGui_ImplDX11_CreateDeviceObjects", E_FAIL);
    state_.resizePending = false;
    state_.occluded = false;
    lost_ = false;
    return S_OK;
}
HRESULT Dx11Renderer::CreateTarget() {
    ComPtr<ID3D11Texture2D> buffer;
    HRESULT hr = swapchain_->GetBuffer(0, IID_PPV_ARGS(buffer.GetAddressOf()));
    if (FAILED(hr)) return Fail("GetBuffer", hr);
    hr = device_->CreateRenderTargetView(buffer.Get(), nullptr, target_.ReleaseAndGetAddressOf());
    return FAILED(hr) ? Fail("CreateRenderTargetView", hr) : S_OK;
}
void Dx11Renderer::QueueResize(UINT width, UINT height, bool minimized) noexcept {
    state_.QueueResize(width, height, minimized);
}
FrameResult Dx11Renderer::BeginFrame() {
    frameReady_ = false;
    if (lost_) return FrameResult::DeviceLost;
    if (!swapchain_ || !imguiInitialized_) return Classify("BeginFrame: not initialized", E_UNEXPECTED);
    if (state_.minimized) {
        MsgWaitForMultipleObjectsEx(0, nullptr, 100, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        return FrameResult::Skipped;
    }
    if (state_.resizePending) {
        context_->OMSetRenderTargets(0, nullptr, nullptr);
        target_.Reset();
        HRESULT hr = swapchain_->ResizeBuffers(0, state_.width, state_.height, DXGI_FORMAT_UNKNOWN, swapFlags_);
        if (FAILED(hr)) return Classify("ResizeBuffers", hr);
        // Do not resize again if only target recreation needs a retry.
        state_.resizePending = false;
    }
    if (!target_) {
        HRESULT hr = CreateTarget();
        if (FAILED(hr)) return Classify("CreateTarget", hr);
    }
    if (state_.occluded) {
        HRESULT hr = swapchain_->Present(0, DXGI_PRESENT_TEST);
        if (hr == DXGI_STATUS_OCCLUDED) {
            MsgWaitForMultipleObjectsEx(0, nullptr, 100, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            return FrameResult::Skipped;
        }
        if (FAILED(hr)) return Classify("Present(TEST)", hr);
        state_.occluded = false;
    }
    if (latency_) {
        DWORD wait = MsgWaitForMultipleObjectsEx(1, &latency_, 100, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (wait == WAIT_FAILED) return Classify("Frame latency wait", HRESULT_FROM_WIN32(GetLastError()));
        if (wait != WAIT_OBJECT_0) return FrameResult::Skipped;
    }
    ImGui_ImplDX11_NewFrame();
    frameReady_ = true;
    return FrameResult::Ready;
}
FrameResult Dx11Renderer::RenderAndPresent(ImDrawData* data, const float clear[4]) {
    if (lost_) return FrameResult::DeviceLost;
    if (!frameReady_ || !data || !clear) return Classify("RenderAndPresent: BeginFrame required", E_UNEXPECTED);
    frameReady_ = false;
    // Flip Present unbinds back buffers: bind the RTV every frame.
    ID3D11RenderTargetView* target = target_.Get();
    context_->OMSetRenderTargets(1, &target, nullptr);
    context_->ClearRenderTargetView(target, clear);
    ImGui_ImplDX11_RenderDrawData(data);
    const HRESULT hr = swapchain_->Present(1, 0);
    if (hr == DXGI_STATUS_OCCLUDED) {
        state_.occluded = true;
        return FrameResult::Skipped;
    }
    return FAILED(hr) ? Classify("Present", hr) : FrameResult::Ready;
}
void Dx11Renderer::ReleaseGpu() noexcept {
    frameReady_ = false;
    if (imguiInitialized_) {
        ImGui_ImplDX11_Shutdown();
        imguiInitialized_ = false;
    }
    if (context_) {
        context_->ClearState();
        context_->Flush();
    }
    target_.Reset();
    if (latency_) { CloseHandle(latency_); latency_ = nullptr; }
    swapchain_.Reset();
    context_.Reset();
    device_.Reset();
    flip_ = false;
    swapFlags_ = 0;
}
void Dx11Renderer::Shutdown() noexcept {
    ReleaseGpu();
    window_ = nullptr;
    state_ = {};
    lost_ = false;
}
HRESULT Dx11Renderer::Recover() {
    if (!window_ || !ImGui::GetCurrentContext()) return Fail("Recover: no window/context", E_UNEXPECTED);
    ReleaseGpu();
    const HRESULT hr = CreateDeviceAndSwapChain();
    if (FAILED(hr)) { ReleaseGpu(); lost_ = true; }
    return hr;
}
} // namespace acid::rendering
