# Diagnostics integration

`acid_diagnostics` is a C++20 library with no UI dependencies. It is linked into
AcidExternal but not called by the application yet; `src/main.cpp` is unchanged.
The Windows implementation measures the hosting process only, using
`GetProcessTimes` and `GetProcessMemoryInfo`. It never opens another process.
The current-process pseudo-handle is borrowed and needs no cleanup. Streams use
RAII and export explicitly closes the stream to detect write/close failures.

```cpp
#include "diagnostics/diagnostics.hpp"
acid::diagnostics::Diagnostics diagnostics;
// Once per frame at the same point in the rendering loop:
diagnostics.mark_frame();
// Sample periodically (e.g. every 250-1000 ms), independently of frame timing:
auto stats = diagnostics.sample_process();
diagnostics.log(acid::diagnostics::LogLevel::info, "Renderer initialized");
auto view = diagnostics.snapshot();
// Future ImGui integration:
// ImGui::PlotLines("Frame ms", view.frame_times_ms.data(),
//                  static_cast<int>(view.frame_times_ms.size()));
acid::diagnostics::export_report(view, L"diagnostics.json");
```

CPU is `(kernel + user time delta) / steady elapsed time * 100`, where 100%
means one logical CPU fully occupied. It can exceed 100% on a multicore process.
The first sample (and first sample following a failed query) has no CPU value.
Sample over meaningful intervals, not every frame. Working set is resident
memory, peak working set is the lifetime peak, and private bytes are private
committed memory, not resident memory. All memory fields are bytes.
Uptime is seconds since OS-reported process creation, including time before this
module is constructed. It uses Windows wall-clock FILETIME and may change if the
system clock is adjusted. CPU/frame elapsed times use a monotonic clock.

Missing optional values mean unavailable, never zero-filled fake readings.
`cpu_error` and `memory_error` hold immediate Win32 errors; zero means query
success. Snapshot process readings are from the last explicit sample, while the
snapshot timestamp is capture time. Calling snapshot does not query Windows.
On non-Windows hosts the portable components work, but process readings remain
unavailable with ERROR_NOT_SUPPORTED (50).

Frame history contains real consecutive mark intervals in milliseconds, oldest
first. The first mark only establishes a baseline. Alternatively pass a measured
interval to `record_frame`; do not use both paths for the same frame. Nonpositive,
nonfinite and unrepresentable intervals are rejected. Default bounds are 240
frames and 500 log entries; zero capacity disables retention. Logs use UTC Unix
millisecond timestamps and UTF-8 messages. Supply valid UTF-8. Logs are retained
in memory and persisted by report export.

Use a Diagnostics instance from one thread, or protect it with an external mutex.
Snapshots own copies, so the UI/export can safely retain them. JSON schema v1
includes units in field names, explicit null readings, errors, frames and logs.
Export overwrites the chosen path, creates no parent directories, and throws
`std::ios_base::failure` on I/O errors; catch this at the UI boundary. Export is
synchronous and not atomic; a failed write may leave a partial report. Use a
worker with an owned snapshot if exporting from a responsive UI.

## Tests

Full Windows build: `ctest --test-dir build -C Release --output-on-failure`.
Standalone Windows build without ImGui download:

```powershell
cmake -S src/diagnostics -B build-diagnostics -A x64
cmake --build build-diagnostics --config Release
ctest --test-dir build-diagnostics -C Release --output-on-failure
```

Portable history/JSON/export tests can also be built on Linux with:

```sh
g++ -std=c++20 -Wall -Wextra -Werror -Isrc src/diagnostics/diagnostics.cpp src/diagnostics/report.cpp tests/diagnostics_tests.cpp -o /tmp/diagnostics_tests
/tmp/diagnostics_tests /tmp/diagnostics-report.json
```

Windows tests query real process counters; they do not assert arbitrary CPU
loads or exact memory sizes. Portable tests cover eviction, invalid intervals,
zero capacities, snapshot ownership, JSON escaping, export and I/O failures.
