#include "diagnostics.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace acid::diagnostics {
Diagnostics::Diagnostics(std::size_t frames, std::size_t logs)
    : frame_capacity_(frames), log_capacity_(logs) {}

ProcessStats Diagnostics::sample_process() {
    process_ = {};
#ifdef _WIN32
    // GetCurrentProcess returns a borrowed pseudo-handle: never CloseHandle it.
    const HANDLE process = GetCurrentProcess();
    FILETIME creation{}, exit{}, kernel{}, user{};
    const auto ticks = [](FILETIME value) {
        return (static_cast<std::uint64_t>(value.dwHighDateTime) << 32) | value.dwLowDateTime;
    };
    if (GetProcessTimes(process, &creation, &exit, &kernel, &user)) {
        const auto now = std::chrono::steady_clock::now();
        const auto total = ticks(kernel) + ticks(user);
        if (last_cpu_ticks_ && last_cpu_time_ && total >= *last_cpu_ticks_) {
            const double seconds = std::chrono::duration<double>(now - *last_cpu_time_).count();
            if (seconds > 0)
                process_.cpu_percent = static_cast<double>(total - *last_cpu_ticks_) / 10000000.0 / seconds * 100.0;
        }
        last_cpu_ticks_ = total;
        last_cpu_time_ = now;
        FILETIME wall{};
        GetSystemTimeAsFileTime(&wall);
        if (ticks(wall) >= ticks(creation))
            process_.uptime_seconds = static_cast<double>(ticks(wall) - ticks(creation)) / 10000000.0;
    } else {
        process_.cpu_error = GetLastError();
        last_cpu_ticks_.reset();
        last_cpu_time_.reset();
    }
    PROCESS_MEMORY_COUNTERS_EX memory{};
    memory.cb = sizeof(memory);
    if (GetProcessMemoryInfo(process, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory))) {
        process_.memory = MemoryStats{memory.WorkingSetSize, memory.PeakWorkingSetSize, memory.PrivateUsage};
    } else {
        process_.memory_error = GetLastError();
    }
#else
    // Portable history/report tests only; never synthesize process readings.
    process_.cpu_error = process_.memory_error = 50; // ERROR_NOT_SUPPORTED
#endif
    return process_;
}
void Diagnostics::mark_frame() {
    const auto now = std::chrono::steady_clock::now();
    if (last_frame_) record_frame(now - *last_frame_);
    last_frame_ = now;
}
bool Diagnostics::record_frame(std::chrono::duration<double, std::milli> elapsed) {
    const double ms = elapsed.count();
    if (!std::isfinite(ms) || ms <= 0 || ms > std::numeric_limits<float>::max()) return false;
    if (frame_capacity_) {
        if (frames_.size() == frame_capacity_) frames_.pop_front();
        frames_.push_back(static_cast<float>(ms));
    }
    return true;
}
void Diagnostics::log(LogLevel level, std::string message) {
    if (!log_capacity_) return;
    if (logs_.size() == log_capacity_) logs_.pop_front();
    logs_.push_back({std::chrono::system_clock::now(), level, std::move(message)});
}
Snapshot Diagnostics::snapshot() const {
    return {std::chrono::system_clock::now(), process_, {frames_.begin(), frames_.end()}, {logs_.begin(), logs_.end()}};
}
} // namespace acid::diagnostics
