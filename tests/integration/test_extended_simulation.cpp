/// @file test_extended_simulation.cpp
/// @brief Extended simulation tests for stress testing and long-running scenarios.

#include <gtest/gtest.h>

#include "test_harness.hpp"

namespace blinkbreak::testing {

using namespace std::chrono_literals;

class ExtendedSimulationTest : public ::testing::Test {
protected:
    TestHarness harness_;
};

// Test: Simulate 8-hour workday
TEST_F(ExtendedSimulationTest, EightHourWorkday) {
    AppConfig config;
    config.short_break.enabled = true;
    config.short_break.interval = Duration(20 * 60);  // 20 minutes
    config.short_break.duration = Duration(20);       // 20 seconds
    config.long_break.enabled = true;
    config.long_break.interval = Duration(60 * 60);  // 60 minutes
    config.long_break.duration = Duration(5 * 60);   // 5 minutes
    config.notification.enabled = false;
    config.auto_start = true;
    harness_.Initialize(config);

    // Simulate 8 hours (use larger step for performance)
    harness_.AdvanceTime(8h, 1s);

    auto stats = harness_.GetStats();

    // With long breaks consuming time (5 min each), fewer short breaks trigger.
    // In 8 hours: ~8 long breaks (8h / 60min) using 8 * 5min = 40 min of break time.
    // Remaining ~7h20min / 20min = ~22 potential short breaks, but long breaks replace
    // some short breaks. Expect ~15-24 short breaks depending on timing.
    EXPECT_GE(stats.short_breaks_triggered, 12);
    EXPECT_LE(stats.short_breaks_triggered, 30);

    // Should have approximately 8 long breaks (8h / 60min)
    EXPECT_GE(stats.long_breaks_triggered, 6);
    EXPECT_LE(stats.long_breaks_triggered, 10);

    // Total simulated time should be approximately 8 hours
    EXPECT_GE(stats.total_simulated_time.count(), 8 * 60 * 60 * 1000 - 1000);
}

// Test: Intermittent idle periods during workday
TEST_F(ExtendedSimulationTest, IntermittentIdlePeriods) {
    AppConfig config;
    config.short_break.enabled = true;
    config.short_break.interval = Duration(5 * 60);  // 5 minutes
    config.short_break.duration = Duration(10);
    config.long_break.enabled = false;
    config.idle.enabled = true;
    config.idle.threshold = Duration(2 * 60);  // 2 minutes
    config.idle.pause_on_idle = true;
    config.notification.enabled = false;
    config.auto_start = true;
    harness_.Initialize(config);

    // Work for 15 minutes
    harness_.AdvanceTime(15min, 500ms);

    // Go idle for 5 minutes (lunch)
    harness_.SimulateIdle(5min);
    harness_.AdvanceTime(5min, 500ms);
    harness_.SimulateActivity();

    // Work for another 15 minutes
    harness_.AdvanceTime(15min, 500ms);

    auto stats = harness_.GetStats();
    EXPECT_GE(stats.idle_pauses, 1);
    EXPECT_GE(stats.short_breaks_triggered, 4);
}

// Test: Multiple DND periods during workday
TEST_F(ExtendedSimulationTest, MultipleDndPeriods) {
    AppConfig config;
    config.short_break.enabled = true;
    config.short_break.interval = Duration(5 * 60);  // 5 minutes
    config.short_break.duration = Duration(10);
    config.long_break.enabled = false;
    config.notification.respect_dnd = true;
    config.notification.enabled = false;
    config.auto_start = true;
    harness_.Initialize(config);

    // Work normally for 10 minutes
    harness_.AdvanceTime(10min, 500ms);

    // Enter meeting (DND on) for 30 minutes
    harness_.SetDndActive(true);
    harness_.AdvanceTime(30min, 500ms);
    harness_.SetDndActive(false);

    // Work normally for 10 minutes
    harness_.AdvanceTime(10min, 500ms);

    auto stats = harness_.GetStats();
    // DND should have suppressed breaks during meeting
    EXPECT_GE(stats.dnd_suppressions, 5);
}

// Test: State consistency after many cycles
TEST_F(ExtendedSimulationTest, StateConsistencyAfterManyCycles) {
    AppConfig config;
    config.short_break.enabled = true;
    config.short_break.interval = Duration(1);  // 1 second
    config.short_break.duration = Duration(1);  // 1 second (use 1s minimum)
    config.long_break.enabled = false;
    config.notification.enabled = false;
    config.auto_start = true;
    harness_.Initialize(config);

    // Run 500 break cycles (faster than 1000 for test performance)
    harness_.AdvanceTime(1000s, 100ms);

    auto stats = harness_.GetStats();
    EXPECT_GE(stats.short_breaks_triggered, 400);

    // State should be valid
    auto state = harness_.GetCurrentState();
    EXPECT_TRUE(state == State::kRunning || state == State::kBreakActive);
}

// Test: Skip and snooze patterns
TEST_F(ExtendedSimulationTest, SkipAndSnoozePatterns) {
    AppConfig config;
    config.short_break.enabled = true;
    config.short_break.interval = Duration(10);
    config.short_break.duration = Duration(5);
    config.long_break.enabled = false;
    config.overlay.allow_skip = true;
    config.overlay.allow_snooze = true;
    config.overlay.snooze_duration = Duration(5);
    config.notification.enabled = false;
    config.auto_start = true;
    harness_.Initialize(config);

    // First break - skip
    harness_.AdvanceTime(12s);
    harness_.SimulateSkip();

    // Second break - snooze then complete
    harness_.AdvanceTime(12s);
    harness_.SimulateSnooze();
    harness_.AdvanceTime(10s);  // Snooze expires, break reappears and completes

    // Third break - complete normally
    harness_.AdvanceTime(15s);

    auto stats = harness_.GetStats();
    EXPECT_EQ(stats.breaks_skipped, 1);
    EXPECT_EQ(stats.breaks_snoozed, 1);
    EXPECT_GE(stats.short_breaks_triggered, 3);
}

// Test: Mixed idle and DND scenarios
TEST_F(ExtendedSimulationTest, MixedIdleAndDnd) {
    AppConfig config;
    config.short_break.enabled = true;
    config.short_break.interval = Duration(30);
    config.short_break.duration = Duration(5);
    config.long_break.enabled = false;
    config.idle.enabled = true;
    config.idle.threshold = Duration(10);
    config.idle.pause_on_idle = true;
    config.notification.respect_dnd = true;
    config.notification.enabled = false;
    config.auto_start = true;
    harness_.Initialize(config);

    // Normal operation
    harness_.AdvanceTime(15s);

    // Go idle
    harness_.SimulateIdle(15s);
    harness_.AdvanceTime(10s);

    // DND during idle
    harness_.SetDndActive(true);

    // Come back active with DND still on
    harness_.SimulateActivity();
    harness_.AdvanceTime(20s);

    // Turn off DND
    harness_.SetDndActive(false);
    harness_.AdvanceTime(40s);

    // Should have some breaks now
    auto stats = harness_.GetStats();
    EXPECT_GE(stats.idle_pauses, 1);
    // DND may have suppressed some breaks
}

// Test: Rapid config changes during operation
TEST_F(ExtendedSimulationTest, RapidConfigChanges) {
    AppConfig config;
    config.short_break.enabled = true;
    config.short_break.interval = Duration(20);
    config.short_break.duration = Duration(5);
    config.long_break.enabled = false;
    config.notification.enabled = false;
    config.auto_start = true;
    harness_.Initialize(config);

    // Make rapid config changes while advancing time
    for (int i = 0; i < 20; ++i) {
        config.overlay.opacity = 0.5f + (i % 5) * 0.1f;
        harness_.UpdateConfig(config);
        harness_.AdvanceTime(5s);
    }

    // State should remain consistent
    auto state = harness_.GetCurrentState();
    EXPECT_TRUE(state == State::kRunning || state == State::kBreakActive);

    auto stats = harness_.GetStats();
    EXPECT_GE(stats.short_breaks_triggered, 1);
}

// Test: Recovery from edge cases
TEST_F(ExtendedSimulationTest, RecoveryFromEdgeCases) {
    AppConfig config;
    config.short_break.enabled = true;
    config.short_break.interval = Duration(10);
    config.short_break.duration = Duration(5);
    config.long_break.enabled = false;
    config.overlay.allow_skip = true;
    config.overlay.allow_snooze = true;
    config.overlay.snooze_duration = Duration(10);  // Short snooze for test
    config.notification.enabled = false;
    config.auto_start = true;
    harness_.Initialize(config);

    // Trigger break
    harness_.AdvanceTime(12s);
    EXPECT_TRUE(harness_.IsBreakActive());

    // Skip during break
    harness_.SimulateSkip();
    EXPECT_FALSE(harness_.IsBreakActive());

    // Another break
    harness_.AdvanceTime(12s);
    EXPECT_TRUE(harness_.IsBreakActive());

    // Snooze
    harness_.SimulateSnooze();

    // Complete after snooze (snooze is 10s + break is 5s)
    harness_.AdvanceTime(20s);

    // Normal operation should continue
    EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);
}

// Test: Long break correctly interleaves with short breaks
TEST_F(ExtendedSimulationTest, LongBreakInterleavesCorrectly) {
    AppConfig config;
    config.short_break.enabled = true;
    config.short_break.interval = Duration(5);
    config.short_break.duration = Duration(1);
    config.long_break.enabled = true;
    config.long_break.interval = Duration(15);  // Every 3 short breaks
    config.long_break.duration = Duration(2);
    config.notification.enabled = false;
    config.auto_start = true;
    harness_.Initialize(config);

    // Run for enough time to see both break types
    harness_.AdvanceTime(60s);

    auto stats = harness_.GetStats();
    EXPECT_GE(stats.short_breaks_triggered, 3);
    EXPECT_GE(stats.long_breaks_triggered, 1);
}

// Test: Statistics accuracy over extended period
TEST_F(ExtendedSimulationTest, StatisticsAccuracyOverTime) {
    AppConfig config;
    config.short_break.enabled = true;
    config.short_break.interval = Duration(10);
    config.short_break.duration = Duration(2);
    config.long_break.enabled = false;
    config.notification.enabled = false;
    config.auto_start = true;
    harness_.Initialize(config);

    auto initial_stats = harness_.GetStats();
    EXPECT_EQ(initial_stats.short_breaks_triggered, 0);
    EXPECT_EQ(initial_stats.total_simulated_time.count(), 0);

    // Run for 2 minutes
    harness_.AdvanceTime(2min);

    auto final_stats = harness_.GetStats();
    // Should have approximately 12 breaks (120s / 10s)
    EXPECT_GE(final_stats.short_breaks_triggered, 10);
    EXPECT_LE(final_stats.short_breaks_triggered, 14);

    // Total simulated time should match
    EXPECT_GE(final_stats.total_simulated_time.count(), 119000);  // ~2 min in ms
}

// Test: Memory stability hint (no actual memory check, just runs)
TEST_F(ExtendedSimulationTest, MemoryStabilityHint) {
    AppConfig config;
    config.short_break.enabled = true;
    config.short_break.interval = Duration(1);
    config.short_break.duration = Duration(1);
    config.long_break.enabled = false;
    config.notification.enabled = false;
    config.auto_start = true;
    harness_.Initialize(config);

    // Run many cycles to check for obvious issues
    for (int i = 0; i < 100; ++i) {
        harness_.AdvanceTime(10s, 500ms);
        harness_.ResetStats();
    }

    // If we get here without crashing, basic stability is OK
    EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);
}

}  // namespace blinkbreak::testing
