#pragma once
#include <chrono>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace acid::diagnostics {
struct MemoryStats {
    std::uint64_t working_set_bytes, peak_working_set_bytes, private_bytes;
};
struct ProcessStats {
    // 100% means one logical processor fully occupied; may exceed 100%.
    std::optional<double> cpu_percent;
    std::optional<MemoryStats> memory;
    std::optional<double> uptime_seconds;
    std::uint32_t cpu_error = 0, memory_error = 0;
};
enum class LogLevel { info, warning, error };
struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string message; // UTF-8
};
struct Snapshot {
    std::chrono::system_clock::time_point timestamp;
    ProcessStats process;
    std::vector<float> frame_times_ms; // oldest first, suitable for ImGui::PlotLines
    std::vector<LogEntry> logs;
};
// Single-thread owned. No background threads or ImGui dependencies.
class Diagnostics {
public:
    explicit Diagnostics(std::size_t frame_capacity = 240, std::size_t log_capacity = 500);
    ProcessStats sample_process();
    // First call establishes a baseline. Call once per rendered frame.
    void mark_frame();
    // Accepts an actual externally measured interval; rejects invalid values.
    bool record_frame(std::chrono::duration<double, std::milli> elapsed);
    void log(LogLevel level, std::string message);
    Snapshot snapshot() const;
private:
    std::size_t frame_capacity_, log_capacity_;
    std::deque<float> frames_;
    std::deque<LogEntry> logs_;
    ProcessStats process_;
    std::optional<std::chrono::steady_clock::time_point> last_frame_, last_cpu_time_;
    std::optional<std::uint64_t> last_cpu_ticks_;
};
// UTF-8 JSON, unavailable readings become null. Throws on I/O failure.
std::string report_json(const Snapshot& snapshot);
void export_report(const Snapshot& snapshot, const std::filesystem::path& destination);
} // namespace acid::diagnostics
