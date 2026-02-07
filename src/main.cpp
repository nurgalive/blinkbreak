/// @file main.cpp
/// @brief Entry point for BlinkBreak application.

#include <blinkbreak/version.hpp>

#include <iostream>
#include <spdlog/spdlog.h>
#include <string>

#include "core/config_manager.hpp"

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

    // Demo configuration system
    blinkbreak::ConfigManager config_manager;

    auto config = blinkbreak::ConfigManager::GetDefault();
    spdlog::info("Default short break interval: {}s", config.short_break.interval.count());
    spdlog::info("Default long break interval: {}s", config.long_break.interval.count());

    auto config_path = blinkbreak::ConfigManager::GetDefaultPath();
    spdlog::info("Config path: {}", config_path.string());

    auto loaded = config_manager.Load(config_path);
    if (loaded) {
        config = loaded.value();
        spdlog::info("Loaded configuration from file");
    } else {
        spdlog::info("Using default configuration: {}", loaded.error().message);
    }

    auto errors = config_manager.Validate(config);
    if (!errors.empty()) {
        spdlog::error("Configuration validation failed:");
        for (const auto& error : errors) {
            spdlog::error("  {}: {}", error.field, error.message);
        }
        return 1;
    }

    spdlog::info("Configuration validated successfully");
    spdlog::info("Stage 4: Complete");

    return 0;
}
