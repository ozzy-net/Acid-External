#pragma once
#include "Config.hpp"
namespace acid::config {
using Settings = acid::Config;
inline bool Load(const std::filesystem::path& path, Settings& settings, std::string& error) {
    return loadConfig(path, settings, error);
}
inline bool Save(const std::filesystem::path& path, const Settings& settings, std::string& error) {
    return saveConfig(path, settings, error);
}
}
