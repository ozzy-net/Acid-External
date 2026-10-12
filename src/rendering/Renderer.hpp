#pragma once
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
namespace acid {
class Renderer {
    template<class T> using ComPtr=Microsoft::WRL::ComPtr<T>;
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<IDXGISwapChain> swapchain_;
    ComPtr<ID3D11RenderTargetView> target_;
    HRESULT createTarget();
public:
    ~Renderer(){reset();}
    HRESULT initialize(HWND);
    HRESULT resize(unsigned width,unsigned height);
    HRESULT present(bool vsync);
    void draw();
    void reset();
    ID3D11Device* device()const{return device_.Get();}
    ID3D11DeviceContext* context()const{return context_.Get();}
    std::string adapter()const;
};
}
