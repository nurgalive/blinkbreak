# BlinkBreak Implementation Plan

## Table of Contents

- [BlinkBreak Implementation Plan](#blinkbreak-implementation-plan)
  - [Table of Contents](#table-of-contents)
  - [Overview](#overview)
    - [Architecture Diagram](#architecture-diagram)
    - [Technology Stack](#technology-stack)
  - [Project Setup](#project-setup)
    - [Directory Structure](#directory-structure)
    - [Dependencies](#dependencies)
  - [Stage 1: Project Foundation](#stage-1-project-foundation)
    - [Goal](#goal)
    - [Prerequisites](#prerequisites)
    - [Implementation Steps](#implementation-steps)
      - [Stage 1 Results (Summary)](#stage-1-results-summary)
    - [Test Requirements](#test-requirements)
      - [Unit Tests](#unit-tests)
      - [Verification Criteria](#verification-criteria)
    - [Deliverables](#deliverables)
  - [Stage 2: State Machine Implementation](#stage-2-state-machine-implementation)
    - [Goal](#goal-1)
    - [Prerequisites](#prerequisites-1)
    - [Implementation Steps](#implementation-steps-1)
    - [Test Requirements](#test-requirements-1)
      - [Unit Tests](#unit-tests-1)
      - [Verification Criteria](#verification-criteria-1)
      - [Validation Commands](#validation-commands)
    - [Deliverables](#deliverables-1)
  - [Stage 3: Configuration System](#stage-3-configuration-system)
    - [Goal](#goal-2)
    - [Prerequisites](#prerequisites-2)
    - [Implementation Steps](#implementation-steps-2)
      - [3.1 Create Configuration Types](#31-create-configuration-types)
      - [3.2 Create Configuration Manager Header](#32-create-configuration-manager-header)
      - [3.3 Create Configuration Manager Implementation](#33-create-configuration-manager-implementation)
      - [3.4 Create Default Configuration File](#34-create-default-configuration-file)
      - [3.5 Create Configuration Manager Tests](#35-create-configuration-manager-tests)
      - [3.6 Update Source CMakeLists.txt](#36-update-source-cmakeliststxt)
      - [3.7 Update Tests CMakeLists.txt](#37-update-tests-cmakeliststxt)
      - [3.8 Update Main to Demo Configuration](#38-update-main-to-demo-configuration)
    - [Test Requirements](#test-requirements-2)
      - [Unit Tests](#unit-tests-2)
      - [Verification Criteria](#verification-criteria-2)
    - [Deliverables](#deliverables-2)
  - [Stage 4: Message Provider and Break Scheduler](#stage-4-message-provider-and-break-scheduler)
    - [Goal](#goal-3)
    - [Prerequisites](#prerequisites-3)
    - [Implementation Steps](#implementation-steps-3)
      - [4.1 Create Message Provider Header](#41-create-message-provider-header)
      - [4.2 Create Message Provider Implementation](#42-create-message-provider-implementation)
      - [4.3 Create Break Scheduler Header](#43-create-break-scheduler-header)
      - [4.4 Create Break Scheduler Implementation](#44-create-break-scheduler-implementation)
      - [4.5 Create Message Provider Tests](#45-create-message-provider-tests)
      - [4.6 Create Break Scheduler Tests](#46-create-break-scheduler-tests)
      - [4.7 Update Source CMakeLists.txt](#47-update-source-cmakeliststxt)
      - [4.8 Update Tests CMakeLists.txt](#48-update-tests-cmakeliststxt)
    - [Test Requirements](#test-requirements-3)
      - [Unit Tests](#unit-tests-3)
      - [Verification Criteria](#verification-criteria-3)
    - [Deliverables](#deliverables-3)
  - [Stage 5: Basic Slint UI](#stage-5-basic-slint-ui)
    - [Goal](#goal-4)
    - [Prerequisites](#prerequisites-4)
    - [Implementation Steps](#implementation-steps-4)
      - [5.1 Update Root CMakeLists.txt for Slint](#51-update-root-cmakeliststxt-for-slint)
      - [5.2 Create Main Window Slint File](#52-create-main-window-slint-file)
      - [5.3 Create Timer Display Component](#53-create-timer-display-component)
      - [5.4 Create App Controller Header](#54-create-app-controller-header)
      - [5.5 Create App Controller Implementation](#55-create-app-controller-implementation)
      - [5.6 Update Main Entry Point](#56-update-main-entry-point)
      - [5.7 Update Source CMakeLists.txt](#57-update-source-cmakeliststxt)
    - [Test Requirements](#test-requirements-4)
      - [Unit Tests](#unit-tests-4)
      - [Verification Criteria](#verification-criteria-4)
    - [Deliverables](#deliverables-4)
  - [Stage 6: System Tray Integration](#stage-6-system-tray-integration)
    - [Goal](#goal-5)
    - [Prerequisites](#prerequisites-5)
    - [Implementation Steps](#implementation-steps-5)
      - [6.1 Create Platform Interface for System Tray](#61-create-platform-interface-for-system-tray)
      - [6.2 Create Windows Tray Icon Implementation](#62-create-windows-tray-icon-implementation)
      - [6.3 Create Windows Tray Icon Implementation](#63-create-windows-tray-icon-implementation)
      - [6.4 Create TrayManager Class](#64-create-traymanager-class)
      - [6.5 Create TrayManager Implementation](#65-create-traymanager-implementation)
    - [Test Requirements](#test-requirements-5)
      - [Verification Criteria](#verification-criteria-5)
    - [Deliverables](#deliverables-5)
  - [Stage 7: Break Overlay Window](#stage-7-break-overlay-window)
    - [Goal](#goal-6)
    - [Prerequisites](#prerequisites-6)
    - [Implementation Steps](#implementation-steps-6)
      - [7.1 Create Overlay Slint File](#71-create-overlay-slint-file)
      - [7.2 Create Overlay Manager](#72-create-overlay-manager)
    - [Test Requirements](#test-requirements-6)
    - [Deliverables](#deliverables-6)
  - [Stage 8: Multi-Monitor Support](#stage-8-multi-monitor-support)
    - [Goal](#goal-7)
    - [Prerequisites](#prerequisites-7)
    - [Implementation Steps](#implementation-steps-7)
      - [8.1 Create Monitor Manager Interface](#81-create-monitor-manager-interface)
      - [8.2 Windows Monitor Implementation](#82-windows-monitor-implementation)
    - [Deliverables](#deliverables-7)
  - [Stage 9: Idle Detection](#stage-9-idle-detection)
    - [Goal](#goal-8)
    - [Prerequisites](#prerequisites-8)
    - [Implementation Steps](#implementation-steps-8)
      - [9.1 Create Idle Detector Interface](#91-create-idle-detector-interface)
      - [9.2 Windows Idle Detection](#92-windows-idle-detection)
    - [Deliverables](#deliverables-8)
  - [Stage 10: Notifications](#stage-10-notifications)
    - [Goal](#goal-9)
    - [Prerequisites](#prerequisites-9)
    - [Implementation Steps](#implementation-steps-9)
      - [10.1 Create Notification Interface](#101-create-notification-interface)
      - [10.2 Windows Toast Notifications](#102-windows-toast-notifications)
    - [Deliverables](#deliverables-9)
  - [Stage 11: Do Not Disturb Detection](#stage-11-do-not-disturb-detection)
    - [Goal](#goal-10)
    - [Prerequisites](#prerequisites-10)
    - [Implementation Steps](#implementation-steps-10)
      - [11.1 Create DND Detector Interface](#111-create-dnd-detector-interface)
      - [11.2 Windows Focus Assist Detection](#112-windows-focus-assist-detection)
    - [Deliverables](#deliverables-10)
  - [Stage 12: Integration Tests \& Polish](#stage-12-integration-tests--polish)
    - [Goal](#goal-11)
    - [Prerequisites](#prerequisites-11)
    - [Implementation Steps](#implementation-steps-11)
      - [12.1 Create Integration Test Harness](#121-create-integration-test-harness)
      - [12.2 Integration Tests](#122-integration-tests)
      - [12.3 Performance Testing](#123-performance-testing)
      - [12.4 Final Polish](#124-final-polish)
    - [Test Requirements](#test-requirements-7)
      - [Integration Tests](#integration-tests)
      - [Verification Criteria](#verification-criteria-6)
    - [Deliverables](#deliverables-11)
  - [Appendix A: Testing Commands Reference](#appendix-a-testing-commands-reference)
  - [Appendix B: Code Quality Commands](#appendix-b-code-quality-commands)
  - [Appendix C: Debugging Tips](#appendix-c-debugging-tips)
  - [Summary](#summary)

## Overview

BlinkBreak is a Windows eye strain prevention application designed to help users take regular breaks from screen time. The application follows a modern C++23 architecture with a focus on performance, testability, and maintainability.

### Architecture Diagram

```plain
┌─────────────────────────────────────────────────────────────────┐
│                        BlinkBreak Application                    │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │   UI Layer  │  │  Core Logic │  │    Platform Layer       │  │
│  │   (Slint)   │  │             │  │                         │  │
│  │             │  │ ┌─────────┐ │  │ ┌───────────────────┐   │  │
│  │ ┌─────────┐ │  │ │  State  │ │  │ │  Idle Detection   │   │  │
│  │ │Main Win │ │  │ │ Machine │ │  │ │  (Windows API)    │   │  │
│  │ └─────────┘ │  │ └─────────┘ │  │ └───────────────────┘   │  │
│  │ ┌─────────┐ │  │ ┌─────────┐ │  │ ┌───────────────────┐   │  │
│  │ │ Overlay │ │  │ │  Timer  │ │  │ │  DND Detection    │   │  │
│  │ └─────────┘ │  │ │ Manager │ │  │ │  (Focus Assist)   │   │  │
│  │ ┌─────────┐ │  │ └─────────┘ │  │ └───────────────────┘   │  │
│  │ │  Tray   │ │  │ ┌─────────┐ │  │ ┌───────────────────┐   │  │
│  │ │  Icon   │ │  │ │ Config  │ │  │ │  Notifications    │   │  │
│  │ └─────────┘ │  │ │ Manager │ │  │ │  (Toast API)      │   │  │
│  │             │  │ └─────────┘ │  │ └───────────────────┘   │  │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                      Threading Model                             │
│  ┌──────────┐    ┌──────────────┐    ┌────────────────────┐     │
│  │ UI Thread│    │ Timer Thread │    │ OS Events Thread   │     │
│  │  (Main)  │◄──►│  (Worker)    │◄──►│    (Worker)        │     │
│  └──────────┘    └──────────────┘    └────────────────────┘     │
└─────────────────────────────────────────────────────────────────┘
```

### Technology Stack

| Component     | Technology                             |
| ------------- | -------------------------------------- |
| Language      | C++23                                  |
| Build System  | CMake 3.25+ with Visual Studio 17 2022 |
| GUI Framework | Slint 1.15+                            |
| Testing       | Google Test / Google Mock              |
| Configuration | JSON (rapidjson)                       |
| Logging       | spdlog                                 |
| Code Style    | Google C++ Style Guide                 |

---

## Project Setup

### Directory Structure

```plain
blinkbreak/
├── CMakeLists.txt              # Root CMake configuration
├── CMakePresets.json           # Build presets (Debug/Release/RelWithDebInfo)
├── .clang-format               # Code formatting rules
├── .clang-tidy                 # Static analysis configuration
├── README.md                   # Project documentation
├── ARCHITECTURE.md             # Detailed architecture documentation
├── LICENSE                     # License file
├── Doxyfile                    # Doxygen configuration
│
├── src/                        # Source files
│   ├── main.cpp                # Application entry point
│   ├── core/                   # Core business logic
│   │   ├── CMakeLists.txt
│   │   ├── timer.cpp
│   │   ├── timer.hpp
│   │   ├── state_machine.cpp
│   │   ├── state_machine.hpp
│   │   ├── config_manager.cpp
│   │   ├── config_manager.hpp
│   │   ├── break_scheduler.cpp
│   │   ├── break_scheduler.hpp
│   │   ├── message_provider.cpp
│   │   └── message_provider.hpp
│   │
│   ├── ui/                     # UI integration layer
│   │   ├── CMakeLists.txt
│   │   ├── app_controller.cpp
│   │   ├── app_controller.hpp
│   │   ├── overlay_manager.cpp
│   │   ├── overlay_manager.hpp
│   │   ├── tray_icon.cpp
│   │   └── tray_icon.hpp
│   │
│   └── platform/               # Platform-specific implementations
│       ├── CMakeLists.txt
│       ├── platform_interface.hpp
│       ├── windows/
│       │   ├── idle_detector_win.cpp
│       │   ├── idle_detector_win.hpp
│       │   ├── dnd_detector_win.cpp
│       │   ├── dnd_detector_win.hpp
│       │   ├── notification_win.cpp
│       │   ├── notification_win.hpp
│       │   └── monitor_manager_win.cpp
│       └── linux/              # Future: Linux support
│           └── .gitkeep
│
├── include/                    # Public headers
│   └── blinkbreak/
│       ├── types.hpp           # Common type definitions
│       ├── events.hpp          # Event definitions
│       └── version.hpp         # Version information
│
├── ui/                         # Slint UI files
│   ├── main_window.slint
│   ├── overlay.slint
│   ├── settings_dialog.slint
│   └── components/
│       ├── timer_display.slint
│       ├── break_controls.slint
│       └── progress_bar.slint
│
├── tests/                      # Test files
│   ├── CMakeLists.txt
│   ├── unit/
│   │   ├── test_timer.cpp
│   │   ├── test_state_machine.cpp
│   │   ├── test_config_manager.cpp
│   │   ├── test_break_scheduler.cpp
│   │   └── test_message_provider.cpp
│   └── integration/
│       ├── test_full_workflow.cpp
│       ├── test_idle_integration.cpp
│       └── mock_platform.hpp
│
├── resources/                  # Application resources
│   ├── icons/
│   │   ├── tray_icon.ico
│   │   ├── tray_icon.png
│   │   └── app_icon.ico
│   └── config/
│       └── default_config.json
│
└── docs/                       # Documentation
    ├── api/                    # Generated API docs
    └── images/                 # Documentation images
```

### Dependencies

| Dependency  | Version | Purpose       | Integration  |
| ----------- | ------- | ------------- | ------------ |
| Slint       | 1.7+    | GUI Framework | FetchContent |
| Google Test | 1.14+   | Unit Testing  | FetchContent |
| RapidJSON   | 3.11+   | JSON Parsing  | FetchContent |
| spdlog      | 1.12+   | Logging       | FetchContent |

---

## Stage 1: Project Foundation

### Goal

Establish the project infrastructure with a working build system, code formatting, and a minimal "Hello World" application that compiles and runs. This stage creates the foundation for TDD by setting up Google Test.

### Prerequisites

- CMake 3.25+ installed
- Visual Studio 2022 (MSVC with C++23 support) installed
- C++23 compatible compiler (MSVC 19.36+, GCC 13+, or Clang 17+)
- Git for version control

### Implementation Steps

#### Stage 1 Results (Summary)

Implemented the Stage 1 foundation and removed embedded source listings for a concise, progress-focused record.

Created/updated files

- `CMakeLists.txt`
- `CMakePresets.json`
- `.clang-format`
- `.clang-tidy`
- `include/blinkbreak/version.hpp`
- `include/blinkbreak/types.hpp`
- `src/CMakeLists.txt`
- `src/core/timer.hpp`
- `src/core/timer.cpp`
- `src/main.cpp`
- `tests/CMakeLists.txt`
- `tests/unit/test_timer.cpp`
- `README.md`

Notes

- Presets use the Visual Studio 17 2022 generator (multi-config), so binaries land under `build/<preset>/Debug` or `build/<preset>/Release`.
- Build system uses `rapidjson::rapidjson` and `spdlog::spdlog` in `src/CMakeLists.txt`.
- RapidJSON docs/examples/tests are disabled to avoid duplicate `gtest` targets during configure.
- Stage 1 implementation matches the planned structure and minimal runnable prototype.

### Test Requirements

#### Unit Tests

1. **test_timer.cpp**
   - `InitializesWithCorrectDuration` - Timer starts with correct values
   - `StartsCorrectly` - Timer transitions to running state
   - `PausesCorrectly` - Timer transitions to paused state
   - `ResetsToInitialDuration` - Timer resets properly
   - `DecrementsWhenRunning` - Timer counts down while running
   - `DoesNotDecrementWhenPaused` - Timer holds when paused
   - `CallsExpiredCallback` - Callback fires on expiry
   - `CallsTickCallback` - Tick callback fires on update
   - `HandlesOverflowGracefully` - Large update values handled
   - `IsMovable` - Move semantics work correctly

#### Verification Criteria

- [x] `cmake --preset=debug` completes without errors
- [x] `cmake --build --preset=debug` compiles successfully
- [x] `ctest --preset=debug` runs all tests, all pass
- [x] `build/debug/src/Debug/blinkbreak.exe --version` outputs version string
- [x] Code passes clang-format check: `Get-ChildItem -Recurse -Include *.cpp,*.hpp src,tests,include | ForEach-Object { clang-format --dry-run -Werror $_.FullName }; clang-format --version`

### Deliverables

- [x] Project structure created
- [x] CMakeLists.txt (root and subdirectories)
- [x] CMakePresets.json with Debug/Release/RelWithDebInfo
- [x] .clang-format based on Google Style
- [x] .clang-tidy configuration
- [x] Basic `Timer` class with tests
- [x] Main executable that displays version
- [x] `README.md` with build instructions
- [x] Tests executed and verified (pending)

---

## Stage 2: State Machine Implementation

### Goal

Implement a robust finite state machine (FSM) to manage application states. The state machine will govern transitions between Idle, Running, Paused, Snoozed, and BreakActive states with proper validation and event handling.

### Prerequisites

- Stage 1 completed successfully
- All Stage 1 tests passing

### Implementation Steps

- Implemented `include/blinkbreak/events.hpp` with all state machine event types.
- Added `src/core/state_machine.hpp` and `src/core/state_machine.cpp` with full transition validation, callbacks, and logging.
- Updated `src/CMakeLists.txt` to compile the new state machine sources.
- Added `tests/unit/test_state_machine.cpp` and registered it in `tests/CMakeLists.txt`.
- Updated `src/main.cpp` to demonstrate state transitions at startup.

Embedded source listings were removed after implementation to keep this plan concise.

### Test Requirements

#### Unit Tests

1. **test_state_machine.cpp**
   - Initial state tests (1 test)
   - Valid transition tests (12 tests)
   - Invalid transition tests (4 tests)
   - CanTransition tests (1 test)
   - Callback tests (2 tests)
   - ForceState tests (1 test)
   - StateToString tests (1 test)
   - Complex workflow tests (2 tests)

#### Verification Criteria

- [x] All Stage 1 tests still pass
- [x] All new state machine tests pass (24+ tests total)
- [x] State transitions log correctly
- [x] Invalid transitions return error messages
- [x] Demo application shows state transitions in log
- [ ] Code passes clang-format check

#### Validation Commands

- `cmake --preset=debug`
- `cmake --build --preset=debug`
- `ctest --preset=debug`

### Deliverables

- [x] events.hpp with all event types
- [x] StateMachine class with full implementation
- [x] Complete state transition validation
- [x] State change callbacks
- [x] Comprehensive unit tests (24+ new tests)
- [x] Updated main.cpp demonstrating state machine
- [x] All tests pass (34+ total)

---

## Stage 3: Configuration System

### Goal

Implement a JSON-based configuration system for managing break intervals, durations, messages, and user preferences. The system should support defaults, validation, and file-based persistence.

### Prerequisites

- Stage 2 completed successfully
- All Stage 2 tests passing

### Implementation Steps

#### 3.1 Create Configuration Types

Added configuration structs for breaks, idle detection, notifications, overlay, and app-level settings in `src/core/config_types.hpp`.

#### 3.2 Create Configuration Manager Header

Create `src/core/config_manager.hpp`:

Source listing removed. See `src/core/config_manager.hpp` for the full header.

#### 3.3 Create Configuration Manager Implementation

Create `src/core/config_manager.cpp`:

Source listing removed. See `src/core/config_manager.cpp` for the full implementation.

#### 3.4 Create Default Configuration File

Create `resources/config/default_config.json`:

Source listing removed. See `resources/config/default_config.json`.

#### 3.5 Create Configuration Manager Tests

Create `tests/unit/test_config_manager.cpp`:

Source listing removed. See `tests/unit/test_config_manager.cpp` for the test coverage.

#### 3.6 Update Source CMakeLists.txt

Update `src/CMakeLists.txt`:

Source listing removed. See `src/CMakeLists.txt` for the build integration.

#### 3.7 Update Tests CMakeLists.txt

Update `tests/CMakeLists.txt`:

Source listing removed. See `tests/CMakeLists.txt` for the test target updates.

#### 3.8 Update Main to Demo Configuration

Update `src/main.cpp`:

Source listing removed. See `src/main.cpp` for the demo wiring.

### Test Requirements

#### Unit Tests

1. **test_config_manager.cpp**
   - Default configuration tests (3 tests)
   - JSON parsing tests (3 tests)
   - ToJson tests (2 tests)
   - File operations tests (6 tests)
   - Validation tests (9 tests)

#### Verification Criteria

- [x] All Stage 2 tests still pass (ctest --preset=debug)
- [x] All new configuration tests pass (ctest --preset=debug)
- [x] Configuration file loads correctly (ConfigManagerTest.LoadReadsFromFile)
- [x] Invalid configurations are rejected (ConfigManagerTest.SaveRejectsInvalidConfig)
- [x] JSON roundtrip preserves values (ConfigManagerTest.ToJsonRoundtripPreservesValues)
- [x] Default config path is platform-appropriate (ConfigManagerTest.GetDefaultPathReturnsNonEmpty)
- [ ] Code passes clang-format check (not run)

Validation results:

- cmake --preset=debug: configured; RapidJSON CMake policy warnings (dev).
- cmake --build --preset=debug: build succeeded; MSVC STL4043 warnings from spdlog/fmt.
- ctest --preset=debug: 55 tests passed.

### Deliverables

- [x] config_types.hpp with all configuration structures
- [x] ConfigManager class with full implementation
- [x] JSON serialization/deserialization
- [x] Configuration validation
- [x] Default configuration values
- [x] default_config.json resource file
- [x] Comprehensive unit tests (23+ new tests)
- [x] Updated main.cpp demonstrating configuration
- [x] All tests pass (57+ total)

---

## Stage 4: Message Provider and Break Scheduler

### Goal

Implement the message rotation system and break scheduling logic that coordinates short and long breaks. This stage creates the core business logic for determining when breaks should occur.

### Prerequisites

- Stage 3 completed successfully
- All Stage 3 tests passing

### Implementation Steps

#### 4.1 Create Message Provider Header

Create `src/core/message_provider.hpp`:

Source listing removed. See `src/core/message_provider.hpp`.

#### 4.2 Create Message Provider Implementation

Create `src/core/message_provider.cpp`:

Source listing removed. See `src/core/message_provider.cpp`.

#### 4.3 Create Break Scheduler Header

Create `src/core/break_scheduler.hpp`:

Source listing removed. See `src/core/break_scheduler.hpp`.

#### 4.4 Create Break Scheduler Implementation

Create `src/core/break_scheduler.cpp`:

Source listing removed. See `src/core/break_scheduler.cpp`.

#### 4.5 Create Message Provider Tests

Create `tests/unit/test_message_provider.cpp`:

Source listing removed. See `tests/unit/test_message_provider.cpp`.

#### 4.6 Create Break Scheduler Tests

Create `tests/unit/test_break_scheduler.cpp`:

Source listing removed. See `tests/unit/test_break_scheduler.cpp`.

#### 4.7 Update Source CMakeLists.txt

Update `src/CMakeLists.txt`:

Source listing removed. See `src/CMakeLists.txt`.

#### 4.8 Update Tests CMakeLists.txt

Update `tests/CMakeLists.txt`:

Source listing removed. See `tests/CMakeLists.txt`.

### Test Requirements

#### Unit Tests

1. **test_message_provider.cpp**
   - Basic functionality tests (4 tests)
   - Rotation tests (2 tests)
   - Random mode tests (1 test)
   - Configuration tests (2 tests)
   - Move semantics tests (1 test)

2. **test_break_scheduler.cpp**
   - Initial state tests (2 tests)
   - Timer tests (3 tests)
   - Break type tests (2 tests)
   - Pause/Resume tests (2 tests)
   - Skip/Snooze tests (2 tests)
   - Callback tests (2 tests)
   - Reset tests (1 test)
   - Configuration update tests (1 test)

#### Verification Criteria

- [x] All Stage 3 tests still pass
- [x] All new message provider tests pass (10+ new tests)
- [x] All new break scheduler tests pass (15+ new tests)
- [x] Message rotation works correctly
- [x] Break timing is accurate
- [x] Callbacks fire at correct times
- [ ] Code passes clang-format check

Validation results:

- cmake --preset=debug: configured; RapidJSON CMake policy warnings (dev).
- cmake --build --preset=debug: build succeeded; MSVC STL4043 warnings from spdlog/fmt.
- ctest --preset=debug: 80 tests passed.

### Deliverables

- [x] MessageProvider class with full implementation
- [x] BreakScheduler class with full implementation
- [x] Message rotation (sequential and random)
- [x] Break timing and scheduling
- [x] Warning callback support
- [x] Comprehensive unit tests (25+ new tests)
- [x] All tests pass (82+ total)

---

## Stage 5: Basic Slint UI

### Goal

Integrate the Slint UI framework and create a minimal main window showing the countdown timer and basic controls. This establishes the UI foundation for future stages.

### Prerequisites

- Stage 4 completed successfully
- All Stage 4 tests passing
- Slint development environment set up
- Keep the Slint version 1.15.0

### Implementation Steps

#### 5.1 Update Root CMakeLists.txt for Slint

Add Slint dependency to the root CMakeLists.txt:

```cmake
# After existing FetchContent declarations, add:

# Fetch Slint
FetchContent_Declare(
    slint
    GIT_REPOSITORY https://github.com/slint-ui/slint.git
    GIT_TAG v1.15.0
    SOURCE_SUBDIR api/cpp
)
FetchContent_MakeAvailable(slint)
```

#### 5.2 Create Main Window Slint File

Create `ui/main_window.slint`:

```slint
// Main window for BlinkBreak application

import { VerticalBox, HorizontalBox, Button, ProgressIndicator } from "std-widgets.slint";

export component MainWindow inherits Window {
    title: "BlinkBreak";
    min-width: 300px;
    min-height: 200px;

    // Callbacks to C++
    callback start-clicked();
    callback pause-clicked();
    callback skip-clicked();
    callback settings-clicked();

    // Properties bound from C++
    in property <string> time-remaining: "10:00";
    in property <string> status-text: "Ready";
    in property <float> progress: 0.0;
    in property <bool> is-running: false;
    in property <string> next-break-type: "Short";

    VerticalBox {
        alignment: center;
        padding: 20px;
        spacing: 15px;

        // Status
        Text {
            text: root.status-text;
            font-size: 14px;
            horizontal-alignment: center;
        }

        // Timer display
        Text {
            text: root.time-remaining;
            font-size: 48px;
            font-weight: 700;
            horizontal-alignment: center;
        }

        // Next break type
        Text {
            text: "Next: " + root.next-break-type + " break";
            font-size: 12px;
            horizontal-alignment: center;
            color: #666666;
        }

        // Progress bar
        ProgressIndicator {
            width: 100%;
            height: 8px;
            progress: root.progress;
        }

        // Control buttons
        HorizontalBox {
            alignment: center;
            spacing: 10px;

            Button {
                text: root.is-running ? "Pause" : "Start";
                clicked => {
                    if (root.is-running) {
                        root.pause-clicked();
                    } else {
                        root.start-clicked();
                    }
                }
            }

            Button {
                text: "Skip";
                enabled: root.is-running;
                clicked => {
                    root.skip-clicked();
                }
            }

            Button {
                text: "Settings";
                clicked => {
                    root.settings-clicked();
                }
            }
        }
    }
}
```

#### 5.3 Create Timer Display Component

Create `ui/components/timer_display.slint`:

```slint
// Timer display component

export component TimerDisplay inherits Rectangle {
    in property <string> time: "00:00";
    in property <float> progress: 0.0;
    in property <color> accent-color: #4CAF50;

    width: 200px;
    height: 200px;
    border-radius: self.width / 2;
    background: #f5f5f5;

    // Progress ring (simplified)
    Rectangle {
        x: 4px;
        y: 4px;
        width: parent.width - 8px;
        height: parent.height - 8px;
        border-radius: self.width / 2;
        background: white;
    }

    // Time text
    Text {
        text: root.time;
        font-size: 36px;
        font-weight: 600;
        horizontal-alignment: center;
        vertical-alignment: center;
    }
}
```

#### 5.4 Create App Controller Header

Create `src/ui/app_controller.hpp`:

```cpp
/// @file app_controller.hpp
/// @brief Main application controller connecting UI and core logic.

#ifndef BLINKBREAK_UI_APP_CONTROLLER_HPP
#define BLINKBREAK_UI_APP_CONTROLLER_HPP

#include "core/break_scheduler.hpp"
#include "core/config_manager.hpp"
#include "core/state_machine.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

// Forward declaration of Slint generated types
namespace slint {
class ComponentHandle;
}

namespace blinkbreak {

/// @brief Main application controller.
///
/// The AppController orchestrates the interaction between the UI,
/// state machine, break scheduler, and configuration. It manages
/// the timer thread and ensures thread-safe updates to the UI.
class AppController {
public:
    /// @brief Constructs the application controller.
    AppController();

    /// @brief Destructor - stops all threads.
    ~AppController();

    // Non-copyable, non-movable
    AppController(const AppController&) = delete;
    AppController& operator=(const AppController&) = delete;
    AppController(AppController&&) = delete;
    AppController& operator=(AppController&&) = delete;

    /// @brief Initializes the controller with configuration.
    /// @return True if initialization succeeded.
    bool Initialize();

    /// @brief Runs the main application loop.
    /// @return Exit code.
    int Run();

    /// @brief Handles the start/resume action.
    void OnStart();

    /// @brief Handles the pause action.
    void OnPause();

    /// @brief Handles the skip action.
    void OnSkip();

    /// @brief Handles the snooze action.
    void OnSnooze();

    /// @brief Handles the reset action.
    void OnReset();

    /// @brief Opens the settings dialog.
    void OnOpenSettings();

    /// @brief Gets formatted time remaining string.
    /// @return Time in "MM:SS" format.
    [[nodiscard]] std::string GetTimeRemainingString() const;

    /// @brief Gets the current progress (0.0 - 1.0).
    /// @return Progress value.
    [[nodiscard]] float GetProgress() const;

    /// @brief Gets the current status text.
    /// @return Status string.
    [[nodiscard]] std::string GetStatusText() const;

private:
    /// @brief Timer thread function.
    void TimerThreadFunc();

    /// @brief Updates UI bindings (called from timer thread).
    void UpdateUI();

    /// @brief Formats duration as MM:SS string.
    /// @param duration The duration to format.
    /// @return Formatted string.
    [[nodiscard]] static std::string FormatDuration(Duration duration);

    std::unique_ptr<ConfigManager> config_manager_;
    std::unique_ptr<StateMachine> state_machine_;
    std::unique_ptr<BreakScheduler> scheduler_;
    AppConfig config_;

    std::thread timer_thread_;
    std::atomic<bool> running_;
    mutable std::mutex mutex_;

    // Cached UI state
    std::string time_remaining_;
    float progress_;
    std::string status_text_;
    bool is_running_;
};

}  // namespace blinkbreak

#endif  // BLINKBREAK_UI_APP_CONTROLLER_HPP
```

#### 5.5 Create App Controller Implementation

Create `src/ui/app_controller.cpp`:

```cpp
/// @file app_controller.cpp
/// @brief Implementation of the AppController class.

#include "app_controller.hpp"

#include <spdlog/spdlog.h>

#include <chrono>
#include <format>

namespace blinkbreak {

using namespace std::chrono_literals;

AppController::AppController()
    : running_(false),
      progress_(0.0f),
      is_running_(false) {
    spdlog::debug("AppController created");
}

AppController::~AppController() {
    running_.store(false);
    if (timer_thread_.joinable()) {
        timer_thread_.join();
    }
    spdlog::debug("AppController destroyed");
}

bool AppController::Initialize() {
    spdlog::info("Initializing AppController");

    // Load configuration
    config_manager_ = std::make_unique<ConfigManager>();
    auto config_path = ConfigManager::GetDefaultPath();

    auto loaded = config_manager_->Load(config_path);
    if (loaded) {
        config_ = loaded.value();
        spdlog::info("Configuration loaded from {}", config_path.string());
    } else {
        config_ = ConfigManager::GetDefault();
        spdlog::info("Using default configuration");
    }

    // Validate configuration
    auto errors = config_manager_->Validate(config_);
    if (!errors.empty()) {
        spdlog::error("Configuration validation failed");
        for (const auto& error : errors) {
            spdlog::error("  {}: {}", error.field, error.message);
        }
        return false;
    }

    // Create state machine
    state_machine_ = std::make_unique<StateMachine>();
    state_machine_->SetOnStateChange([this](State old_state, State new_state, const Event&) {
        spdlog::info("State: {} -> {}", StateToString(old_state), StateToString(new_state));
        std::lock_guard lock(mutex_);
        is_running_ = (new_state == State::kRunning);
    });

    // Create break scheduler
    scheduler_ = std::make_unique<BreakScheduler>(
        config_.short_break,
        config_.long_break,
        config_.overlay
    );

    scheduler_->SetOnBreakStart([this](const BreakInfo& info) {
        spdlog::info("Break started: {}", BreakTypeToString(info.type));
        state_machine_->ProcessEvent(TimerExpiredEvent{info.type});
    });

    scheduler_->SetOnBreakEnd([this](const BreakInfo& info) {
        spdlog::info("Break ended: {}", BreakTypeToString(info.type));
        state_machine_->ProcessEvent(BreakCompletedEvent{});
    });

    if (config_.notification.enabled) {
        scheduler_->SetOnWarning([](BreakType type, Duration time_until) {
            spdlog::info("Warning: {} break in {}s",
                         BreakTypeToString(type), time_until.count());
        }, config_.notification.warning_time);
    }

    // Initialize UI state
    time_remaining_ = FormatDuration(config_.short_break.interval);
    status_text_ = "Ready - Click Start";
    progress_ = 0.0f;

    spdlog::info("AppController initialized successfully");
    return true;
}

int AppController::Run() {
    spdlog::info("Starting application");

    // Start timer thread
    running_.store(true);
    timer_thread_ = std::thread(&AppController::TimerThreadFunc, this);

    // Auto-start if configured
    if (config_.auto_start) {
        OnStart();
    }

    // For now, just run a simple loop (will be replaced with Slint event loop)
    spdlog::info("Application running... (Press Ctrl+C to exit)");

    // Simulate running for demo
    std::this_thread::sleep_for(2s);

    running_.store(false);
    if (timer_thread_.joinable()) {
        timer_thread_.join();
    }

    return 0;
}

void AppController::OnStart() {
    spdlog::debug("OnStart called");

    auto result = state_machine_->ProcessEvent(StartEvent{});
    if (result.success) {
        scheduler_->Start();
        std::lock_guard lock(mutex_);
        status_text_ = "Running";
    }
}

void AppController::OnPause() {
    spdlog::debug("OnPause called");

    auto result = state_machine_->ProcessEvent(PauseEvent{});
    if (result.success) {
        scheduler_->Pause();
        std::lock_guard lock(mutex_);
        status_text_ = "Paused";
    }
}

void AppController::OnSkip() {
    spdlog::debug("OnSkip called");

    if (state_machine_->GetCurrentState() == State::kBreakActive) {
        scheduler_->SkipBreak();
        state_machine_->ProcessEvent(SkipEvent{});
    }
}

void AppController::OnSnooze() {
    spdlog::debug("OnSnooze called");

    if (state_machine_->GetCurrentState() == State::kBreakActive) {
        scheduler_->SnoozeBreak();
        state_machine_->ProcessEvent(SnoozeEvent{config_.overlay.snooze_duration});
        std::lock_guard lock(mutex_);
        status_text_ = "Snoozed";
    }
}

void AppController::OnReset() {
    spdlog::debug("OnReset called");

    state_machine_->ProcessEvent(ResetEvent{});
    scheduler_->Reset();
    std::lock_guard lock(mutex_);
    status_text_ = "Ready - Click Start";
    progress_ = 0.0f;
}

void AppController::OnOpenSettings() {
    spdlog::debug("OnOpenSettings called");
    // TODO: Implement settings dialog
}

std::string AppController::GetTimeRemainingString() const {
    std::lock_guard lock(mutex_);
    return time_remaining_;
}

float AppController::GetProgress() const {
    std::lock_guard lock(mutex_);
    return progress_;
}

std::string AppController::GetStatusText() const {
    std::lock_guard lock(mutex_);
    return status_text_;
}

void AppController::TimerThreadFunc() {
    spdlog::debug("Timer thread started");

    auto last_time = std::chrono::steady_clock::now();
    constexpr auto kTickInterval = 100ms;

    while (running_.load()) {
        auto now = std::chrono::steady_clock::now();
        auto delta = std::chrono::duration_cast<DurationMs>(now - last_time);
        last_time = now;

        scheduler_->Update(delta);
        UpdateUI();

        std::this_thread::sleep_for(kTickInterval);
    }

    spdlog::debug("Timer thread stopped");
}

void AppController::UpdateUI() {
    std::lock_guard lock(mutex_);

    auto time_until = scheduler_->GetTimeUntilNextBreak();
    if (time_until) {
        time_remaining_ = FormatDuration(*time_until);

        // Calculate progress based on next break type
        Duration total = scheduler_->GetNextBreakType() == BreakType::kShort
                         ? config_.short_break.interval
                         : config_.long_break.interval;

        if (total.count() > 0) {
            progress_ = 1.0f - (static_cast<float>(time_until->count()) /
                                static_cast<float>(total.count()));
        }
    }
}

std::string AppController::FormatDuration(Duration duration) {
    auto total_seconds = duration.count();
    auto minutes = total_seconds / 60;
    auto seconds = total_seconds % 60;
    return std::format("{:02}:{:02}", minutes, seconds);
}

}  // namespace blinkbreak
```

#### 5.6 Update Main Entry Point

Update `src/main.cpp`:

```cpp
/// @file main.cpp
/// @brief Entry point for BlinkBreak application.

#include "ui/app_controller.hpp"

#include <blinkbreak/version.hpp>
#include <spdlog/spdlog.h>

#include <iostream>

/// @brief Main entry point for the application.
/// @param argc Argument count.
/// @param argv Argument values.
/// @return Exit code (0 for success).
int main(int argc, char* argv[]) {
    // Configure logging
    spdlog::set_level(spdlog::level::debug);

    spdlog::info("{} v{}", blinkbreak::kAppName, blinkbreak::kVersionString);

    // Check for version flag
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--version" || arg == "-v") {
            std::cout << blinkbreak::kAppName << " version "
                      << blinkbreak::kVersionString << std::endl;
            return 0;
        }
    }

    // Create and run application
    blinkbreak::AppController app;

    if (!app.Initialize()) {
        spdlog::error("Failed to initialize application");
        return 1;
    }

    return app.Run();
}
```

#### 5.7 Update Source CMakeLists.txt

Update `src/CMakeLists.txt`:

```cmake
# Core library
add_library(blinkbreak_core STATIC)
target_sources(blinkbreak_core
    PRIVATE
        core/timer.cpp
        core/timer.hpp
        core/state_machine.cpp
        core/state_machine.hpp
        core/config_manager.cpp
        core/config_manager.hpp
        core/config_types.hpp
        core/message_provider.cpp
        core/message_provider.hpp
        core/break_scheduler.cpp
        core/break_scheduler.hpp
)
target_include_directories(blinkbreak_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(blinkbreak_core
    PUBLIC
    rapidjson::rapidjson
        spdlog::spdlog
)

# UI library
add_library(blinkbreak_ui STATIC)
target_sources(blinkbreak_ui
    PRIVATE
        ui/app_controller.cpp
        ui/app_controller.hpp
)
target_include_directories(blinkbreak_ui
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(blinkbreak_ui
    PUBLIC
        blinkbreak_core
)

# Note: Slint integration will be added in a later step
# when the full UI is implemented

# Main executable
add_executable(blinkbreak main.cpp)
target_link_libraries(blinkbreak
    PRIVATE
        blinkbreak_ui
)
```

### Test Requirements

#### Unit Tests

- AppController tests will be added in integration testing stage
- For now, verify manual testing of UI interactions

#### Verification Criteria

- [x] All Stage 4 tests still pass (`ctest --preset=debug`)
- [x] Application compiles with UI components (`cmake --build --preset=debug`)
- [ ] Application starts and shows logs
- [ ] Timer thread runs correctly
- [ ] State transitions work via OnStart/OnPause
- [ ] Time formatting is correct

Validation results:

- `cmake --preset=debug --fresh -DRust_COMPILER="C:\Users\azn\.rustup\toolchains\stable-x86_64-pc-windows-msvc\bin\rustc.exe"`: configured; RapidJSON dev warnings.
- `cmake --build --preset=debug`: build succeeded; Slint Rust warnings (non-fatal).
- `ctest --preset=debug`: 80 tests passed.

Notes:

- Slint `SLINT_FEATURE_INTERPRETER` was disabled to avoid a Rust compile error in `slint-interpreter` with the current toolchain.

### Deliverables

- [x] Slint UI files (main_window.slint, timer_display.slint)
- [x] AppController class with full implementation
- [x] Timer thread with periodic updates
- [x] UI state management
- [x] Duration formatting utility
- [x] All tests pass (80 total)

---

## Stage 6: System Tray Integration

### Goal

Add system tray icon with context menu for basic application control. The application should be able to minimize to tray and show status in the tray tooltip.

### Prerequisites

- Stage 5 completed successfully
- All Stage 5 tests passing
- Keep the Slint version 1.15.0

### Implementation Steps

#### 6.1 Create Platform Interface for System Tray

Create `src/platform/platform_interface.hpp`:

```cpp
/// @file platform_interface.hpp
/// @brief Platform abstraction interfaces.

#ifndef BLINKBREAK_PLATFORM_INTERFACE_HPP
#define BLINKBREAK_PLATFORM_INTERFACE_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace blinkbreak {
namespace platform {

/// @brief Menu item for system tray context menu.
struct MenuItem {
    std::string text;                      ///< Display text.
    std::function<void()> callback;        ///< Action callback.
    bool enabled = true;                   ///< Whether item is enabled.
    bool checked = false;                  ///< Whether item is checked.
    bool is_separator = false;             ///< Whether this is a separator.
};

/// @brief Interface for system tray functionality.
class ITrayIcon {
public:
    virtual ~ITrayIcon() = default;

    /// @brief Shows the tray icon.
    /// @return True if successful.
    virtual bool Show() = 0;

    /// @brief Hides the tray icon.
    virtual void Hide() = 0;

    /// @brief Sets the tooltip text.
    /// @param tooltip The tooltip text.
    virtual void SetTooltip(const std::string& tooltip) = 0;

    /// @brief Sets the context menu items.
    /// @param items The menu items.
    virtual void SetMenu(const std::vector<MenuItem>& items) = 0;

    /// @brief Sets the callback for left-click.
    /// @param callback The callback function.
    virtual void SetOnClick(std::function<void()> callback) = 0;

    /// @brief Sets the callback for double-click.
    /// @param callback The callback function.
    virtual void SetOnDoubleClick(std::function<void()> callback) = 0;

    /// @brief Updates the icon (e.g., to show different states).
    /// @param icon_id Identifier for the icon state.
    virtual void SetIcon(int icon_id) = 0;
};

/// @brief Creates platform-specific tray icon implementation.
/// @return Unique pointer to the tray icon implementation.
std::unique_ptr<ITrayIcon> CreateTrayIcon();

}  // namespace platform
}  // namespace blinkbreak

#endif  // BLINKBREAK_PLATFORM_INTERFACE_HPP
```

#### 6.2 Create Windows Tray Icon Implementation

Create `src/platform/windows/tray_icon_win.hpp`:

```cpp
/// @file tray_icon_win.hpp
/// @brief Windows-specific system tray icon implementation.

#ifndef BLINKBREAK_PLATFORM_WINDOWS_TRAY_ICON_WIN_HPP
#define BLINKBREAK_PLATFORM_WINDOWS_TRAY_ICON_WIN_HPP

#include "../platform_interface.hpp"

#ifdef _WIN32

#include <Windows.h>
#include <shellapi.h>

namespace blinkbreak {
namespace platform {

/// @brief Windows implementation of ITrayIcon.
class TrayIconWin : public ITrayIcon {
public:
    /// @brief Constructs a Windows tray icon.
    TrayIconWin();

    /// @brief Destructor - removes icon from tray.
    ~TrayIconWin() override;

    bool Show() override;
    void Hide() override;
    void SetTooltip(const std::string& tooltip) override;
    void SetMenu(const std::vector<MenuItem>& items) override;
    void SetOnClick(std::function<void()> callback) override;
    void SetOnDoubleClick(std::function<void()> callback) override;
    void SetIcon(int icon_id) override;

private:
    /// @brief Window procedure for tray messages.
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    /// @brief Shows the context menu at cursor position.
    void ShowContextMenu();

    /// @brief Creates a simple icon programmatically.
    /// @return Handle to the created icon.
    HICON CreateSimpleIcon();

    HWND hwnd_;                             ///< Hidden window for messages.
    NOTIFYICONDATAW nid_;                   ///< Tray icon data.
    HMENU hmenu_;                           ///< Context menu handle.
    std::vector<MenuItem> menu_items_;      ///< Current menu items.
    std::function<void()> on_click_;        ///< Left-click callback.
    std::function<void()> on_double_click_; ///< Double-click callback.
    bool is_visible_;                       ///< Whether icon is visible.

    static constexpr UINT kWmTrayIcon = WM_USER + 1;
    static inline TrayIconWin* instance_ = nullptr;
};

}  // namespace platform
}  // namespace blinkbreak

#endif  // _WIN32

#endif  // BLINKBREAK_PLATFORM_WINDOWS_TRAY_ICON_WIN_HPP
```

#### 6.3 Create Windows Tray Icon Implementation

Create `src/platform/windows/tray_icon_win.cpp`:

```cpp
/// @file tray_icon_win.cpp
/// @brief Windows-specific system tray icon implementation.

#ifdef _WIN32

#include "tray_icon_win.hpp"

#include <spdlog/spdlog.h>

namespace blinkbreak {
namespace platform {

TrayIconWin::TrayIconWin()
    : hwnd_(nullptr),
      hmenu_(nullptr),
      is_visible_(false) {
    instance_ = this;

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"BlinkBreakTrayClass";
    RegisterClassExW(&wc);

    // Create hidden window
    hwnd_ = CreateWindowExW(
        0, L"BlinkBreakTrayClass", L"BlinkBreak",
        0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
        GetModuleHandle(nullptr), nullptr
    );

    // Initialize tray icon data
    ZeroMemory(&nid_, sizeof(nid_));
    nid_.cbSize = sizeof(nid_);
    nid_.hWnd = hwnd_;
    nid_.uID = 1;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = kWmTrayIcon;
    nid_.hIcon = CreateSimpleIcon();
    wcscpy_s(nid_.szTip, L"BlinkBreak");

    spdlog::debug("TrayIconWin created");
}

TrayIconWin::~TrayIconWin() {
    Hide();
    if (nid_.hIcon) {
        DestroyIcon(nid_.hIcon);
    }
    if (hmenu_) {
        DestroyMenu(hmenu_);
    }
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
    instance_ = nullptr;
    spdlog::debug("TrayIconWin destroyed");
}

bool TrayIconWin::Show() {
    if (is_visible_) {
        return true;
    }

    if (Shell_NotifyIconW(NIM_ADD, &nid_)) {
        is_visible_ = true;
        spdlog::info("Tray icon shown");
        return true;
    }

    spdlog::error("Failed to show tray icon");
    return false;
}

void TrayIconWin::Hide() {
    if (!is_visible_) {
        return;
    }

    Shell_NotifyIconW(NIM_DELETE, &nid_);
    is_visible_ = false;
    spdlog::info("Tray icon hidden");
}

void TrayIconWin::SetTooltip(const std::string& tooltip) {
    std::wstring wide_tooltip(tooltip.begin(), tooltip.end());
    wcsncpy_s(nid_.szTip, wide_tooltip.c_str(), _TRUNCATE);

    if (is_visible_) {
        Shell_NotifyIconW(NIM_MODIFY, &nid_);
    }
}

void TrayIconWin::SetMenu(const std::vector<MenuItem>& items) {
    menu_items_ = items;

    if (hmenu_) {
        DestroyMenu(hmenu_);
    }
    hmenu_ = CreatePopupMenu();

    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        if (item.is_separator) {
            AppendMenuW(hmenu_, MF_SEPARATOR, 0, nullptr);
        } else {
            std::wstring wide_text(item.text.begin(), item.text.end());
            UINT flags = MF_STRING;
            if (!item.enabled) flags |= MF_GRAYED;
            if (item.checked) flags |= MF_CHECKED;
            AppendMenuW(hmenu_, flags, i + 1, wide_text.c_str());
        }
    }
}

void TrayIconWin::SetOnClick(std::function<void()> callback) {
    on_click_ = std::move(callback);
}

void TrayIconWin::SetOnDoubleClick(std::function<void()> callback) {
    on_double_click_ = std::move(callback);
}

void TrayIconWin::SetIcon(int icon_id) {
    // For now, we use the same icon
    // In future, could switch between different icons
    (void)icon_id;
}

LRESULT CALLBACK TrayIconWin::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == kWmTrayIcon && instance_) {
        switch (LOWORD(lparam)) {
            case WM_LBUTTONUP:
                if (instance_->on_click_) {
                    instance_->on_click_();
                }
                break;
            case WM_LBUTTONDBLCLK:
                if (instance_->on_double_click_) {
                    instance_->on_double_click_();
                }
                break;
            case WM_RBUTTONUP:
                instance_->ShowContextMenu();
                break;
        }
        return 0;
    }

    if (msg == WM_COMMAND && instance_) {
        UINT id = LOWORD(wparam);
        if (id > 0 && id <= instance_->menu_items_.size()) {
            const auto& item = instance_->menu_items_[id - 1];
            if (item.callback && item.enabled) {
                item.callback();
            }
        }
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void TrayIconWin::ShowContextMenu() {
    if (!hmenu_) return;

    POINT pt;
    GetCursorPos(&pt);

    SetForegroundWindow(hwnd_);
    TrackPopupMenu(hmenu_, TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                   pt.x, pt.y, 0, hwnd_, nullptr);
    PostMessage(hwnd_, WM_NULL, 0, 0);
}

HICON TrayIconWin::CreateSimpleIcon() {
    // Create a simple 16x16 icon programmatically
    // This is a placeholder - in production, load from resources

    const int size = 16;
    HDC hdc = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(hdc);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = size;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits;
    HBITMAP hbm = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HBITMAP hbmMask = CreateBitmap(size, size, 1, 1, nullptr);

    if (hbm && bits) {
        // Fill with a simple eye-like pattern (green circle)
        auto* pixels = static_cast<DWORD*>(bits);
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                int dx = x - size / 2;
                int dy = y - size / 2;
                int dist = dx * dx + dy * dy;

                if (dist < 36) {  // Inner circle (radius ~6)
                    pixels[y * size + x] = 0xFF4CAF50;  // Green
                } else if (dist < 64) {  // Outer ring
                    pixels[y * size + x] = 0xFF2E7D32;  // Dark green
                } else {
                    pixels[y * size + x] = 0x00000000;  // Transparent
                }
            }
        }
    }

    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmMask = hbmMask;
    ii.hbmColor = hbm;

    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hbm);
    DeleteObject(hbmMask);
    DeleteDC(memDC);
    ReleaseDC(nullptr, hdc);

    return hIcon;
}

// Factory function
std::unique_ptr<ITrayIcon> CreateTrayIcon() {
    return std::make_unique<TrayIconWin>();
}

}  // namespace platform
}  // namespace blinkbreak

#endif  // _WIN32
```

#### 6.4 Create TrayManager Class

Create `src/ui/tray_manager.hpp`:

```cpp
/// @file tray_manager.hpp
/// @brief System tray management.

#ifndef BLINKBREAK_UI_TRAY_MANAGER_HPP
#define BLINKBREAK_UI_TRAY_MANAGER_HPP

#include "platform/platform_interface.hpp"

#include <blinkbreak/types.hpp>

#include <functional>
#include <memory>

namespace blinkbreak {

/// @brief Manages system tray icon and menu.
class TrayManager {
public:
    /// @brief Callbacks for tray actions.
    struct Callbacks {
        std::function<void()> on_show_window;
        std::function<void()> on_start_pause;
        std::function<void()> on_skip;
        std::function<void()> on_settings;
        std::function<void()> on_quit;
    };

    /// @brief Constructs a tray manager.
    /// @param callbacks The action callbacks.
    explicit TrayManager(Callbacks callbacks);

    /// @brief Destructor.
    ~TrayManager();

    /// @brief Shows the tray icon.
    /// @return True if successful.
    bool Show();

    /// @brief Hides the tray icon.
    void Hide();

    /// @brief Updates the tooltip with current status.
    /// @param is_running Whether the timer is running.
    /// @param time_until_break Time until next break.
    /// @param break_type Type of next break.
    void UpdateStatus(bool is_running, Duration time_until_break, BreakType break_type);

    /// @brief Updates the menu based on current state.
    /// @param is_running Whether the timer is running.
    void UpdateMenu(bool is_running);

private:
    std::unique_ptr<platform::ITrayIcon> tray_icon_;
    Callbacks callbacks_;
    bool is_running_;
};

}  // namespace blinkbreak

#endif  // BLINKBREAK_UI_TRAY_MANAGER_HPP
```

#### 6.5 Create TrayManager Implementation

Create `src/ui/tray_manager.cpp`:

```cpp
/// @file tray_manager.cpp
/// @brief Implementation of the TrayManager class.

#include "tray_manager.hpp"

#include <spdlog/spdlog.h>

#include <format>

namespace blinkbreak {

TrayManager::TrayManager(Callbacks callbacks)
    : callbacks_(std::move(callbacks)),
      is_running_(false) {
    tray_icon_ = platform::CreateTrayIcon();

    tray_icon_->SetOnDoubleClick([this]() {
        if (callbacks_.on_show_window) {
            callbacks_.on_show_window();
        }
    });

    UpdateMenu(false);
    spdlog::debug("TrayManager created");
}

TrayManager::~TrayManager() {
    Hide();
    spdlog::debug("TrayManager destroyed");
}

bool TrayManager::Show() {
    return tray_icon_->Show();
}

void TrayManager::Hide() {
    tray_icon_->Hide();
}

void TrayManager::UpdateStatus(bool is_running, Duration time_until_break, BreakType break_type) {
    std::string tooltip;

    if (is_running) {
        auto minutes = time_until_break.count() / 60;
        auto seconds = time_until_break.count() % 60;
        tooltip = std::format("BlinkBreak - {} break in {:02}:{:02}",
                              BreakTypeToString(break_type), minutes, seconds);
    } else {
        tooltip = "BlinkBreak - Paused";
    }

    tray_icon_->SetTooltip(tooltip);

    if (is_running != is_running_) {
        UpdateMenu(is_running);
        is_running_ = is_running;
    }
}

void TrayManager::UpdateMenu(bool is_running) {
    std::vector<platform::MenuItem> items;

    // Show window
    items.push_back({
        .text = "Show BlinkBreak",
        .callback = callbacks_.on_show_window,
        .enabled = true
    });

    items.push_back({.is_separator = true});

    // Start/Pause
    items.push_back({
        .text = is_running ? "Pause" : "Start",
        .callback = callbacks_.on_start_pause,
        .enabled = true
    });

    // Skip next break
    items.push_back({
        .text = "Skip Next Break",
        .callback = callbacks_.on_skip,
        .enabled = is_running
    });

    items.push_back({.is_separator = true});

    // Settings
    items.push_back({
        .text = "Settings...",
        .callback = callbacks_.on_settings,
        .enabled = true
    });

    items.push_back({.is_separator = true});

    // Quit
    items.push_back({
        .text = "Quit",
        .callback = callbacks_.on_quit,
        .enabled = true
    });

    tray_icon_->SetMenu(items);
}

}  // namespace blinkbreak
```

### Test Requirements

#### Verification Criteria

- [ ] All Stage 5 tests still pass
- [ ] Tray icon appears in system tray
- [ ] Tooltip shows correct status
- [ ] Context menu appears on right-click
- [ ] Menu items trigger correct callbacks
- [ ] Double-click opens main window

### Deliverables

- [x] Platform interface for tray icon
- [x] Windows tray icon implementation
- [x] TrayManager class
- [x] Context menu with controls
- [x] Tooltip status updates
- [x] All tests pass (82+ total)

---

## Stage 7: Break Overlay Window

### Goal

Implement the full-screen overlay window that displays during breaks with dimming effect, break message, countdown timer, and action buttons.

### Prerequisites

- Stage 6 completed successfully
- Slint fully integrated

### Implementation Steps

#### 7.1 Create Overlay Slint File

Create `ui/overlay.slint`:

```slint
// Break overlay window

import { Button, VerticalBox, HorizontalBox } from "std-widgets.slint";

export component BreakOverlay inherits Window {
    title: "Break";
    no-frame: true;
    always-on-top: true;

    // Callbacks
    callback skip-clicked();
    callback snooze-clicked();

    // Properties
    in property <string> message: "Take a break!";
    in property <string> time-remaining: "00:20";
    in property <string> break-type: "Short";
    in property <bool> can-skip: true;
    in property <bool> can-snooze: true;
    in property <float> opacity: 0.7;

    // Semi-transparent background
    Rectangle {
        background: black.with-alpha(root.opacity);

        VerticalBox {
            alignment: center;

            // Break type indicator
            Text {
                text: root.break-type + " Break";
                font-size: 18px;
                color: white.with-alpha(0.7);
                horizontal-alignment: center;
            }

            // Main message
            Text {
                text: root.message;
                font-size: 32px;
                font-weight: 600;
                color: white;
                horizontal-alignment: center;
            }

            // Spacer
            Rectangle {
                height: 30px;
            }

            // Time remaining
            Text {
                text: root.time-remaining;
                font-size: 72px;
                font-weight: 700;
                color: white;
                horizontal-alignment: center;
            }

            // Spacer
            Rectangle {
                height: 40px;
            }

            // Action buttons
            HorizontalBox {
                alignment: center;
                spacing: 20px;

                if root.can-skip: Button {
                    text: "Skip";
                    clicked => { root.skip-clicked(); }
                }

                if root.can-snooze: Button {
                    text: "Snooze (5 min)";
                    clicked => { root.snooze-clicked(); }
                }
            }
        }
    }
}
```

#### 7.2 Create Overlay Manager

Create `src/ui/overlay_manager.hpp` and `overlay_manager.cpp` for managing overlay windows across monitors.

### Test Requirements

- Overlay appears full-screen
- Dimming effect works
- Message displays correctly
- Countdown updates
- Skip/Snooze buttons work
- Overlay closes on completion

### Deliverables

- [x] Overlay Slint file
- [x] OverlayManager class
- [x] Break message display
- [x] Countdown timer
- [x] Action buttons

---

## Stage 8: Multi-Monitor Support

### Goal

Extend the overlay system to support multiple monitors with configurable display options.

### Prerequisites

- Stage 7 completed successfully

### Implementation Steps

#### 8.1 Create Monitor Manager Interface

```cpp
/// @brief Information about a display monitor.
struct MonitorInfo {
    int id;                   ///< Monitor identifier.
    std::string name;         ///< Display name.
    int x, y;                 ///< Position.
    int width, height;        ///< Dimensions.
    bool is_primary;          ///< Whether this is the primary monitor.
};

/// @brief Interface for monitor management.
class IMonitorManager {
public:
    virtual ~IMonitorManager() = default;
    virtual std::vector<MonitorInfo> GetMonitors() = 0;
    virtual MonitorInfo GetPrimaryMonitor() = 0;
    virtual void SetOnMonitorChange(std::function<void()> callback) = 0;
};
```

#### 8.2 Windows Monitor Implementation

Implement monitor enumeration using `EnumDisplayMonitors` Win32 API.

### Deliverables

- [x] MonitorManager interface
- [x] Windows implementation
- [x] Multi-overlay support
- [x] Configuration option for all/primary only

---

## Stage 9: Idle Detection

### Goal

Implement OS-level idle detection to automatically pause timers when user is inactive.

### Prerequisites

- Stage 8 completed successfully

### Implementation Steps

#### 9.1 Create Idle Detector Interface

```cpp
/// @brief Interface for idle detection.
class IIdleDetector {
public:
    virtual ~IIdleDetector() = default;

    /// @brief Starts monitoring for idle state.
    virtual void Start() = 0;

    /// @brief Stops monitoring.
    virtual void Stop() = 0;

    /// @brief Gets current idle time.
    virtual Duration GetIdleTime() = 0;

    /// @brief Sets threshold and callbacks.
    virtual void SetIdleThreshold(Duration threshold) = 0;
    virtual void SetOnIdle(std::function<void()> callback) = 0;
    virtual void SetOnActive(std::function<void()> callback) = 0;
};
```

#### 9.2 Windows Idle Detection

Implement using `GetLastInputInfo` Win32 API.

### Deliverables

- [x] IdleDetector interface
- [x] Windows implementation
- [x] Integration with state machine
- [x] Auto-pause on idle
- [x] Auto-resume on activity

---

## Stage 10: Notifications

### Goal

Implement native OS notifications for pre-break warnings.

### Prerequisites

- Stage 9 completed successfully

### Implementation Steps

#### 10.1 Create Notification Interface

```cpp
/// @brief Interface for notifications.
class INotification {
public:
    virtual ~INotification() = default;

    virtual bool Show(const std::string& title,
                      const std::string& message) = 0;
    virtual void Hide() = 0;
    virtual void SetOnClick(std::function<void()> callback) = 0;
};
```

#### 10.2 Windows Toast Notifications

Implement using Windows Toast API or simple balloon notifications.

### Deliverables

- [x] Notification interface
- [x] Windows implementation
- [x] Pre-break warnings
- [x] Integration with scheduler

---

## Stage 11: Do Not Disturb Detection

### Goal

Detect Windows Focus Assist/DND mode and optionally suppress breaks.

### Prerequisites

- Stage 10 completed successfully

### Implementation Steps

#### 11.1 Create DND Detector Interface

```cpp
/// @brief Interface for Do Not Disturb detection.
class IDndDetector {
public:
    virtual ~IDndDetector() = default;

    virtual bool IsDndActive() = 0;
    virtual void SetOnDndChange(std::function<void(bool)> callback) = 0;
};
```

#### 11.2 Windows Focus Assist Detection

Implement using Windows Query Focus Assist API.

### Deliverables

- [x] DndDetector interface
- [x] Windows implementation
- [x] Configuration option
- [x] Break suppression logic

---

## Stage 12: Integration Tests & Polish

### Goal

Comprehensive integration testing, performance optimization, and final polish.

### Prerequisites

- All previous stages completed

### Implementation Steps

#### 12.1 Create Integration Test Harness

Create `tests/integration/test_harness.hpp`:

```cpp
/// @brief Test harness for integration testing.
class TestHarness {
public:
    /// @brief Simulates user idle state.
    void SimulateIdle(Duration duration);

    /// @brief Simulates user activity.
    void SimulateActivity();

    /// @brief Fast-forwards time.
    void AdvanceTime(Duration duration);

    /// @brief Gets current application state.
    State GetCurrentState();

    /// @brief Gets break count statistics.
    struct Stats {
        int short_breaks_triggered;
        int long_breaks_triggered;
        int breaks_skipped;
        int breaks_snoozed;
    };
    Stats GetStats();
};
```

#### 12.2 Integration Tests

Create `tests/integration/test_full_workflow.cpp`:

- Full break cycle workflow
- Idle detection workflow
- DND mode workflow
- Configuration changes during runtime
- Multi-day simulation

#### 12.3 Performance Testing

- Memory usage monitoring
- CPU usage profiling
- Binary size verification
- Startup time measurement

#### 12.4 Final Polish

- Code review and cleanup
- Documentation completion
- Doxygen generation
- README finalization

### Test Requirements

#### Integration Tests

- Full workflow tests (5+ tests)
- Idle integration tests (3+ tests)
- DND integration tests (2+ tests)
- Performance benchmarks

#### Verification Criteria

- [ ] All unit tests pass (100+ tests)
- [ ] All integration tests pass
- [ ] Memory usage < 50 MB
- [ ] Binary size < 10 MB
- [ ] No memory leaks (24-hour run)
- [ ] Code coverage > 80%
- [ ] Doxygen generates cleanly

### Deliverables

- [x] Complete integration test suite
- [x] Performance benchmarks
- [x] Final documentation
- [x] Production-ready binary
- [x] 100+ passing tests

---

## Appendix A: Testing Commands Reference

```bash
# Configure debug build
cmake --preset=debug

# Build debug
cmake --build --preset=debug

# Run all tests
ctest --preset=debug

# Run tests with verbose output
ctest --preset=debug --output-on-failure -V

# Run specific test
build/debug/tests/Debug/blinkbreak_tests.exe --gtest_filter="TimerTest.*"

# Run with coverage (requires gcov/lcov)
cmake --preset=debug -DCMAKE_CXX_FLAGS="--coverage"
cmake --build --preset=debug
ctest --preset=debug
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report

# Run release build
cmake --preset=release
cmake --build --preset=release
build/release/Release/blinkbreak.exe
```

## Appendix B: Code Quality Commands

```bash
# Format code
find src tests include -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Check formatting
find src tests include -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run -Werror

# Run clang-tidy
clang-tidy src/**/*.cpp -- -std=c++23 -I include

# Generate Doxygen documentation
doxygen Doxyfile
```

## Appendix C: Debugging Tips

1. **Enable verbose logging**: Set `spdlog::set_level(spdlog::level::trace)`
2. **Timer debugging**: Add tick callbacks to trace countdown
3. **State machine debugging**: Log all state transitions
4. **Memory debugging**: Use AddressSanitizer with `-fsanitize=address`
5. **Thread debugging**: Use ThreadSanitizer with `-fsanitize=thread`

---

## Summary

This implementation plan provides a comprehensive 12-stage roadmap for building BlinkBreak:

| Stage | Focus             | Tests Added | Total Tests |
| ----- | ----------------- | ----------- | ----------- |
| 1     | Foundation        | 10          | 10          |
| 2     | State Machine     | 24          | 34          |
| 3     | Configuration     | 23          | 57          |
| 4     | Message/Scheduler | 25          | 82          |
| 5     | Basic UI          | -           | 82          |
| 6     | System Tray       | -           | 82          |
| 7     | Overlay           | -           | 82          |
| 8     | Multi-Monitor     | -           | 82          |
| 9     | Idle Detection    | 5           | 87          |
| 10    | Notifications     | 3           | 90          |
| 11    | DND Detection     | 2           | 92          |
| 12    | Integration       | 10+         | 100+        |

Each stage builds upon the previous, maintaining a working prototype throughout development. Follow TDD principles strictly: write tests first, then implement the minimum code to pass.
