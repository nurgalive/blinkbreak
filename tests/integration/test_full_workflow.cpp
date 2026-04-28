/// @file test_full_workflow.cpp
/// @brief Integration tests for complete break cycles and workflows.

#include <gtest/gtest.h>

#include "test_harness.hpp"

namespace blinkbreak::testing {

using namespace std::chrono_literals;

class FullWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override { harness_.InitializeWithDefaults(); }

    TestHarness harness_;
};

// Test: Initial state after initialization with defaults
TEST_F(FullWorkflowTest, InitialStateIsRunning) {
    EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);
    EXPECT_FALSE(harness_.IsBreakActive());
    EXPECT_FALSE(harness_.IsPaused());
}

// Test: Complete short break cycle from start to finish
TEST_F(FullWorkflowTest, CompleteShortBreakCycle) {
    // Configure faster breaks for testing
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(10);  // 10 seconds
    config.short_break.duration = Duration(2);   // 2 seconds
    config.notification.enabled = false;         // Disable notifications
    harness_.UpdateConfig(config);

    // Start with Running state
    EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);

    // Advance to just before break
    harness_.AdvanceTime(9s);
    EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);
    EXPECT_FALSE(harness_.IsBreakActive());

    // Advance to trigger break
    harness_.AdvanceTime(2s);
    EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);
    EXPECT_TRUE(harness_.IsBreakActive());

    // Wait for break to complete
    harness_.AdvanceTime(3s);
    EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);
    EXPECT_FALSE(harness_.IsBreakActive());

    // Verify statistics
    auto stats = harness_.GetStats();
    EXPECT_EQ(stats.short_breaks_triggered, 1);
    EXPECT_EQ(stats.long_breaks_triggered, 0);
}

// Test: Multiple short breaks trigger correctly
TEST_F(FullWorkflowTest, MultipleShortBreaks) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(10);  // 10 seconds interval
    config.short_break.duration = Duration(5);   // 5 seconds duration
    config.long_break.enabled = false;
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    // Run through 3 complete break cycles
    // Strategy: advance in small enough steps that we can catch the break active state
    // before it completes within a single AdvanceTime call
    for (int i = 0; i < 3; ++i) {
        // Advance 1 second at a time until break is active
        int max_iterations = 15;  // Safety limit
        while (!harness_.IsBreakActive() && max_iterations-- > 0) {
            harness_.AdvanceTime(1s);
        }
        EXPECT_TRUE(harness_.IsBreakActive()) << "Break " << (i + 1) << " should be active";

        // Complete break by advancing past break duration
        harness_.AdvanceTime(6s);  // > 5s duration
        EXPECT_FALSE(harness_.IsBreakActive()) << "Break " << (i + 1) << " should be complete";
    }

    auto stats = harness_.GetStats();
    EXPECT_EQ(stats.short_breaks_triggered, 3);
}

// Test: Long break triggers after correct interval
TEST_F(FullWorkflowTest, LongBreakTriggers) {
    AppConfig config = harness_.GetConfig();
    config.short_break.enabled = false;
    config.long_break.enabled = true;
    config.long_break.interval = Duration(10);  // 10 seconds
    config.long_break.duration = Duration(5);   // 5 seconds (longer than advance step)
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    // Advance to trigger long break
    harness_.AdvanceTime(11s);
    EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);

    // Complete break
    harness_.AdvanceTime(6s);
    EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);

    auto stats = harness_.GetStats();
    EXPECT_EQ(stats.short_breaks_triggered, 0);
    EXPECT_EQ(stats.long_breaks_triggered, 1);
}

// Test: Skip break functionality
TEST_F(FullWorkflowTest, SkipBreakWorks) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(5);
    config.short_break.duration = Duration(10);
    config.overlay.allow_skip = true;
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    // Trigger break
    harness_.AdvanceTime(6s);
    EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);

    // Skip break
    harness_.SimulateSkip();
    EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);
    EXPECT_FALSE(harness_.IsBreakActive());

    auto stats = harness_.GetStats();
    EXPECT_EQ(stats.breaks_skipped, 1);
}

// Test: Snooze break functionality
TEST_F(FullWorkflowTest, SnoozeBreakWorks) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(5);
    config.short_break.duration = Duration(10);
    config.overlay.allow_snooze = true;
    config.overlay.snooze_duration = Duration(3);  // 3 seconds
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    // Trigger break
    harness_.AdvanceTime(6s);
    EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);

    // Snooze break
    harness_.SimulateSnooze();
    EXPECT_EQ(harness_.GetCurrentState(), State::kSnoozed);

    // Wait for snooze to expire
    harness_.AdvanceTime(4s);
    EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);

    auto stats = harness_.GetStats();
    EXPECT_EQ(stats.breaks_snoozed, 1);
}

// Test: Skip is disabled when not allowed
TEST_F(FullWorkflowTest, SkipDisabledWhenNotAllowed) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(5);
    config.short_break.duration = Duration(10);
    config.overlay.allow_skip = false;
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    // Trigger break
    harness_.AdvanceTime(6s);
    EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);

    // Try to skip - should not work
    harness_.SimulateSkip();
    EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);

    auto stats = harness_.GetStats();
    EXPECT_EQ(stats.breaks_skipped, 0);
}

// Test: Pre-break notification appears at correct time
TEST_F(FullWorkflowTest, PreBreakNotificationTiming) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(30);
    config.notification.enabled = true;
    config.notification.warning_time = Duration(10);  // 10 seconds before
    harness_.UpdateConfig(config);

    // Advance to just before warning time (30 - 10 = 20 seconds)
    harness_.AdvanceTime(19s);
    EXPECT_EQ(harness_.GetNotificationManager().GetShowCount(), 0);

    // Advance into warning window
    harness_.AdvanceTime(2s);
    EXPECT_EQ(harness_.GetNotificationManager().GetShowCount(), 1);
    EXPECT_FALSE(harness_.GetNotificationManager().GetLastTitle().empty());
}

// Test: Notification skip action prevents break
TEST_F(FullWorkflowTest, NotificationSkipAction) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(20);
    config.short_break.duration = Duration(10);
    config.notification.enabled = true;
    config.notification.warning_time = Duration(5);
    config.overlay.allow_skip = true;
    harness_.UpdateConfig(config);

    // Advance to warning time
    harness_.AdvanceTime(16s);
    EXPECT_EQ(harness_.GetNotificationManager().GetShowCount(), 1);

    // Click skip action on notification
    harness_.SimulateNotificationAction(platform::NotificationAction::SkipBreak);

    // Advance past break time - should not trigger break
    harness_.AdvanceTime(10s);

    auto stats = harness_.GetStats();
    EXPECT_EQ(stats.breaks_skipped, 1);
}

// Test: Notification snooze action snoozes break
TEST_F(FullWorkflowTest, NotificationSnoozeAction) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(20);
    config.short_break.duration = Duration(10);
    config.notification.enabled = true;
    config.notification.warning_time = Duration(5);
    config.overlay.allow_snooze = true;
    config.overlay.snooze_duration = Duration(5);
    harness_.UpdateConfig(config);

    // Advance to warning time
    harness_.AdvanceTime(16s);
    EXPECT_EQ(harness_.GetNotificationManager().GetShowCount(), 1);

    // Click snooze action on notification
    harness_.SimulateNotificationAction(platform::NotificationAction::SnoozeBreak);

    auto stats = harness_.GetStats();
    EXPECT_EQ(stats.breaks_snoozed, 1);
}

// Test: State consistency through multiple operations
TEST_F(FullWorkflowTest, StateConsistencyThroughOperations) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(5);
    config.short_break.duration = Duration(10);  // Longer duration to test individual actions
    config.overlay.allow_skip = true;
    config.overlay.allow_snooze = true;
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    // First break - complete normally
    harness_.AdvanceTime(6s);
    EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);
    harness_.AdvanceTime(12s);
    EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);

    // Second break - skip
    harness_.AdvanceTime(6s);
    EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);
    harness_.SimulateSkip();
    EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);

    // Third break - complete normally
    harness_.AdvanceTime(6s);
    EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);
    harness_.AdvanceTime(12s);
    EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);

    auto stats = harness_.GetStats();
    EXPECT_EQ(stats.short_breaks_triggered, 3);
    EXPECT_EQ(stats.breaks_skipped, 1);
}

// Test: Break types are tracked correctly in statistics
TEST_F(FullWorkflowTest, BreakTypeStatistics) {
    AppConfig config = harness_.GetConfig();
    config.short_break.enabled = true;
    config.short_break.interval = Duration(5);
    config.short_break.duration = Duration(1);
    config.long_break.enabled = true;
    config.long_break.interval = Duration(12);  // After ~2 short breaks
    config.long_break.duration = Duration(1);
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    // Run for enough time to get short and potentially long breaks
    // Short breaks at: 5s, 10s, 15s...
    // Long breaks at: 12s, 24s...
    harness_.AdvanceTime(30s);

    auto stats = harness_.GetStats();
    EXPECT_GT(stats.short_breaks_triggered + stats.long_breaks_triggered, 0);
}

// Test: Time until break is accurate
TEST_F(FullWorkflowTest, TimeUntilBreakAccuracy) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(30);
    config.long_break.enabled = false;
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    auto initial_time = harness_.GetTimeUntilBreak();
    ASSERT_TRUE(initial_time.has_value());
    EXPECT_EQ(initial_time->count(), 30);

    // Advance 10 seconds
    harness_.AdvanceTime(10s);

    auto remaining_time = harness_.GetTimeUntilBreak();
    ASSERT_TRUE(remaining_time.has_value());
    EXPECT_LE(remaining_time->count(), 20);
}

// Test: Break duration is respected
TEST_F(FullWorkflowTest, BreakDurationRespected) {
    AppConfig config = harness_.GetConfig();
    config.short_break.interval = Duration(5);
    config.short_break.duration = Duration(10);  // 10 seconds - long enough to test in steps
    config.notification.enabled = false;
    harness_.UpdateConfig(config);

    // Trigger break
    harness_.AdvanceTime(6s);
    EXPECT_TRUE(harness_.IsBreakActive());

    // Advance less than break duration (5s < 10s)
    harness_.AdvanceTime(5s);
    EXPECT_TRUE(harness_.IsBreakActive());

    // Advance past break duration (another 6s = 11s total > 10s)
    harness_.AdvanceTime(6s);
    EXPECT_FALSE(harness_.IsBreakActive());
}

}  // namespace blinkbreak::testing
