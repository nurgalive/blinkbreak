# BlinkBreak

A lightweight, cross-platform eye strain prevention application.

## Features

- Configurable short and long break reminders
- Non-intrusive overlay notifications
- System tray integration
- Multi-monitor support
- Idle detection (auto-pause when away)
- Do Not Disturb awareness

## Progress

- Stage 2 (State Machine Implementation): complete
  - Events and `StateMachine` implemented
  - Unit tests added for transitions, callbacks, and workflows
  - `main.cpp` demonstrates basic state transitions
- Stage 3 (Configuration System): complete
  - Configuration types, manager, and validation added
  - Default config JSON added under `resources/config`
  - Unit tests added for parsing, validation, and file IO
  - `main.cpp` demonstrates config load/validate flow

## Building

### Prerequisites

- CMake 3.25 or later
- Visual Studio 2022 (MSVC with C++23 support)

### Build Commands

Presets use the Visual Studio 17 2022 generator (multi-config).

```bash
# Configure (Debug)
cmake --preset=debug

# Configure clean
cmake --preset=debug --fresh

# Configure with Rust
cmake --preset=debug -DBLINKBREAK_ENABLE_SLINT=ON

# Build
cmake --build --preset=debug

# Configure with Rust (with explicit paths)
$toolchain = "$env:USERPROFILE\.rustup\toolchains\stable-x86_64-pc-windows-msvc\bin"
cmake --preset=debug --fresh `
  -DRust_COMPILER="$toolchain\rustc.exe" `
  -DRust_CARGO="$toolchain\cargo.exe"

# Configure with Rust (with explicit paths, only compiler)
cmake --preset=debug --fresh -DRust_COMPILER="C:\Users\azn\.rustup\toolchains\stable-x86_64-pc-windows-msvc\bin\rustc.exe"


# Run tests
ctest --preset=debug

# Configure and build (Release)
cmake --preset=release
cmake --build --preset=release
```

### Clangd support

Create a `compile_commands.json` for clangd:

```powershell
cmake -S . -B build/ninja-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY -DCMAKE_C_COMPILER="C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe" -DCMAKE_CXX_COMPILER="C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe"
```

Check that `compile_commands.json` is generated:

```powershell
ls build/ninja-debug/compile_commands.json
```

### Clang-Format

Dry run to check formatting:

```powershell
Get-ChildItem -Recurse -Include *.cpp,*.hpp src,tests,include | ForEach-Object { clang-format --dry-run -Werror $_.FullName }; clang-format --version
```

Apply formatting:

```powershell
Get-ChildItem -Recurse -Include *.cpp,*.hpp src,tests,include | ForEach-Object { clang-format -i $_.FullName }; clang-format --version
```

### Running

```bash
# Debug build (Visual Studio generator)
build/debug/src/Debug/blinkbreak.exe

# With version flag
build/debug/src/Debug/blinkbreak.exe --version
```

## Project Structure

- `src/` - Source code
  - `core/` - Core business logic
  - `ui/` - UI integration
  - `platform/` - Platform-specific code
- `include/` - Public headers
- `tests/` - Unit and integration tests
- `ui/` - Slint UI files
- `resources/` - Icons and assets

## License

MIT License
