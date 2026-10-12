#pragma once
#include "config/Config.hpp"
#include "diagnostics/Diagnostics.hpp"
#include <windows.h>
#include <filesystem>
#include <array>
namespace acid {
class Interface {
    int tab_=0;
    float transition_=1;
    std::array<float,4> hover_{};
    std::string notice_;
    std::filesystem::path configPath_;
    Config& config_;
    Diagnostics& diagnostics_;
    void dashboard();
    void visualization();
    void diagnostics();
    void settings();
    void notify(const std::string&);
public:
    Interface(Config& config,Diagnostics& diagnostics,std::filesystem::path path):configPath_(std::move(path)),config_(config),diagnostics_(diagnostics){}
    void draw(HWND);
    void configureStyle();
};
}
