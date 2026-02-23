/// @file main.cpp
/// @brief Entry point for BlinkBreak application.

#include "ui/app_controller.hpp"

#include <blinkbreak/version.hpp>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <Windows.h>
#include <cstdlib>
#endif

#include <iostream>
#include <string>

/// @brief Main entry point for the application.
/// @param argc Argument count.
/// @param argv Argument values.
/// @return Exit code (0 for success).
int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#endif

    // Configure logging
    spdlog::set_level(spdlog::level::debug);

    spdlog::info("{} v{}", blinkbreak::kAppName, blinkbreak::kVersionString);

    // Parse command-line flags
    bool use_software_renderer = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--version" || arg == "-v") {
            std::cout << blinkbreak::kAppName << " version " << blinkbreak::kVersionString
                      << std::endl;
            return 0;
        }
        if (arg == "--software-renderer") {
            use_software_renderer = true;
        }
    }

    // Force Slint to use the software renderer if requested.
    // This bypasses GPU swapchain issues on portrait (rotated) monitors
    // where the Slint/skia DirectX backend creates the rendering surface
    // at the native (un-rotated) panel dimensions instead of the rotated
    // window dimensions.
    //
    // Usage: blinkbreak.exe --software-renderer
    if (use_software_renderer) {
#ifdef _WIN32
        _putenv_s("SLINT_BACKEND", "winit-software");
#else
        setenv("SLINT_BACKEND", "winit-software", 1);
#endif
        spdlog::info("Software renderer enabled (SLINT_BACKEND=winit-software)");
    }

    // Create and run application
    blinkbreak::AppController app;

    if (!app.Initialize()) {
        spdlog::error("Failed to initialize application");
        return 1;
    }

    return app.Run();
}