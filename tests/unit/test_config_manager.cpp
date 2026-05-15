/// @file test_config_manager.cpp
/// @brief Unit tests for the ConfigManager class.

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "core/config_manager.hpp"

namespace blinkbreak {
namespace {

using namespace std::chrono_literals;

/// @brief Test fixture for ConfigManager tests.
class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_manager_ = std::make_unique<ConfigManager>();
        test_dir_ = std::filesystem::temp_directory_path() / "blinkbreak_test";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override { std::filesystem::remove_all(test_dir_); }

    /// @brief Creates a test config file with given content.
    std::filesystem::path CreateTestFile(const std::string& content,
                                         const std::string& filename = "test_config.json") {
        auto path = test_dir_ / filename;
        std::ofstream file(path);
        file << content;
        return path;
    }

    std::unique_ptr<ConfigManager> config_manager_;
    std::filesystem::path test_dir_;
};

// ============================================================================
// Default Configuration Tests
// ============================================================================

/// @test GetDefault returns valid configuration.
TEST_F(ConfigManagerTest, GetDefaultReturnsValidConfig) {
    auto config = ConfigManager::GetDefault();

    EXPECT_TRUE(config.short_break.enabled);
    EXPECT_EQ(config.short_break.interval, 600s);
    EXPECT_EQ(config.short_break.duration, 20s);
    EXPECT_FALSE(config.short_break.messages.empty());

    EXPECT_TRUE(config.long_break.enabled);
    EXPECT_EQ(config.long_break.interval, 3600s);
    EXPECT_EQ(config.long_break.duration, 300s);

    EXPECT_TRUE(config.idle.enabled);
    EXPECT_EQ(config.idle.threshold, 180s);

    EXPECT_TRUE(config.notification.enabled);
    EXPECT_EQ(config.notification.warning_time, 30s);

    EXPECT_TRUE(config.theme.follow_system);
    EXPECT_FALSE(config.theme.dark_mode);

    EXPECT_FLOAT_EQ(config.overlay.opacity, 0.7f);
}

/// @test Notification defaults are correct.
TEST_F(ConfigManagerTest, NotificationConfigDefaults) {
    auto config = ConfigManager::GetDefault();

    EXPECT_TRUE(config.notification.enabled);
    EXPECT_EQ(config.notification.warning_time, 30s);
    EXPECT_TRUE(config.notification.respect_dnd);
    EXPECT_FALSE(config.notification.respect_fullscreen);
}

/// @test GetDefault configuration passes validation.
TEST_F(ConfigManagerTest, DefaultConfigPassesValidation) {
    auto config = ConfigManager::GetDefault();
    auto errors = config_manager_->Validate(config);
    EXPECT_TRUE(errors.empty());
}

/// @test GetDefaultPath returns non-empty path.
TEST_F(ConfigManagerTest, GetDefaultPathReturnsNonEmpty) {
    auto path = ConfigManager::GetDefaultPath();
    EXPECT_FALSE(path.empty());
    EXPECT_TRUE(path.string().find("config.json") != std::string::npos);
}

// ============================================================================
// JSON Parsing Tests
// ============================================================================

/// @test ParseJson parses valid JSON.
TEST_F(ConfigManagerTest, ParseJsonParsesValidJson) {
    std::string json = R"({
        "short_break": {
            "enabled": true,
            "interval_seconds": 300,
            "duration_seconds": 15,
            "messages": ["Test message"],
            "rotate_messages": false
        },
        "long_break": {
            "enabled": false,
            "interval_seconds": 1800,
            "duration_seconds": 120,
            "messages": ["Long break"],
            "rotate_messages": true
        }
    })";

    auto result = config_manager_->ParseJson(json);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->short_break.enabled);
    EXPECT_EQ(result->short_break.interval, 300s);
    EXPECT_EQ(result->short_break.duration, 15s);
    EXPECT_FALSE(result->short_break.rotate_messages);
    EXPECT_FALSE(result->long_break.enabled);
}

/// @test ParseJson returns error for invalid JSON.
TEST_F(ConfigManagerTest, ParseJsonReturnsErrorForInvalidJson) {
    std::string invalid_json = "{ invalid json }";

    auto result = config_manager_->ParseJson(invalid_json);

    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().message.empty());
}

/// @test ParseJson merges with defaults for missing fields.
TEST_F(ConfigManagerTest, ParseJsonMergesWithDefaults) {
    std::string minimal_json = R"({
        "short_break": {
            "enabled": true,
            "interval_seconds": 300,
            "duration_seconds": 15
        }
    })";

    auto result = config_manager_->ParseJson(minimal_json);

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->short_break.messages.empty());
    EXPECT_TRUE(result->theme.follow_system);
    EXPECT_FALSE(result->theme.dark_mode);
}

/// @test ParseJson reads theme configuration.
TEST_F(ConfigManagerTest, ParseJsonReadsThemeConfig) {
    std::string json = R"({
        "theme": {
            "follow_system": false,
            "dark_mode": true
        }
    })";

    auto result = config_manager_->ParseJson(json);

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->theme.follow_system);
    EXPECT_TRUE(result->theme.dark_mode);
}

/// @test ParseJson reads notification configuration.
TEST_F(ConfigManagerTest, ParseJsonReadsNotificationConfig) {
    std::string json = R"({
        "notification": {
            "enabled": false,
            "warning_seconds": 45,
            "respect_dnd": false,
            "respect_fullscreen": true
        }
    })";

    auto result = config_manager_->ParseJson(json);

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->notification.enabled);
    EXPECT_EQ(result->notification.warning_time, 45s);
    EXPECT_FALSE(result->notification.respect_dnd);
    EXPECT_TRUE(result->notification.respect_fullscreen);
}

// ============================================================================
// ToJson Tests
// ============================================================================

/// @test ToJson produces valid JSON.
TEST_F(ConfigManagerTest, ToJsonProducesValidJson) {
    auto config = ConfigManager::GetDefault();
    std::string json = config_manager_->ToJson(config);

    auto result = config_manager_->ParseJson(json);
    EXPECT_TRUE(result.has_value());
}

/// @test ToJson roundtrip preserves values.
TEST_F(ConfigManagerTest, ToJsonRoundtripPreservesValues) {
    auto original = ConfigManager::GetDefault();
    original.short_break.interval = 123s;
    original.overlay.opacity = 0.5f;
    original.theme.follow_system = false;
    original.theme.dark_mode = true;
    original.notification.respect_fullscreen = true;

    std::string json = config_manager_->ToJson(original);
    auto parsed = config_manager_->ParseJson(json);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->short_break.interval, 123s);
    EXPECT_FLOAT_EQ(parsed->overlay.opacity, 0.5f);
    EXPECT_FALSE(parsed->theme.follow_system);
    EXPECT_TRUE(parsed->theme.dark_mode);
    EXPECT_TRUE(parsed->notification.respect_fullscreen);
}

// ============================================================================
// File Operations Tests
// ============================================================================

/// @test Load reads configuration from file.
TEST_F(ConfigManagerTest, LoadReadsFromFile) {
    std::string json = R"({
        "short_break": {
            "enabled": true,
            "interval_seconds": 400,
            "duration_seconds": 25,
            "messages": ["File message"]
        }
    })";
    auto path = CreateTestFile(json);

    auto result = config_manager_->Load(path);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->short_break.interval, 400s);
}

/// @test Load returns error for non-existent file.
TEST_F(ConfigManagerTest, LoadReturnsErrorForNonExistentFile) {
    auto result = config_manager_->Load(test_dir_ / "nonexistent.json");

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().message.find("not found") != std::string::npos);
}

/// @test Save writes configuration to file.
TEST_F(ConfigManagerTest, SaveWritesToFile) {
    auto config = ConfigManager::GetDefault();
    auto path = test_dir_ / "saved_config.json";

    auto result = config_manager_->Save(config, path);

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(std::filesystem::exists(path));
}

/// @test Save creates parent directories.
TEST_F(ConfigManagerTest, SaveCreatesParentDirectories) {
    auto config = ConfigManager::GetDefault();
    auto path = test_dir_ / "subdir" / "nested" / "config.json";

    auto result = config_manager_->Save(config, path);

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(std::filesystem::exists(path));
}

/// @test Save rejects invalid configuration.
TEST_F(ConfigManagerTest, SaveRejectsInvalidConfig) {
    auto config = ConfigManager::GetDefault();
    config.overlay.opacity = 2.0f;
    auto path = test_dir_ / "invalid_config.json";

    auto result = config_manager_->Save(config, path);

    EXPECT_FALSE(result.has_value());
}

/// @test Load and Save roundtrip.
TEST_F(ConfigManagerTest, LoadSaveRoundtrip) {
    auto original = ConfigManager::GetDefault();
    original.short_break.interval = 999s;
    auto path = test_dir_ / "roundtrip.json";

    auto save_result = config_manager_->Save(original, path);
    EXPECT_TRUE(save_result);
    auto loaded = config_manager_->Load(path);

    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->short_break.interval, 999s);
}

// ============================================================================
// Validation Tests
// ============================================================================

/// @test Validate detects invalid break interval.
TEST_F(ConfigManagerTest, ValidateDetectsInvalidBreakInterval) {
    auto config = ConfigManager::GetDefault();
    config.short_break.interval = Duration::zero();

    auto errors = config_manager_->Validate(config);

    EXPECT_FALSE(errors.empty());
    EXPECT_TRUE(errors[0].field.find("interval") != std::string::npos);
}

/// @test Validate detects break duration >= interval.
TEST_F(ConfigManagerTest, ValidateDetectsDurationGreaterThanInterval) {
    auto config = ConfigManager::GetDefault();
    config.short_break.duration = config.short_break.interval;

    auto errors = config_manager_->Validate(config);

    EXPECT_FALSE(errors.empty());
}

/// @test Validate detects invalid opacity.
TEST_F(ConfigManagerTest, ValidateDetectsInvalidOpacity) {
    auto config = ConfigManager::GetDefault();
    config.overlay.opacity = 1.5f;

    auto errors = config_manager_->Validate(config);

    EXPECT_FALSE(errors.empty());
    EXPECT_TRUE(errors[0].field.find("opacity") != std::string::npos);
}

/// @test Validate detects empty messages.
TEST_F(ConfigManagerTest, ValidateDetectsEmptyMessages) {
    auto config = ConfigManager::GetDefault();
    config.short_break.messages.clear();

    auto errors = config_manager_->Validate(config);

    EXPECT_FALSE(errors.empty());
    EXPECT_TRUE(errors[0].field.find("messages") != std::string::npos);
}

/// @test Validate skips disabled breaks.
TEST_F(ConfigManagerTest, ValidateSkipsDisabledBreaks) {
    auto config = ConfigManager::GetDefault();
    config.short_break.enabled = false;
    config.short_break.interval = Duration::zero();

    auto errors = config_manager_->Validate(config);

    for (const auto& error : errors) {
        EXPECT_TRUE(error.field.find("short_break.interval") == std::string::npos);
    }
}

/// @test Validate detects long break interval <= short break interval.
TEST_F(ConfigManagerTest, ValidateDetectsLongBreakShorterThanShort) {
    auto config = ConfigManager::GetDefault();
    config.short_break.interval = 3600s;
    config.long_break.interval = 1800s;

    auto errors = config_manager_->Validate(config);

    EXPECT_FALSE(errors.empty());
}

/// @test Validate detects invalid idle threshold.
TEST_F(ConfigManagerTest, ValidateDetectsInvalidIdleThreshold) {
    auto config = ConfigManager::GetDefault();
    config.idle.enabled = true;
    config.idle.threshold = Duration::zero();

    auto errors = config_manager_->Validate(config);

    EXPECT_FALSE(errors.empty());
    EXPECT_TRUE(errors[0].field.find("idle") != std::string::npos);
}

/// @test Validate detects invalid snooze duration.
TEST_F(ConfigManagerTest, ValidateDetectsInvalidSnoozeDuration) {
    auto config = ConfigManager::GetDefault();
    config.overlay.allow_snooze = true;
    config.overlay.snooze_duration = Duration::zero();

    auto errors = config_manager_->Validate(config);

    EXPECT_FALSE(errors.empty());
    EXPECT_TRUE(errors[0].field.find("snooze") != std::string::npos);
}

// ============================================================================
// Reset Breaks on Idle Tests
// ============================================================================

/// @test Reset-on-idle defaults are correct.
TEST_F(ConfigManagerTest, ResetOnIdleDefaults) {
    auto config = ConfigManager::GetDefault();

    EXPECT_TRUE(config.idle.reset_on_idle);
    EXPECT_EQ(config.idle.reset_threshold, 1200s);  // 20 minutes
}

/// @test Validate detects reset threshold <= regular idle threshold.
TEST_F(ConfigManagerTest, ValidateDetectsResetOnIdleThresholdTooSmall) {
    auto config = ConfigManager::GetDefault();
    config.idle.enabled = true;
    config.idle.threshold = 180s;
    config.idle.reset_on_idle = true;
    config.idle.reset_threshold = 60s;  // Less than regular threshold

    auto errors = config_manager_->Validate(config);

    EXPECT_FALSE(errors.empty());
    bool found_reset_threshold_error = false;
    for (const auto& error : errors) {
        if (error.field.find("reset_threshold") != std::string::npos) {
            found_reset_threshold_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_reset_threshold_error);
}

/// @test Validate accepts reset threshold > regular idle threshold.
TEST_F(ConfigManagerTest, ValidateAcceptsValidResetOnIdleThreshold) {
    auto config = ConfigManager::GetDefault();
    config.idle.enabled = true;
    config.idle.threshold = 180s;
    config.idle.reset_on_idle = true;
    config.idle.reset_threshold = 1200s;  // Greater than regular threshold

    auto errors = config_manager_->Validate(config);

    // Should not have reset_threshold errors
    for (const auto& error : errors) {
        EXPECT_TRUE(error.field.find("reset_threshold") == std::string::npos)
            << "Unexpected reset_threshold error: " << error.message;
    }
}

/// @test Validate skips reset-on-idle validation when disabled.
TEST_F(ConfigManagerTest, ValidateSkipsResetOnIdleWhenDisabled) {
    auto config = ConfigManager::GetDefault();
    config.idle.enabled = true;
    config.idle.threshold = 180s;
    config.idle.reset_on_idle = false;
    config.idle.reset_threshold = 60s;  // Invalid but should be skipped

    auto errors = config_manager_->Validate(config);

    // Should not have reset_threshold errors because feature is disabled
    for (const auto& error : errors) {
        EXPECT_TRUE(error.field.find("reset_threshold") == std::string::npos)
            << "Unexpected reset_threshold error when disabled: " << error.message;
    }
}

/// @test ParseJson reads reset-on-idle settings.
TEST_F(ConfigManagerTest, ParseJsonReadsResetOnIdleConfig) {
    std::string json = R"({
        "idle": {
            "enabled": true,
            "threshold_seconds": 180,
            "pause_on_idle": true,
            "reset_on_idle": false,
            "reset_threshold_seconds": 900
        }
    })";

    auto result = config_manager_->ParseJson(json);
    ASSERT_TRUE(result.has_value());

    EXPECT_FALSE(result->idle.reset_on_idle);
    EXPECT_EQ(result->idle.reset_threshold, 900s);
}

/// @test ToJson roundtrip preserves reset-on-idle values.
TEST_F(ConfigManagerTest, ToJsonRoundtripPreservesResetOnIdleValues) {
    auto config = ConfigManager::GetDefault();
    config.idle.reset_on_idle = false;
    config.idle.reset_threshold = 1800s;  // 30 minutes

    auto json = config_manager_->ToJson(config);
    auto result = config_manager_->ParseJson(json);

    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->idle.reset_on_idle);
    EXPECT_EQ(result->idle.reset_threshold, 1800s);
}

}  // namespace
}  // namespace blinkbreak
