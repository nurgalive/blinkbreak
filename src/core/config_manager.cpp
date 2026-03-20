#include "config_manager.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <glaze/glaze.hpp>
#include <spdlog/spdlog.h>
#include <string>

namespace blinkbreak {
namespace {

const AppConfig& DefaultConfig() {
    static const AppConfig kDefaultConfig = ConfigManager::GetDefault();
    return kDefaultConfig;
}

}  // namespace

ConfigManager::ConfigManager() {
    spdlog::debug("ConfigManager initialized");
}

ConfigManager::~ConfigManager() = default;

ConfigResult<AppConfig> ConfigManager::Load(const std::filesystem::path& path) {
    spdlog::info("Loading configuration from: {}", path.string());

    if (!std::filesystem::exists(path)) {
        return std::unexpected(
            ConfigError{.message = "Configuration file not found", .field = path.string()});
    }

    AppConfig config = GetDefault();
    std::string buffer;
    auto error = glz::read_file_json(config, path.string(), buffer);
    if (error) {
        return std::unexpected(ConfigError{
            .message = "Failed to parse configuration: " + glz::format_error(error, buffer),
            .field = path.string()});
    }

    spdlog::debug("Configuration loaded successfully");
    return config;
}

ConfigResult<void> ConfigManager::Save(const AppConfig& config, const std::filesystem::path& path) {
    spdlog::info("Saving configuration to: {}", path.string());

    auto errors = Validate(config);
    if (!errors.empty()) {
        return std::unexpected(
            ConfigError{.message = "Configuration validation failed: " + errors[0].message,
                        .field = errors[0].field});
    }

    auto parent = path.parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent)) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            return std::unexpected(
                ConfigError{.message = "Failed to create directory: " + ec.message(),
                            .field = parent.string()});
        }
    }

    std::string buffer;
    auto error = glz::write_file_json(config, path.string(), buffer);
    if (error) {
        return std::unexpected(
            ConfigError{.message = "Failed to save configuration (error code: " + std::to_string(static_cast<int>(error.ec)) + ")",
                        .field = path.string()});
    }

    spdlog::debug("Configuration saved successfully");
    return {};
}

std::vector<ConfigError> ConfigManager::Validate(const AppConfig& config) const {
    std::vector<ConfigError> errors;

    auto short_errors = ValidateBreakConfig(config.short_break, "short_break");
    errors.insert(errors.end(), short_errors.begin(), short_errors.end());

    auto long_errors = ValidateBreakConfig(config.long_break, "long_break");
    errors.insert(errors.end(), long_errors.begin(), long_errors.end());

    if (config.idle.enabled && config.idle.threshold <= Duration::zero()) {
        errors.push_back(
            {.message = "Idle threshold must be positive when enabled", .field = "idle.threshold"});
    }

    if (config.notification.enabled && config.notification.warning_time < Duration::zero()) {
        errors.push_back(
            {.message = "Warning time cannot be negative", .field = "notification.warning_time"});
    }

    if (config.overlay.opacity < 0.0f || config.overlay.opacity > 1.0f) {
        errors.push_back(
            {.message = "Opacity must be between 0.0 and 1.0", .field = "overlay.opacity"});
    }

    if (config.overlay.allow_snooze && config.overlay.snooze_duration <= Duration::zero()) {
        errors.push_back({.message = "Snooze duration must be positive when snooze is allowed",
                          .field = "overlay.snooze_duration"});
    }

    if (config.short_break.enabled && config.long_break.enabled) {
        if (config.long_break.interval <= config.short_break.interval) {
            errors.push_back(
                {.message = "Long break interval should be greater than short break interval",
                 .field = "long_break.interval"});
        }
    }

    return errors;
}

AppConfig ConfigManager::GetDefault() {
    AppConfig config;

    config.short_break.enabled = true;
    config.short_break.interval = Duration(600);
    config.short_break.duration = Duration(20);
    config.short_break.messages = {"Look away from the screen", "Focus on something 20 feet away",
                                   "Blink slowly 10 times", "Close your eyes and relax"};
    config.short_break.rotate_messages = true;

    config.long_break.enabled = true;
    config.long_break.interval = Duration(3600);
    config.long_break.duration = Duration(300);
    config.long_break.messages = {"Time for a longer break!", "Stand up and stretch",
                                  "Take a short walk", "Hydrate yourself"};
    config.long_break.rotate_messages = true;

    config.idle.enabled = true;
    config.idle.threshold = Duration(180);
    config.idle.pause_on_idle = true;
    config.idle.reset_on_idle = false;
    config.idle.show_timer = false;

    config.notification.enabled = true;
    config.notification.warning_time = Duration(30);
    config.notification.respect_dnd = true;

    config.theme.follow_system = true;
    config.theme.dark_mode = false;

    config.overlay.opacity = 0.7f;
    config.overlay.show_on_all_monitors = true;
    config.overlay.allow_skip = true;
    config.overlay.allow_snooze = true;
    config.overlay.snooze_duration = Duration(300);

    config.start_minimized = false;
    config.auto_start = false;

    return config;
}

std::filesystem::path ConfigManager::GetDefaultPath() {
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        return std::filesystem::path(appdata) / "BlinkBreak" / "config.json";
    }
    return std::filesystem::path("config.json");
#else
    const char* home = std::getenv("HOME");
    if (home) {
        return std::filesystem::path(home) / ".config" / "blinkbreak" / "config.json";
    }
    return std::filesystem::path("config.json");
#endif
}

ConfigResult<AppConfig> ConfigManager::ParseJson(const std::string& json_string) {
    AppConfig config = GetDefault();
    auto error = glz::read_json(config, json_string);
    if (error) {
        return std::unexpected(ConfigError{
            .message = "JSON parse error: " + glz::format_error(error, json_string), .field = ""});
    }

    // Ensure messages are not empty if they were cleared by JSON
    if (config.short_break.messages.empty()) {
        config.short_break.messages = GetDefault().short_break.messages;
    }
    if (config.long_break.messages.empty()) {
        config.long_break.messages = GetDefault().long_break.messages;
    }

    spdlog::debug("Configuration parsed successfully");
    return config;
}

std::string ConfigManager::ToJson(const AppConfig& config) const {
    return glz::write_json(config);
}

std::vector<ConfigError> ConfigManager::ValidateBreakConfig(const BreakConfig& config,
                                                            const std::string& name) const {
    std::vector<ConfigError> errors;

    if (!config.enabled) {
        return errors;
    }

    if (config.interval <= Duration::zero()) {
        errors.push_back(
            {.message = "Break interval must be positive", .field = name + ".interval"});
    }

    if (config.duration <= Duration::zero()) {
        errors.push_back(
            {.message = "Break duration must be positive", .field = name + ".duration"});
    }

    if (config.duration >= config.interval) {
        errors.push_back(
            {.message = "Break duration must be less than interval", .field = name + ".duration"});
    }

    if (config.messages.empty()) {
        errors.push_back(
            {.message = "At least one message is required", .field = name + ".messages"});
    }

    return errors;
}

}  // namespace blinkbreak
