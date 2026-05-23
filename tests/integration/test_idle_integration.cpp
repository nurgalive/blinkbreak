/// @file test_idle_integration.cpp
/// @brief Integration tests for idle detection functionality.

#include <gtest/gtest.h>

#include "test_harness.hpp"

namespace blinkbreak::testing
{

using namespace std::chrono_literals;

class IdleIntegrationTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    AppConfig config;
    config.short_break.enabled = true;
    config.short_break.interval = Duration(30);
    config.short_break.duration = Duration(5);
    config.long_break.enabled = false;
    config.idle.enabled = true;
    config.idle.threshold = Duration(10);  // 10 seconds
    config.idle.pause_on_idle = true;
    config.idle.reset_short_on_idle = false;
    config.idle.reset_long_on_idle = false;
    config.notification.enabled = false;
    config.auto_start = true;
    harness_.Initialize(config);
  }

  TestHarness harness_;
};

// Test: Timer pauses when user becomes idle
TEST_F(IdleIntegrationTest, TimerPausesOnIdle)
{
  // Advance some time
  harness_.AdvanceTime(15s);

  // Simulate user going idle (beyond threshold)
  harness_.SimulateIdle(15s);

  EXPECT_TRUE(harness_.IsPaused());
  EXPECT_TRUE(harness_.IsPausedByIdle());

  // Advance time while idle - timer should not progress
  harness_.AdvanceTime(20s);

  // Timer should still be paused
  EXPECT_TRUE(harness_.IsPaused());
  // When paused, GetTimeUntilBreak() returns nullopt - this is expected behavior
}

// Test: Timer resumes when user becomes active
TEST_F(IdleIntegrationTest, TimerResumesOnActive)
{
  harness_.AdvanceTime(10s);

  // Simulate idle
  harness_.SimulateIdle(15s);
  EXPECT_TRUE(harness_.IsPaused());
  EXPECT_TRUE(harness_.IsPausedByIdle());

  // Simulate user activity
  harness_.SimulateActivity();

  EXPECT_FALSE(harness_.IsPaused());
  EXPECT_FALSE(harness_.IsPausedByIdle());
  EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);
}

// Test: Timer resets after reset-on-idle threshold when configured
TEST_F(IdleIntegrationTest, TimerResetsOnIdleWhenConfigured)
{
  AppConfig config = harness_.GetConfig();
  config.idle.pause_on_idle = false;
  config.idle.reset_short_on_idle = true;
  config.idle.reset_short_threshold = Duration(12);  // 12 seconds
  config.idle.reset_long_on_idle = false;
  harness_.UpdateConfig(config);

  // Advance halfway through the interval
  harness_.AdvanceTime(20s);
  auto time_before = harness_.GetTimeUntilBreak();
  ASSERT_TRUE(time_before.has_value());
  EXPECT_LT(time_before->count(), 15);  // Should have progressed

  // Simulate idle long enough to cross reset threshold
  harness_.SimulateIdle(15s);
  harness_.AdvanceTime(1s);

  auto time_after = harness_.GetTimeUntilBreak();
  ASSERT_TRUE(time_after.has_value());
  EXPECT_GE(time_after->count(), 28);  // Should be close to full interval (30s)
}

// Test: Idle during break does not cause issues
TEST_F(IdleIntegrationTest, IdleDuringBreakHandled)
{
  AppConfig config = harness_.GetConfig();
  config.short_break.interval = Duration(5);
  config.short_break.duration = Duration(10);
  harness_.UpdateConfig(config);

  // Trigger break
  harness_.AdvanceTime(6s);
  EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);

  // Go idle during break - should not affect break
  harness_.SimulateIdle(15s);

  // Break should still be active (idle during break doesn't change state)
  // But break timer continues, so it should complete
  harness_.AdvanceTime(12s);
  EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);

  // Check that we had exactly one break
  auto stats = harness_.GetStats();
  EXPECT_EQ(stats.short_breaks_triggered, 1);
}

// Test: Statistics track idle pauses correctly
TEST_F(IdleIntegrationTest, IdlePauseStatistics)
{
  // First idle period
  harness_.SimulateIdle(15s);
  harness_.AdvanceTime(5s);
  harness_.SimulateActivity();

  // Second idle period
  harness_.AdvanceTime(5s);
  harness_.SimulateIdle(15s);
  harness_.AdvanceTime(5s);
  harness_.SimulateActivity();

  auto stats = harness_.GetStats();
  EXPECT_EQ(stats.idle_pauses, 2);
}

// Test: Idle threshold is respected
TEST_F(IdleIntegrationTest, IdleThresholdRespected)
{
  // Idle time less than threshold should not pause
  harness_.SimulateIdle(5s);  // Less than 10s threshold
  EXPECT_FALSE(harness_.IsPaused());

  // Idle time at/above threshold should pause
  harness_.SimulateIdle(12s);  // Above threshold
  EXPECT_TRUE(harness_.IsPaused());
}

// Test: Idle detection disabled works correctly
TEST_F(IdleIntegrationTest, IdleDetectionDisabled)
{
  AppConfig config = harness_.GetConfig();
  config.idle.enabled = false;
  harness_.UpdateConfig(config);

  // Even with long idle time, should not pause
  harness_.SimulateIdle(60s);
  EXPECT_FALSE(harness_.IsPaused());
}

// Test: Multiple idle/active cycles work correctly
TEST_F(IdleIntegrationTest, MultipleIdleActiveCycles)
{
  for (int i = 0; i < 3; ++i)
  {
    // Advance time
    harness_.AdvanceTime(5s);

    // Go idle
    harness_.SimulateIdle(15s);
    EXPECT_TRUE(harness_.IsPaused());

    // Come back active
    harness_.SimulateActivity();
    EXPECT_FALSE(harness_.IsPaused());
    EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);
  }

  auto stats = harness_.GetStats();
  EXPECT_EQ(stats.idle_pauses, 3);
}

// Test: Break still triggers after idle/resume cycle
TEST_F(IdleIntegrationTest, BreakTriggersAfterIdleResume)
{
  AppConfig config = harness_.GetConfig();
  config.short_break.interval = Duration(20);
  harness_.UpdateConfig(config);

  // Advance 10 seconds
  harness_.AdvanceTime(10s);

  // Go idle for a while
  harness_.SimulateIdle(15s);
  harness_.AdvanceTime(30s);

  // Come back active
  harness_.SimulateActivity();

  // Continue to trigger break
  harness_.AdvanceTime(15s);

  // Break should have triggered
  auto stats = harness_.GetStats();
  EXPECT_GE(stats.short_breaks_triggered, 1);
}

// Test: Pause on idle without reset preserves progress
TEST_F(IdleIntegrationTest, PauseOnIdlePreservesProgress)
{
  AppConfig config = harness_.GetConfig();
  config.short_break.interval = Duration(30);
  config.idle.pause_on_idle = true;
  config.idle.reset_short_on_idle = false;
  config.idle.reset_long_on_idle = false;
  harness_.UpdateConfig(config);

  // Advance 20 seconds (10 seconds remaining)
  harness_.AdvanceTime(20s);
  auto time_before = harness_.GetTimeUntilBreak();
  ASSERT_TRUE(time_before.has_value());

  // Go idle
  harness_.SimulateIdle(15s);

  // Time while idle (should not count)
  harness_.AdvanceTime(100s);

  // Come back active
  harness_.SimulateActivity();

  // Progress should be preserved
  auto time_after = harness_.GetTimeUntilBreak();
  ASSERT_TRUE(time_after.has_value());
  EXPECT_NEAR(time_before->count(), time_after->count(), 2);
}

// Test: Idle callbacks trigger at correct times
TEST_F(IdleIntegrationTest, IdleCallbacksAtCorrectTime)
{
  bool idle_triggered = false;
  bool active_triggered = false;

  harness_.GetIdleDetector().SetOnIdle([&idle_triggered]() { idle_triggered = true; });
  harness_.GetIdleDetector().SetOnActive([&active_triggered]() { active_triggered = true; });

  // Trigger idle
  harness_.SimulateIdle(15s);
  EXPECT_TRUE(idle_triggered);
  EXPECT_FALSE(active_triggered);

  // Trigger active
  harness_.SimulateActivity();
  EXPECT_TRUE(active_triggered);
}

}  // namespace blinkbreak::testing
