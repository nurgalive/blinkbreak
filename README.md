# BlinkBreak

<!--toc:start-->

- [BlinkBreak](#blinkbreak)
  - [Features](#features)
  - [Progress](#progress)
  - [Building](#building)
    - [Prerequisites](#prerequisites)
    - [Build Commands](#build-commands)
    - [Clangd support](#clangd-support)
    - [Clang-Format](#clang-format)
    - [Running](#running)
  - [Project Structure](#project-structure)
  - [License](#license)
  <!--toc:end-->

A lightweight, cross-platform eye strain prevention application.

## Features

- Configurable short and long break reminders
- Non-intrusive overlay notifications
- System tray integration
- Multi-monitor support
- Idle detection (auto-pause when away)
- Do Not Disturb awareness

## Progress

- Stage 1 (Project Foundation): complete
  - Build system with CMake and Visual Studio 2022
  - Basic `Timer` class with full test coverage
  - Code formatting with clang-format
- Stage 2 (State Machine Implementation): complete
  - Events and `StateMachine` implemented
  - Unit tests added for transitions, callbacks, and workflows
  - `main.cpp` demonstrates basic state transitions
- Stage 3 (Configuration System): complete
  - Configuration types, manager, and validation added
  - Default config JSON added under `resources/config`
  - Unit tests added for parsing, validation, and file IO
  - `main.cpp` demonstrates config load/validate flow
- Stage 4 (Message Provider and Break Scheduler): complete
  - `MessageProvider` with sequential and random rotation
  - `BreakScheduler` for coordinating short and long breaks
  - Full unit test coverage (25+ new tests)
  - 80 tests total passing
- Stage 5 (Basic Slint UI): complete
  - Slint UI framework integration
  - Main window with timer display and controls
  - `AppController` orchestrating UI and core logic
  - Timer thread with periodic updates
- Stage 6 (System Tray Integration): complete
  - Platform abstraction layer for cross-platform support
  - Windows tray icon implementation with Win32 API
  - `TrayManager` with context menu (Show, Start/Pause, Skip, Settings, Quit)
  - Tooltip status updates showing time until break
  - Full integration with `AppController`
  - Bug fix #1: Menu now updates correctly when resuming from paused state
  - Bug fix #2: Window minimizes to tray on close instead of quitting app

## Building

### Prerequisites

- CMake 3.25 or later
- Visual Studio 2022 (MSVC with C++23 support)
- Rust 19.0 or later (optional, for Slint UI integration)
- Slint 1.15 or later (optional, for UI development)
- Clang-Format 15 or later (optional, for code formatting)
- Gtest 1.17 or later (for unit testing)
- Spdlog 1.17 or later (for logging)

### Build Commands

Presets use the Visual Studio 17 2022 generator (multi-config).

```bash
# Configure (Debug)
cmake --preset=debug --fresh

# Configure with Rust (with explicit paths)
$toolchain = "$env:USERPROFILE\.rustup\toolchains\stable-x86_64-pc-windows-msvc\bin"
cmake --preset=debug --fresh `
  -DRust_COMPILER="$toolchain\rustc.exe" `
  -DRust_CARGO="$toolchain\cargo.exe"

# Build
cmake --build --preset=debug

# Run tests
ctest --preset=debug

# Run tests verbosely
ctest --preset=debug --output-on-failure -V

# Save test output to file
ctest --preset=debug -V > test_output.txt

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
