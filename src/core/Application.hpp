#pragma once
#include "rendering/Renderer.hpp"
#include "config/Config.hpp"
#include "diagnostics/Diagnostics.hpp"
#include <windows.h>
namespace acid {
class Application {
    HINSTANCE instance_;
    HWND hwnd_=nullptr;
    Renderer renderer_;
    Config config_;
    Diagnostics diagnostics_;
    bool imgui_=false, win32Backend_=false, dx11Backend_=false, classRegistered_=false;
    unsigned width_=0,height_=0;
    bool minimized_=false;
    std::filesystem::path configPath_;
    static LRESULT CALLBACK procedure(HWND,UINT,WPARAM,LPARAM);
    LRESULT message(HWND,UINT,WPARAM,LPARAM);
    bool initialize(int show);
    bool recoverDevice();
    void loadFonts();
public:
    explicit Application(HINSTANCE instance):instance_(instance){}
    ~Application();
    int run(int show,bool smokeTest=false);
};
}
