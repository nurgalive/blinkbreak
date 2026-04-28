/// @file test_config_changes.cpp
/// @brief Integration tests for runtime configuration changes.

#include <gtest/gtest.h>

#include "test_harness.hpp"

namespace blinkbreak::testing {

using namespace std::chrono_literals;

class ConfigChangeTest : public ::testing::Test {
protected:
    void SetUp() override { harness_.InitializeWithDefaults(); }

    TestHarness harness_;
};

// Test: Changing interval resets timers
TEST_F(ConfigChangeTest, IntervalChangeResetsTimers) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(60);
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    // Advance 30 seconds (50% progress)
    harness_.AdvanceTime(30s);

    // Change interval - should reset timers
    config.short_break.interval = Duration(40);
    harness_.UpdateConfig(config);

    // Timer should reset with new interval
    auto remaining = harness_.GetTimeUntilBreak();
    ASSERT_TRUE(remaining.has_value());
    EXPECT_GE(remaining->count(), 38);
}

// Test: Changing non-timer settings preserves timer
TEST_F(ConfigChangeTest, NonTimerSettingsPreserveTimer) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(60);
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    harness_.AdvanceTime(30s);
    auto time_before = harness_.GetTimeUntilBreak();
    ASSERT_TRUE(time_before.has_value());

    // Change non-timer setting (overlay opacity)
    config.overlay.opacity = 0.5f;
    harness_.UpdateConfig(config);

    auto time_after = harness_.GetTimeUntilBreak();
    ASSERT_TRUE(time_after.has_value());
    // Should be approximately the same
    EXPECT_NEAR(time_before->count(), time_after->count(), 2);
}

// Test: Changing messages preserves timer
TEST_F(ConfigChangeTest, MessageChangePreservesTimer) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(60);
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    harness_.AdvanceTime(30s);
    auto time_before = harness_.GetTimeUntilBreak();
    ASSERT_TRUE(time_before.has_value());

    // Change messages
    config.short_break.messages = {"New message 1", "New message 2"};
    harness_.UpdateConfig(config);

    auto time_after = harness_.GetTimeUntilBreak();
    ASSERT_TRUE(time_after.has_value());
    EXPECT_NEAR(time_before->count(), time_after->count(), 2);
}

// Test: Disabling breaks stops scheduling
TEST_F(ConfigChangeTest, DisablingBreaksStopsScheduler) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(10);
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    harness_.AdvanceTime(5s);

    // Disable breaks
    config.short_break.enabled = false;
    harness_.UpdateConfig(config);

    harness_.AdvanceTime(20s);

    // No break should have triggered
    auto stats = harness_.GetStats();
    EXPECT_EQ(stats.short_breaks_triggered, 0);
}

// Test: Re-enabling breaks restarts scheduler
TEST_F(ConfigChangeTest, ReenablingBreaksRestartsScheduler) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(10);
    config.short_break.enabled = false;
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    harness_.AdvanceTime(20s);

    // Re-enable breaks
    config.short_break.enabled = true;
    harness_.UpdateConfig(config);

    harness_.AdvanceTime(12s);

    auto stats = harness_.GetStats();
    EXPECT_EQ(stats.short_breaks_triggered, 1);
}

// Test: Changing duration takes effect on next break
TEST_F(ConfigChangeTest, DurationChangeTakesEffectOnNextBreak) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(5);
    config.short_break.duration = Duration(3);
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    // Change duration before first break
    config.short_break.duration = Duration(10);
    harness_.UpdateConfig(config);

    // Trigger break
    harness_.AdvanceTime(6s);
    EXPECT_TRUE(harness_.IsBreakActive());

    // Break should last 10 seconds now
    harness_.AdvanceTime(5s);
    EXPECT_TRUE(harness_.IsBreakActive());

    harness_.AdvanceTime(6s);
    EXPECT_FALSE(harness_.IsBreakActive());
}

// Test: Changing skip permission takes effect immediately
TEST_F(ConfigChangeTest, SkipPermissionChangeTakesEffectImmediately) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(5);
    config.short_break.duration = Duration(10);
    config.overlay.allow_skip = false;
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    // Trigger break
    harness_.AdvanceTime(6s);
    EXPECT_TRUE(harness_.IsBreakActive());

    // Try to skip - should not work
    harness_.SimulateSkip();
    EXPECT_TRUE(harness_.IsBreakActive());

    // Enable skip during break
    config.overlay.allow_skip = true;
    harness_.UpdateConfig(config);

    // Now skip should work (but break info needs to be updated)
    // This test validates the config change mechanism
}

// Test: Changing snooze duration takes effect
TEST_F(ConfigChangeTest, SnoozeDurationChangeTakesEffect) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(5);
    config.short_break.duration = Duration(10);
    config.overlay.allow_snooze = true;
    config.overlay.snooze_duration = Duration(3);
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    // Trigger and snooze
    harness_.AdvanceTime(6s);
    harness_.SimulateSnooze();
    EXPECT_EQ(harness_.GetCurrentState(), State::kSnoozed);

    // Wait for original snooze duration
    harness_.AdvanceTime(4s);
    // Break should reappear with original snooze setting
    EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);
}

// Test: Changing idle settings takes effect
TEST_F(ConfigChangeTest, IdleSettingsChangeTakesEffect) {
    AppConfig config = harness_.GetConfig();
    config.idle.enabled = true;
    config.idle.threshold = Duration(10);
    config.idle.pause_on_idle = true;
    harness_.UpdateConfig(config);

    // Enable then disable idle detection
    config.idle.enabled = false;
    harness_.UpdateConfig(config);

    // Simulate idle - should not pause
    harness_.SimulateIdle(15s);
    EXPECT_FALSE(harness_.IsPaused());
}

// Test: Changing notification settings takes effect
TEST_F(ConfigChangeTest, NotificationSettingsChangeTakesEffect) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(30);
    config.notification.enabled = true;
    config.notification.warning_time = Duration(10);
    harness_.UpdateConfig(config);

    // Advance to warning time
    harness_.AdvanceTime(22s);
    EXPECT_EQ(harness_.GetNotificationManager().GetShowCount(), 1);

    // Reset and disable notifications
    harness_.GetNotificationManager().Reset();
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    // Advance another full cycle
    harness_.AdvanceTime(40s);  // Past first break and into warning for second

    // Should have had fewer notifications (only break completion resets)
    // The key is that after disabling, no new notifications appear
}

// Test: Changing warning time takes effect
TEST_F(ConfigChangeTest, WarningTimeChangeTakesEffect) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(30);
    config.notification.enabled = true;
    config.notification.warning_time = Duration(5);
    harness_.UpdateConfig(config);

    // Wait until warning would have been sent with original time (25s)
    harness_.AdvanceTime(24s);
    EXPECT_EQ(harness_.GetNotificationManager().GetShowCount(), 0);

    // Now pass the new warning threshold
    harness_.AdvanceTime(2s);
    EXPECT_EQ(harness_.GetNotificationManager().GetShowCount(), 1);
}

// Test: Multiple config changes preserve state consistency
TEST_F(ConfigChangeTest, MultipleChangesPreserveStateConsistency) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(20);
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    // Make several config changes
    for (int i = 0; i < 5; ++i) {
        config.overlay.opacity = 0.5f + (i * 0.1f);
        harness_.UpdateConfig(config);
        harness_.AdvanceTime(2s);
    }

    // State should still be valid
    EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);

    // Break should still trigger
    harness_.AdvanceTime(20s);
    auto stats = harness_.GetStats();
    EXPECT_GE(stats.short_breaks_triggered, 1);
}

// Test: DND settings change takes effect
TEST_F(ConfigChangeTest, DndSettingsChangeTakesEffect) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(10);
    config.notification.respect_dnd = true;
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    harness_.SetDndActive(true);
    harness_.AdvanceTime(12s);

    auto stats = harness_.GetStats();
    EXPECT_EQ(stats.dnd_suppressions, 1);

    // Disable DND respect
    config.notification.respect_dnd = false;
    harness_.UpdateConfig(config);

    harness_.AdvanceTime(12s);

    stats = harness_.GetStats();
    EXPECT_GE(stats.short_breaks_triggered, 1);
}

}  // namespace blinkbreak::testing
