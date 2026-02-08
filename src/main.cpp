/// @file main.cpp
/// @brief Entry point for BlinkBreak application.

#include "ui/app_controller.hpp"

#include <blinkbreak/version.hpp>
#include <spdlog/spdlog.h>

#include <iostream>
#include <string>

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
            std::cout << blinkbreak::kAppName << " version " << blinkbreak::kVersionString
                      << std::endl;
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