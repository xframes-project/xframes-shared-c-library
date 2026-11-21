# xframes-shared-c-library

Looking to integrate this a JVM language? Have a look at the [JNI project](https://github.com/xframes-project/xframes-jni-library).

## Building

### Current transactional C API

The default target now builds `include/xframes-transaction.h` against the current
XFrames schema-2 commit API. It provides opaque native handles, synchronous UTF-8
JSON publication/state queries and explicit destruction. `xf_abi_version()` is 1.
Returned strings are borrowed until the next call on the same handle; copy them
immediately. Serialize handle access, and do not destroy a handle during a call.
C++ exceptions are translated into null/error results, never propagated across
the C boundary. A null apply result is an unknown outcome, requiring quarantine.

`xf_create` creates a headless production native surface. The additive ABI-1
Windows extension `xf_create_window` creates an OpenGL window on its own native
thread and waits for initialization. Its JSON options are `title`, `width`,
`height`, `visible` and `diagnostics`. One rendered surface per process is
supported because the renderer uses process-global ImGui/GLFW state. It uses an
embedded default font and requires no external assets for basic controls.

`xf_poll_events` drains owned UTF-8 JSON events; native callbacks never enter the
managed caller directly. Events include `kind`, `nodeId`, `payload` and native
monotonic `occurredAtMs`. Button, text, numeric, boolean, combo, table, prefetch
and script-error callbacks are forwarded. `xf_frame` exposes frame/window
diagnostics. `xf_window_command` accepts `close`, `resize` (width/height) and
`screenshot` (path); screenshot completion includes its path and optional error.

The additive ABI-1 extension `xf_element_op` forwards one widget operation (a JSON
object with a string `op`, such as a table's or plot's `setData` or `appendData`)
to a live node. It returns once the widget has handled the operation, serialized
with commits, and reports malformed operations and unknown nodes through
`xf_last_error`. Operation names a widget does not know are ignored.

The engine is built from the `xframes` submodule, which follows `main`. Its
`virtual-table` element keeps rows in the application: it shows a window of rows
sent as props and reports the rows on screen as `tableRange` events (`first`,
`last`), alongside the existing `tableSort` and `tableRowClick` events. A
publication re-links only the parents whose child list changed, so the frame
after a small change does not lay out the whole tree again. A commit with
`"childLists": "changed"` carries only the child lists that changed: an omitted
list is kept, and a created node without one has no children. The engine then
validates and applies only what the commit names, without copying the tree.
`xf_state` lists `"changedChildLists"` in `commitFeatures`;
`"complete"`, the default, still requires every list.

On OS close, the renderer queues `closeRequested` and retains the native surface.
The owner must stop new work, drain publication and call `xf_destroy`, which
destroys graphics resources on the renderer thread and joins it. Rendering
failures also notify the owner before destruction. Event queues are bounded;
overflow reports `rendererError` and requests shutdown. A failed destruction
retains the handle and its dependent resources.

Window options are fully validated before graphics allocation. A construction
guard releases ImGui/ImPlot contexts if renderer construction fails; subsequent
initialization failures run cleanup on the owner thread. The integration harness
checks allocation balance and resource retention through real C-ABI calls.

`scripts/build-transactions.ps1 -EnableTestHooks` builds an opt-in diagnostic DLL
with private failure injection for initialization, rendering and teardown, plus
event overflow and graphics-allocation probes. Run `xframes-core`'s harness with
`--native-failures <dll>` to exercise those paths. These exports are absent from
ordinary builds. Rebuild without that switch before shipping; the Windows package
script rejects a DLL with test exports. An injected teardown failure happens
before native release, so the same handle and managed resources remain available
for an explicit retry.

From `xframes-core`, `./tools/check-native.ps1 -SharedLibraryRepository
C:/dev/xframes-shared-c-library -XFramesSource C:/dev/xframes -VcpkgInstalled
C:/dev/xframes/packages/dear-imgui/npm/node/build/vcpkg_installed
-SoftwareRendering` runs the full validation sequence. It restores a production
DLL after fault checks, verifies the absence of test exports, and retains source
hashes, logs, rendered screenshots and controlled contention measurements.
The software OpenGL dependency is pinned and isolated to the harness output;
omit that switch to test the installed graphics driver. The core repository's
Windows CI workflow calls this same script. Publish coordinated shared-library
changes before running it against a remote branch.

This target is consumed by `xframes-core`'s F# `SharedLibraryTransport` and
`RenderedWindow`; it requires no Node.js runtime. The old incremental wrapper
source remains available as historical
reference, including local edits, but is not compiled by the transactional target.

On Windows with Visual Studio C++ tools:

```powershell
./scripts/build-transactions.ps1 -XFramesSource C:/dev/xframes `
  -VcpkgInstalled C:/dev/xframes/packages/dear-imgui/npm/node/build/vcpkg_installed
```

Omit `-VcpkgInstalled` to install manifest dependencies. `-XFramesSource` defaults
to the `xframes` submodule, now pinned to
`2f5093ee93900b81540fb36b5ae34f2673f13fee`; initialize its required dependencies
before building. The explicit source option can use an existing initialized
checkout. All generated build files stay under `build/transactions`.

For CMake on other platforms, set `XFRAMES_SOURCE_DIR`, use the source checkout's
vcpkg toolchain, and configure `XFRAMES_TRANSACTIONAL=ON`. The headless target is
portable; rendered C-ABI hosting currently supports Windows only.
`XFRAMES_TRANSACTIONAL=OFF`
retains the historical build recipe and requires its historical XFrames version.

The F# integration check is run from `xframes-core`:

```powershell
dotnet run --project tests/XFrames.Core.Harness -c Release -- --native C:/dev/xframes-shared-c-library/build/transactions/xframesshared.dll
dotnet run --project tests/XFrames.Core.Harness -c Release -- --rendered C:/dev/xframes-shared-c-library/build/transactions/xframesshared.dll
```

It exercises real creation, UTF-8, patch, reorder, destruction receipts, native
validation rejection, recreation and late-task fencing. The rendered check adds
OpenGL submission, resize/suspend, screenshot, window ownership and close handoff.
`xframes-core/tools/package-windows.ps1` builds a self-contained Windows counter
and measures real button/text callbacks, submitted frames and OS-close shutdown.

### Windows (VS 2022)

`cmake --build ./build -- /property:Configuration=Release`


### Raspberry Pi

- `export VCPKG_FORCE_SYSTEM_BINARIES=1`
- `export ARM64_LINUX=1`

### RISC-V (Linux)

- `export VCPKG_FORCE_SYSTEM_BINARIES=1`
- `export RISCV64_LINUX=1`

## Screenshots

### Windows 11

![image](https://github.com/user-attachments/assets/b6a656f2-b4ca-4c64-8dfa-73bf37e44181)

### Ubuntu x64 (WSL2)

![image](https://github.com/user-attachments/assets/1a41824a-b6ad-41c2-aa4d-4e0317787a56)
