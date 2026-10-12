# Acid V2 integration review

## Contributions reviewed

| Contribution | Reviewed source | Integration decision |
| --- | --- | --- |
| Diagnostics, PR #1 | `083274d` | Integrated: bounded histories/logs, explicit unavailable counters, RAII report writes, current-process scope. |
| Premium UI | `2226ae9` | Integrated: responsive clipped UI and native controls; replaced its unchecked raw DirectX entry point and connected real diagnostics. |
| Rendering/performance | `aa2424a` | Integrated: COM ownership, deferred resize, flip-model/legacy fallback, latency pacing, loss recovery. Added hardware-to-WARP fallback and fixed smoke test consuming two latency waits before rendering. |
| Build QA | `43f66aa` | Matches main; retained foundation and expanded CI to Debug/Release plus integration tests. |
| Premium diagnostics, PR #2 | `db1618f` | Became ready during integration; reviewed and reused validated JSON configuration and application manifest. Competing UI/diagnostics/renderer implementations excluded to maintain one implementation per module. |
| Separate configuration / sixth agent | No published branch or PR at inspection | Pending. Integration supplies a small validated persistence module for existing UI settings. |

No agent branch is rewritten. Reviewed UI and rendering branch commits are retained
in integration merge history. PR #2 files were imported only after it became ready and were reviewed. The old raw
DirectX implementation is removed from the entry point rather than kept alongside
the renderer. No new third-party dependencies are introduced.

## Integration behavior

Diagnostics sampling occurs every 500 ms; bounded frame intervals exclude pauses
while minimized, occluded, or recovering the device. CPU retains the diagnostics
module's documented per-core convention. Missing values are displayed as unavailable.
Report failures are shown in the UI. Renderer failures preserve their HRESULT and
operation in diagnostic messages; unrecoverable failures return a nonzero exit code.
Settings reject malformed JSON, duplicate/unknown keys, oversized files, out-of-range and trailing data without partially applying
it. Saving checks writes/close and atomically replaces the previous file. Concurrent
application instances are not supported for shared settings writes.

## Validation and limits

The integration workspace is Linux, so native Windows runtime and DirectX checks
require the Windows CI runner. Portable tests exercise report generation, bounded
history, unsupported measurements, settings round-trip/rejection, renderer frame
state, and ImGui geometry across multiple window sizes and pages. Windows CI adds
real process-counter assertions, renderer initialization/resize/presentation/recovery,
and an application smoke test that renders twelve frames and resizes the window.
Assertions remain enabled for the UI smoke test in Release.

Manual Windows 10/11 checks remain necessary for visual appearance, mixed-DPI
monitors, minimize/restore and occlusion behavior, prolonged resource lifetime,
and actual driver device loss. Recovery smoke tests recreate a device deliberately;
they do not simulate every driver failure. No claim of exhaustive crash/leak freedom
is made. The custom chrome uses pixel dimensions and needs manual mixed-DPI review.
CI results and exact validated commit are recorded in the integration pull request.
