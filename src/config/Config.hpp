#pragma once
#include <array>
#include <filesystem>
#include <string>
namespace acid {
struct Config {
    std::array<float, 3> accent{0.43f, 0.38f, 0.96f};
    std::array<float, 4> previewColor{0.43f, 0.55f, 1.f, 1.f};
    float radius = 120.f;
    float thickness = 2.f;
    float crosshairSize = 12.f;
    float background = 0.065f;
    bool circle = true;
    bool crosshair = true;
    bool animations = true;
    bool vsync = true;
    bool operator==(const Config&) const = default;
};
std::string serialize(const Config&);
bool parseConfig(const std::string&, Config&, std::string& error);
bool loadConfig(const std::filesystem::path&, Config&, std::string& error);
bool saveConfig(const std::filesystem::path&, const Config&, std::string& error);
}
