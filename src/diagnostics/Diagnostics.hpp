#pragma once
#include "History.hpp"
#include <chrono>
#include <deque>
#include <filesystem>
#include <string>
#include <cstdint>
namespace acid {
class Diagnostics {
    using Clock=std::chrono::steady_clock;
    Clock::time_point start_=Clock::now(), previous_=start_, sampled_=start_;
    std::uint64_t lastCpu_=0;
    unsigned processors_=1;
public:
    History<240> frames, cpuHistory, memoryHistory;
    float frameMs=0, cpuPercent=0;
    double workingMiB=0, privateMiB=0;
    bool cpuAvailable=false, memoryAvailable=false;
    std::string system, gpu;
    std::deque<std::string> logs;
    Diagnostics();
    void tick();
    void resume();
    double uptime()const;
    void log(const std::string&);
    bool exportReport(const std::filesystem::path&,std::string& error)const;
};
}
