#include "config/settings.h"
#include <fstream>
#include <limits>
#include <stdexcept>
using namespace acid::config;
void check(bool value) { if (!value) throw std::runtime_error("Settings check failed"); }
int main() {
    const auto path = std::filesystem::path("settings-test.json");
    Settings a; a.radius = 205; a.circle = false; a.accent[1] = 0.75f;
    std::string error; check(Save(path, a, error));
    Settings b; check(Load(path, b, error));
    check(b.radius == a.radius && b.circle == a.circle && b.accent[1] == a.accent[1]);
    for (const char* invalid : {"{}", "{\"version\":2}", "{\"version\":1,\"radius\":999}",
         "{\"version\":1,\"circle\":2}", "{\"version\":1} trailing", "truncated",
         "{\"version\":1,\"version\":1}", "{\"version\":1,\"radius\":01}",
         "{\"version\":1,\"accent\":[1,2,3]}", "{\"version\":1,\"radius\":1e999}",
         "{\"version\":1,}"}) {
        { std::ofstream f(path); f << invalid; }
        check(!Load(path, b, error)); check(b.radius == 205 && !b.circle);
    }
    a.radius = std::numeric_limits<float>::quiet_NaN(); check(!Save(path, a, error));
    std::filesystem::remove(path);
}
