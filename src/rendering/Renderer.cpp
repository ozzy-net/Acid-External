#include "Renderer.hpp"
#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
namespace acid {
HRESULT Renderer::createTarget() {
    ComPtr<ID3D11Texture2D> buffer;HRESULT hr=swapchain_->GetBuffer(0,IID_PPV_ARGS(buffer.GetAddressOf()));
    if(FAILED(hr))return hr;return device_->CreateRenderTargetView(buffer.Get(),nullptr,target_.GetAddressOf());
}
HRESULT Renderer::initialize(HWND hwnd) {
    reset();DXGI_SWAP_CHAIN_DESC desc{};desc.BufferCount=2;desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.OutputWindow=hwnd;desc.SampleDesc.Count=1;desc.Windowed=TRUE;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
    constexpr D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_0};D3D_FEATURE_LEVEL selected{};
    HRESULT hr=D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,levels,1,D3D11_SDK_VERSION,&desc,swapchain_.GetAddressOf(),device_.GetAddressOf(),&selected,context_.GetAddressOf());
    if(FAILED(hr)){reset();hr=D3D11CreateDeviceAndSwapChain(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,levels,1,D3D11_SDK_VERSION,&desc,swapchain_.GetAddressOf(),device_.GetAddressOf(),&selected,context_.GetAddressOf());}
    if(FAILED(hr)){reset();return hr;}hr=createTarget();if(FAILED(hr))reset();return hr;
}
HRESULT Renderer::resize(unsigned width,unsigned height) {
    if(!width||!height)return S_OK;context_->OMSetRenderTargets(0,nullptr,nullptr);target_.Reset();
    HRESULT hr=swapchain_->ResizeBuffers(0,width,height,DXGI_FORMAT_UNKNOWN,0);if(FAILED(hr))return hr;return createTarget();
}
void Renderer::draw() {
    const float color[]={0.025f,0.03f,0.05f,1.f};auto* target=target_.Get();context_->OMSetRenderTargets(1,&target,nullptr);context_->ClearRenderTargetView(target,color);ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
HRESULT Renderer::present(bool vsync){return swapchain_->Present(vsync?1:0,0);}
void Renderer::reset(){target_.Reset();if(context_){context_->ClearState();context_->Flush();}swapchain_.Reset();context_.Reset();device_.Reset();}
std::string Renderer::adapter()const {
    ComPtr<IDXGIDevice> dxgi;ComPtr<IDXGIAdapter> adapter;
    if(!device_||FAILED(device_.As(&dxgi))||FAILED(dxgi->GetAdapter(adapter.GetAddressOf())))return "Adapter information unavailable";
    DXGI_ADAPTER_DESC desc{};if(FAILED(adapter->GetDesc(&desc)))return "Adapter information unavailable";
    const int size=WideCharToMultiByte(CP_UTF8,0,desc.Description,-1,nullptr,0,nullptr,nullptr);if(size<=1)return "Unknown adapter";
    std::string name(static_cast<std::size_t>(size),'\0');WideCharToMultiByte(CP_UTF8,0,desc.Description,-1,name.data(),size,nullptr,nullptr);name.pop_back();return name;
}
}
