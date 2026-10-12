#include "diagnostics.hpp"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace acid::diagnostics {
namespace {
void quoted(std::ostream& out, const std::string& text) {
    out << '"';
    for (unsigned char c : text) {
        switch (c) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        default:
            if (c < 0x20) out << "\\u00" << "0123456789abcdef"[c >> 4] << "0123456789abcdef"[c & 15];
            else out << static_cast<char>(c);
        }
    }
    out << '"';
}
void number(std::ostream& out, std::optional<double> value) {
    if (value && std::isfinite(*value)) out << *value;
    else out << "null";
}
auto unix_ms(std::chrono::system_clock::time_point value) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(value.time_since_epoch()).count();
}
}
std::string report_json(const Snapshot& s) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(17) << "{\"schema_version\":1,\"timestamp_unix_ms\":" << unix_ms(s.timestamp)
        << ",\"cpu_percent_one_core\":";
    number(out, s.process.cpu_percent);
    out << ",\"uptime_seconds\":"; number(out, s.process.uptime_seconds);
    out << ",\"cpu_error\":" << s.process.cpu_error << ",\"memory_error\":" << s.process.memory_error << ",\"memory\":";
    if (s.process.memory) {
        const auto& m = *s.process.memory;
        out << "{\"working_set_bytes\":" << m.working_set_bytes << ",\"peak_working_set_bytes\":" << m.peak_working_set_bytes
            << ",\"private_bytes\":" << m.private_bytes << '}';
    } else out << "null";
    out << ",\"frame_times_ms\":[";
    bool first = true;
    for (float ms : s.frame_times_ms) {
        if (!first) out << ',';
        first = false;
        number(out, static_cast<double>(ms));
    }
    out << "],\"logs\":[";
    first = true;
    for (const auto& entry : s.logs) {
        if (!first) out << ',';
        first = false;
        out << "{\"timestamp_unix_ms\":" << unix_ms(entry.timestamp) << ",\"level\":";
        quoted(out, entry.level == LogLevel::error ? "error" : entry.level == LogLevel::warning ? "warning" : "info");
        out << ",\"message\":"; quoted(out, entry.message); out << '}';
    }
    out << "]}\n";
    return out.str();
}
void export_report(const Snapshot& s, const std::filesystem::path& destination) {
    const auto json = report_json(s);
    std::ofstream file;
    file.exceptions(std::ios::failbit | std::ios::badbit);
    file.open(destination, std::ios::binary | std::ios::trunc);
    file.write(json.data(), static_cast<std::streamsize>(json.size()));
    file.close();
}
} // namespace acid::diagnostics
