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

`xf_create` currently creates a headless production native surface. Rendered
window hosting and event forwarding are the next integration step. This target
is consumed by `xframes-core`'s F# `SharedLibraryTransport`; it requires no Node.js
runtime. The old incremental wrapper source remains available as historical
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
vcpkg toolchain, and configure `XFRAMES_TRANSACTIONAL=ON`. The new target is written
portably; Windows is the platform verified so far. `XFRAMES_TRANSACTIONAL=OFF`
retains the historical build recipe and requires its historical XFrames version.

The F# integration check is run from `xframes-core`:

```powershell
dotnet run --project tests/XFrames.Core.Harness -c Release -- --native C:/dev/xframes-shared-c-library/build/transactions/xframesshared.dll
```

It exercises real creation, UTF-8, patch, reorder, destruction receipts, native
validation rejection, recreation and late-task fencing.

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
