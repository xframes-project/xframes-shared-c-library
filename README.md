# xframes-shared-c-library

Looking to integrate this a JVM language? Have a look at the [JNI project](https://github.com/xframes-project/xframes-jni-library).

## Building

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
