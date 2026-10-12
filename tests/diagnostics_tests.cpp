#include "diagnostics/diagnostics.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <thread>
#include <stdexcept>
using namespace acid::diagnostics;
void check(bool value) { if (!value) throw std::runtime_error("check failed"); }
int main(int argc, char** argv) {
    try {
        Diagnostics d(2, 2);
        d.mark_frame();
        check(d.snapshot().frame_times_ms.empty());
        check(!d.record_frame(std::chrono::milliseconds(0)));
        check(!d.record_frame(std::chrono::milliseconds(-1)));
        check(!d.record_frame(std::chrono::duration<double, std::milli>(std::numeric_limits<double>::infinity())));
        check(!d.record_frame(std::chrono::duration<double, std::milli>(std::numeric_limits<double>::quiet_NaN())));
        for (int ms : {1, 2, 3}) check(d.record_frame(std::chrono::milliseconds(ms)));
        check(d.snapshot().frame_times_ms == std::vector<float>({2, 3}));
        d.log(LogLevel::info, "discard");
        d.log(LogLevel::warning, "quote\" slash\\ line\n\t");
        d.log(LogLevel::error, "UTF-8: \xc3\xa9");
        auto s = d.snapshot();
        check(s.logs.size() == 2 && s.logs.front().level == LogLevel::warning);
        d.log(LogLevel::info, "later");
        check(s.logs.back().level == LogLevel::error);
        const auto json = report_json(s);
        check(json.find("\"memory\":null") != std::string::npos);
        check(json.find("quote\\\" slash\\\\ line\\u000a\\u0009") != std::string::npos);
        Diagnostics disabled(0, 0);
        disabled.record_frame(std::chrono::milliseconds(1));
        disabled.log(LogLevel::info, "ignored");
        check(disabled.snapshot().logs.empty() && disabled.snapshot().frame_times_ms.empty());
        if (argc > 1) {
            export_report(s, argv[1]);
            std::ifstream file(argv[1], std::ios::binary);
            std::string actual((std::istreambuf_iterator<char>(file)), {});
            check(actual == json);
            bool failed = false;
            try { export_report(s, std::filesystem::path(argv[1]) / "missing" / "report.json"); }
            catch (const std::ios_base::failure&) { failed = true; }
            check(failed);
        }
#ifdef _WIN32
        const auto initial = d.sample_process();
        check(!initial.cpu_percent && initial.memory && initial.uptime_seconds);
        check(initial.memory->working_set_bytes > 0 && initial.memory->private_bytes > 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        const auto next = d.sample_process();
        check(next.cpu_percent && *next.cpu_percent >= 0 && next.cpu_error == 0 && next.memory_error == 0);
        check(next.uptime_seconds && *next.uptime_seconds >= *initial.uptime_seconds);
#else
        check(!d.sample_process().memory && !d.snapshot().process.cpu_percent);
#endif
        std::cout << "Diagnostics tests passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
