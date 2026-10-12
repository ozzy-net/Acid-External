#include "Application.hpp"
#include "ui/Interface.hpp"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include <windowsx.h>
#include <shlobj.h>
#include <algorithm>
#include <chrono>
#include <stdexcept>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
namespace acid {
namespace {
constexpr wchar_t ClassName[]=L"AcidDiagnosticsWindow";
std::filesystem::path preferences(){PWSTR path=nullptr;const HRESULT hr=SHGetKnownFolderPath(FOLDERID_LocalAppData,KF_FLAG_CREATE,nullptr,&path);if(FAILED(hr))throw std::runtime_error("Cannot locate LocalAppData");std::filesystem::path result(path);CoTaskMemFree(path);return result/L"AcidExternal"/L"config.json";}
void fatal(HWND hwnd,const wchar_t* message){MessageBoxW(hwnd,message,L"Acid External / initialization error",MB_OK|MB_ICONERROR);}
}
Application::~Application(){
    if(dx11Backend_)ImGui_ImplDX11_Shutdown();if(win32Backend_)ImGui_ImplWin32_Shutdown();if(imgui_)ImGui::DestroyContext();renderer_.reset();if(hwnd_&&IsWindow(hwnd_))DestroyWindow(hwnd_);if(classRegistered_)UnregisterClassW(ClassName,instance_);
}
LRESULT CALLBACK Application::procedure(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    auto* app=reinterpret_cast<Application*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE){app=static_cast<Application*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(app));}
    return app?app->message(hwnd,msg,wp,lp):DefWindowProcW(hwnd,msg,wp,lp);
}
LRESULT Application::message(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    // Nonclient messages remain native, allowing keyboard system menu, resizing, snapping and dragging.
    switch(msg){
    case WM_NCCALCSIZE:if(wp){auto* rect=reinterpret_cast<NCCALCSIZE_PARAMS*>(lp);if(IsZoomed(hwnd)){MONITORINFO monitor{sizeof(monitor)};if(GetMonitorInfoW(MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST),&monitor))rect->rgrc[0]=monitor.rcWork;}return 0;}break;
    case WM_NCHITTEST:{POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};ScreenToClient(hwnd,&p);RECT r{};GetClientRect(hwnd,&r);const float dpi=static_cast<float>(GetDpiForWindow(hwnd))/96.f;const int edge=static_cast<int>(8*dpi);
        if(!IsZoomed(hwnd)){const bool l=p.x<edge,t=p.y<edge,rr=p.x>=r.right-edge,b=p.y>=r.bottom-edge;if(t&&l)return HTTOPLEFT;if(t&&rr)return HTTOPRIGHT;if(b&&l)return HTBOTTOMLEFT;if(b&&rr)return HTBOTTOMRIGHT;if(l)return HTLEFT;if(rr)return HTRIGHT;if(t)return HTTOP;if(b)return HTBOTTOM;}
        if(p.y<static_cast<int>(48*dpi)&&p.x<r.right-static_cast<int>(144*dpi))return HTCAPTION;return HTCLIENT;}
    case WM_GETMINMAXINFO:{auto* limits=reinterpret_cast<MINMAXINFO*>(lp);const UINT dpi=GetDpiForWindow(hwnd);limits->ptMinTrackSize={MulDiv(900,static_cast<int>(dpi),96),MulDiv(680,static_cast<int>(dpi),96)};return 0;}
    case WM_SIZE:minimized_=wp==SIZE_MINIMIZED;if(!minimized_){width_=LOWORD(lp);height_=HIWORD(lp);}return 0;
    case WM_DPICHANGED:{auto* r=reinterpret_cast<RECT*>(lp);SetWindowPos(hwnd,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);if(imgui_)ImGui::GetIO().FontGlobalScale=static_cast<float>(HIWORD(wp))/96.f;return 0;}
    case WM_SYSCOMMAND:if((wp&0xfff0)==SC_KEYMENU && lp==0)return 0;break;
    case WM_CLOSE:DestroyWindow(hwnd);return 0;
    case WM_DESTROY:PostQuitMessage(0);return 0;
    }
    if(imgui_&&win32Backend_&&ImGui_ImplWin32_WndProcHandler(hwnd,msg,wp,lp))return 1;
    return DefWindowProcW(hwnd,msg,wp,lp);
}
void Application::loadFonts(){
    wchar_t windows[MAX_PATH]{};GetWindowsDirectoryW(windows,MAX_PATH);auto path=std::filesystem::path(windows)/L"Fonts"/L"segoeui.ttf";auto& io=ImGui::GetIO();
    if(std::filesystem::exists(path)){if(!io.Fonts->AddFontFromFileTTF(path.string().c_str(),17.f))io.Fonts->AddFontDefault();}else io.Fonts->AddFontDefault();io.FontGlobalScale=static_cast<float>(GetDpiForWindow(hwnd_))/96.f;
}
bool Application::initialize(int show){
    ImGui_ImplWin32_EnableDpiAwareness();
    configPath_=preferences();std::string error;if(std::filesystem::exists(configPath_)&&!loadConfig(configPath_,config_,error))diagnostics_.log("Configuration rejected: "+error);
    WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.style=CS_HREDRAW|CS_VREDRAW;wc.lpfnWndProc=procedure;wc.hInstance=instance_;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.lpszClassName=ClassName;
    classRegistered_=RegisterClassExW(&wc)!=0;if(!classRegistered_){fatal(nullptr,L"Window class registration failed.");return false;}
    hwnd_=CreateWindowExW(0,ClassName,L"Acid External V2 / Diagnostics",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1180,820,nullptr,nullptr,instance_,this);
    if(!hwnd_){fatal(nullptr,L"Application window creation failed.");return false;}
    SetWindowPos(hwnd_,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
    if(FAILED(renderer_.initialize(hwnd_))){fatal(hwnd_,L"DirectX 11 initialization failed for both hardware and software rendering.");return false;}
    IMGUI_CHECKVERSION();ImGui::CreateContext();imgui_=true;auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;loadFonts();
    win32Backend_=ImGui_ImplWin32_Init(hwnd_);if(!win32Backend_){fatal(hwnd_,L"Win32 UI backend initialization failed.");return false;}
    dx11Backend_=ImGui_ImplDX11_Init(renderer_.device(),renderer_.context());if(!dx11Backend_){fatal(hwnd_,L"DirectX UI backend initialization failed.");return false;}
    diagnostics_.gpu=renderer_.adapter();diagnostics_.log("DirectX renderer ready: "+diagnostics_.gpu);ShowWindow(hwnd_,show);UpdateWindow(hwnd_);return true;
}
bool Application::recoverDevice(){
    diagnostics_.log("Graphics device lost; recreating renderer.");if(dx11Backend_){ImGui_ImplDX11_Shutdown();dx11Backend_=false;}if(FAILED(renderer_.initialize(hwnd_)))return false;
    dx11Backend_=ImGui_ImplDX11_Init(renderer_.device(),renderer_.context());diagnostics_.gpu=renderer_.adapter();diagnostics_.resume();return dx11Backend_;
}
int Application::run(int show,bool smokeTest){
    if(!initialize(show))return 1;diagnostics_.resume();Interface ui(config_,diagnostics_,configPath_);bool running=true,occluded=false,paused=false;int exitCode=0;unsigned frames=0;
    auto smokeStart=std::chrono::steady_clock::now();
    while(running){const auto frameStart=std::chrono::steady_clock::now();MSG msg{};while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT)running=false;TranslateMessage(&msg);DispatchMessageW(&msg);}if(!running)break;
        if(smokeTest&&std::chrono::steady_clock::now()-smokeStart>std::chrono::seconds(20)){exitCode=2;break;}
        if(minimized_){MsgWaitForMultipleObjectsEx(0,nullptr,100,QS_ALLINPUT,MWMO_INPUTAVAILABLE);paused=true;continue;}
        if(occluded){const HRESULT hr=renderer_.present(true);if(hr==DXGI_STATUS_OCCLUDED){MsgWaitForMultipleObjectsEx(0,nullptr,100,QS_ALLINPUT,MWMO_INPUTAVAILABLE);paused=true;continue;}if(FAILED(hr)&&!recoverDevice()){exitCode=1;break;}occluded=false;}
        if(paused){diagnostics_.resume();paused=false;}
        if(width_&&height_){const HRESULT hr=renderer_.resize(width_,height_);width_=height_=0;if(FAILED(hr)&&!recoverDevice()){diagnostics_.log("Resize and renderer recovery failed.");exitCode=1;break;}}
        diagnostics_.tick();ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();ImGui::NewFrame();ui.draw(hwnd_);ImGui::Render();renderer_.draw();
        const HRESULT hr=renderer_.present(config_.vsync);if(hr==DXGI_STATUS_OCCLUDED)occluded=true;else if(FAILED(hr)&&!recoverDevice()){diagnostics_.log("Presentation and renderer recovery failed.");exitCode=1;break;}
        if(!config_.vsync){const auto elapsed=std::chrono::steady_clock::now()-frameStart;const auto remaining=std::chrono::milliseconds(4)-std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);if(remaining.count()>0)MsgWaitForMultipleObjectsEx(0,nullptr,static_cast<DWORD>(remaining.count()),QS_ALLINPUT,MWMO_INPUTAVAILABLE);}
        if(smokeTest){++frames;if(frames==2)SetWindowPos(hwnd_,nullptr,0,0,1024,768,SWP_NOMOVE|SWP_NOZORDER);if(frames>=12)break;}
    }
    if(!smokeTest){std::string error;if(!saveConfig(configPath_,config_,error)){diagnostics_.log("Could not save preferences: "+error);MessageBoxA(IsWindow(hwnd_)?hwnd_:nullptr,error.c_str(),"Acid External / configuration save failed",MB_OK|MB_ICONWARNING);}}
    if(exitCode)fatal(IsWindow(hwnd_)?hwnd_:nullptr,L"The graphics renderer could not continue. Restart the application.");return exitCode;
}
}
