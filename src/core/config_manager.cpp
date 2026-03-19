/// @file config_manager.cpp
/// @brief Implementation of the ConfigManager class.

#include "config_manager.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <spdlog/spdlog.h>
#include <string>

namespace blinkbreak {
namespace {

using rapidjson::Document;
using rapidjson::Value;

const AppConfig& DefaultConfig() {
    static const AppConfig kDefaultConfig = ConfigManager::GetDefault();
    return kDefaultConfig;
}

Duration DurationFromSeconds(std::int64_t seconds) {
    return Duration(seconds);
}

std::int64_t SecondsFromDuration(Duration duration) {
    return static_cast<std::int64_t>(duration.count());
}

bool IsObjectField(const Value& obj, const char* name) {
    return obj.HasMember(name) && obj[name].IsObject();
}

bool IsArrayField(const Value& obj, const char* name) {
    return obj.HasMember(name) && obj[name].IsArray();
}

bool IsBoolField(const Value& obj, const char* name) {
    return obj.HasMember(name) && obj[name].IsBool();
}

bool IsNumberField(const Value& obj, const char* name) {
    return obj.HasMember(name) && obj[name].IsNumber();
}

BreakConfig ReadBreakConfig(const Value& obj) {
    BreakConfig config = DefaultConfig().short_break;

    if (IsBoolField(obj, "enabled")) {
        config.enabled = obj["enabled"].GetBool();
    }
    if (IsNumberField(obj, "interval_seconds")) {
        config.interval = DurationFromSeconds(obj["interval_seconds"].GetInt64());
    }
    if (IsNumberField(obj, "duration_seconds")) {
        config.duration = DurationFromSeconds(obj["duration_seconds"].GetInt64());
    }
    if (IsArrayField(obj, "messages")) {
        // Only replace messages if the JSON explicitly provides them.
        config.messages.clear();
        for (const auto& msg : obj["messages"].GetArray()) {
            if (msg.IsString()) {
                config.messages.emplace_back(msg.GetString());
            }
        }
    }
    if (IsBoolField(obj, "rotate_messages")) {
        config.rotate_messages = obj["rotate_messages"].GetBool();
    }

    return config;
}

IdleConfig ReadIdleConfig(const Value& obj) {
    IdleConfig config = DefaultConfig().idle;

    if (IsBoolField(obj, "enabled")) {
        config.enabled = obj["enabled"].GetBool();
    }
    if (IsNumberField(obj, "threshold_seconds")) {
        config.threshold = DurationFromSeconds(obj["threshold_seconds"].GetInt64());
    }
    if (IsBoolField(obj, "pause_on_idle")) {
        config.pause_on_idle = obj["pause_on_idle"].GetBool();
    }
    if (IsBoolField(obj, "reset_on_idle")) {
        config.reset_on_idle = obj["reset_on_idle"].GetBool();
    }
    if (IsBoolField(obj, "show_timer")) {
        config.show_timer = obj["show_timer"].GetBool();
    }

    return config;
}

NotificationConfig ReadNotificationConfig(const Value& obj) {
    NotificationConfig config = DefaultConfig().notification;

    if (IsBoolField(obj, "enabled")) {
        config.enabled = obj["enabled"].GetBool();
    }
    if (IsNumberField(obj, "warning_seconds")) {
        config.warning_time = DurationFromSeconds(obj["warning_seconds"].GetInt64());
    }
    if (IsBoolField(obj, "respect_dnd")) {
        config.respect_dnd = obj["respect_dnd"].GetBool();
    }

    return config;
}

ThemeConfig ReadThemeConfig(const Value& obj) {
    ThemeConfig config = DefaultConfig().theme;

    if (IsBoolField(obj, "follow_system")) {
        config.follow_system = obj["follow_system"].GetBool();
    }
    if (IsBoolField(obj, "dark_mode")) {
        config.dark_mode = obj["dark_mode"].GetBool();
    }

    return config;
}

OverlayConfig ReadOverlayConfig(const Value& obj) {
    OverlayConfig config = DefaultConfig().overlay;

    if (IsNumberField(obj, "opacity")) {
        config.opacity = obj["opacity"].GetFloat();
    }
    if (IsBoolField(obj, "show_on_all_monitors")) {
        config.show_on_all_monitors = obj["show_on_all_monitors"].GetBool();
    }
    if (IsBoolField(obj, "allow_skip")) {
        config.allow_skip = obj["allow_skip"].GetBool();
    }
    if (IsBoolField(obj, "allow_snooze")) {
        config.allow_snooze = obj["allow_snooze"].GetBool();
    }
    if (IsNumberField(obj, "snooze_duration_seconds")) {
        config.snooze_duration = DurationFromSeconds(obj["snooze_duration_seconds"].GetInt64());
    }

    return config;
}

void WriteMessagesArray(Value& array, rapidjson::Document::AllocatorType& allocator,
                        const std::vector<std::string>& messages) {
    for (const auto& msg : messages) {
        Value entry;
        entry.SetString(msg.c_str(), static_cast<rapidjson::SizeType>(msg.size()), allocator);
        array.PushBack(entry, allocator);
    }
}

void WriteBreakConfig(Value& obj, const BreakConfig& config,
                      rapidjson::Document::AllocatorType& allocator) {
    obj.SetObject();

    obj.AddMember("enabled", config.enabled, allocator);
    obj.AddMember("interval_seconds", SecondsFromDuration(config.interval), allocator);
    obj.AddMember("duration_seconds", SecondsFromDuration(config.duration), allocator);

    Value messages(rapidjson::kArrayType);
    WriteMessagesArray(messages, allocator, config.messages);
    obj.AddMember("messages", messages, allocator);

    obj.AddMember("rotate_messages", config.rotate_messages, allocator);
}

void WriteIdleConfig(Value& obj, const IdleConfig& config,
                     rapidjson::Document::AllocatorType& allocator) {
    obj.SetObject();

    obj.AddMember("enabled", config.enabled, allocator);
    obj.AddMember("threshold_seconds", SecondsFromDuration(config.threshold), allocator);
    obj.AddMember("pause_on_idle", config.pause_on_idle, allocator);
    obj.AddMember("reset_on_idle", config.reset_on_idle, allocator);
    obj.AddMember("show_timer", config.show_timer, allocator);
}

void WriteNotificationConfig(Value& obj, const NotificationConfig& config,
                             rapidjson::Document::AllocatorType& allocator) {
    obj.SetObject();

    obj.AddMember("enabled", config.enabled, allocator);
    obj.AddMember("warning_seconds", SecondsFromDuration(config.warning_time), allocator);
    obj.AddMember("respect_dnd", config.respect_dnd, allocator);
}

void WriteThemeConfig(Value& obj, const ThemeConfig& config,
                      rapidjson::Document::AllocatorType& allocator) {
    obj.SetObject();

    obj.AddMember("follow_system", config.follow_system, allocator);
    obj.AddMember("dark_mode", config.dark_mode, allocator);
}

void WriteOverlayConfig(Value& obj, const OverlayConfig& config,
                        rapidjson::Document::AllocatorType& allocator) {
    obj.SetObject();

    obj.AddMember("opacity", config.opacity, allocator);
    obj.AddMember("show_on_all_monitors", config.show_on_all_monitors, allocator);
    obj.AddMember("allow_skip", config.allow_skip, allocator);
    obj.AddMember("allow_snooze", config.allow_snooze, allocator);
    obj.AddMember("snooze_duration_seconds", SecondsFromDuration(config.snooze_duration),
                  allocator);
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

    std::ifstream file(path);
    if (!file.is_open()) {
        return std::unexpected(
            ConfigError{.message = "Failed to open configuration file", .field = path.string()});
    }

    try {
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return ParseJson(content);
    } catch (const std::exception& e) {
        return std::unexpected(ConfigError{
            .message = std::string("Failed to read file: ") + e.what(), .field = path.string()});
    }
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

    std::ofstream file(path);
    if (!file.is_open()) {
        return std::unexpected(
            ConfigError{.message = "Failed to open file for writing", .field = path.string()});
    }

    try {
        file << ToJson(config);
        spdlog::debug("Configuration saved successfully");
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(ConfigError{
            .message = std::string("Failed to write file: ") + e.what(), .field = path.string()});
    }
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
    Document doc;
    doc.Parse(json_string.c_str());

    if (doc.HasParseError() || !doc.IsObject()) {
        return std::unexpected(ConfigError{.message = "JSON parse error", .field = ""});
    }

    AppConfig config = GetDefault();

    if (IsObjectField(doc, "short_break")) {
        config.short_break = ReadBreakConfig(doc["short_break"]);
    }
    if (IsObjectField(doc, "long_break")) {
        config.long_break = ReadBreakConfig(doc["long_break"]);
    }
    if (IsObjectField(doc, "idle")) {
        config.idle = ReadIdleConfig(doc["idle"]);
    }
    if (IsObjectField(doc, "notification")) {
        config.notification = ReadNotificationConfig(doc["notification"]);
    }
    if (IsObjectField(doc, "theme")) {
        config.theme = ReadThemeConfig(doc["theme"]);
    }
    if (IsObjectField(doc, "overlay")) {
        config.overlay = ReadOverlayConfig(doc["overlay"]);
    }

    if (IsBoolField(doc, "start_minimized")) {
        config.start_minimized = doc["start_minimized"].GetBool();
    }
    if (IsBoolField(doc, "auto_start")) {
        config.auto_start = doc["auto_start"].GetBool();
    }

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
    Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    Value short_break(rapidjson::kObjectType);
    Value long_break(rapidjson::kObjectType);
    Value idle(rapidjson::kObjectType);
    Value notification(rapidjson::kObjectType);
    Value theme(rapidjson::kObjectType);
    Value overlay(rapidjson::kObjectType);

    WriteBreakConfig(short_break, config.short_break, allocator);
    WriteBreakConfig(long_break, config.long_break, allocator);
    WriteIdleConfig(idle, config.idle, allocator);
    WriteNotificationConfig(notification, config.notification, allocator);
    WriteThemeConfig(theme, config.theme, allocator);
    WriteOverlayConfig(overlay, config.overlay, allocator);

    doc.AddMember("short_break", short_break, allocator);
    doc.AddMember("long_break", long_break, allocator);
    doc.AddMember("idle", idle, allocator);
    doc.AddMember("notification", notification, allocator);
    doc.AddMember("theme", theme, allocator);
    doc.AddMember("overlay", overlay, allocator);
    doc.AddMember("start_minimized", config.start_minimized, allocator);
    doc.AddMember("auto_start", config.auto_start, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
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
