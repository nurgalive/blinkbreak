/// @file test_dnd_integration.cpp
/// @brief Integration tests for Do Not Disturb functionality.

#include <gtest/gtest.h>

#include "test_harness.hpp"

namespace blinkbreak::testing
{

using namespace std::chrono_literals;

class DndIntegrationTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    AppConfig config;
    config.short_break.enabled = true;
    config.short_break.interval = Duration(10);
    config.short_break.duration = Duration(5);
    config.long_break.enabled = false;
    config.notification.enabled = true;
    config.notification.respect_dnd = true;
    config.notification.respect_fullscreen = false;
    config.auto_start = true;
    harness_.Initialize(config);
  }

  TestHarness harness_;
};

// Test: Break is suppressed when DND is active
TEST_F(DndIntegrationTest, BreakSuppressedDuringDnd)
{
  harness_.SetDndActive(true);

  // Advance past break time
  harness_.AdvanceTime(15s);

  // Break should not trigger
  EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);
  EXPECT_FALSE(harness_.IsBreakActive());

  auto stats = harness_.GetStats();
  EXPECT_EQ(stats.dnd_suppressions, 1);
  EXPECT_EQ(stats.short_breaks_triggered, 0);
}

// Test: Break triggers normally when DND is disabled
TEST_F(DndIntegrationTest, BreakTriggersWhenDndDisabled)
{
  harness_.SetDndActive(false);

  // Advance past break time
  harness_.AdvanceTime(12s);

  // Break should trigger
  EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);

  auto stats = harness_.GetStats();
  EXPECT_EQ(stats.dnd_suppressions, 0);
  EXPECT_EQ(stats.short_breaks_triggered, 1);
}

// Test: Notification suppressed during DND
TEST_F(DndIntegrationTest, NotificationSuppressedDuringDnd)
{
  AppConfig config = harness_.GetConfig();
  config.notification.warning_time = Duration(5);
  harness_.UpdateConfig(config);

  harness_.SetDndActive(true);

  // Advance into warning window
  harness_.AdvanceTime(7s);

  // Notification should not have been shown
  EXPECT_EQ(harness_.GetNotificationManager().GetShowCount(), 0);
}

// Test: Notification shows when DND is off
TEST_F(DndIntegrationTest, NotificationShowsWhenDndOff)
{
  AppConfig config = harness_.GetConfig();
  config.notification.warning_time = Duration(5);
  harness_.UpdateConfig(config);

  harness_.SetDndActive(false);

  // Advance into warning window
  harness_.AdvanceTime(7s);

  // Notification should have been shown
  EXPECT_EQ(harness_.GetNotificationManager().GetShowCount(), 1);
}

// Test: Fullscreen detection suppresses when enabled
TEST_F(DndIntegrationTest, FullscreenSuppressionWhenEnabled)
{
  AppConfig config = harness_.GetConfig();
  config.notification.respect_fullscreen = true;
  harness_.UpdateConfig(config);

  harness_.SetFullScreenActive(true);

  // Advance past break time
  harness_.AdvanceTime(15s);

  // Break should not trigger
  EXPECT_EQ(harness_.GetCurrentState(), State::kRunning);

  auto stats = harness_.GetStats();
  EXPECT_GE(stats.dnd_suppressions, 1);
}

// Test: Fullscreen detection does not suppress when disabled
TEST_F(DndIntegrationTest, FullscreenNoSuppressionWhenDisabled)
{
  AppConfig config = harness_.GetConfig();
  config.notification.respect_fullscreen = false;
  harness_.UpdateConfig(config);

  harness_.SetFullScreenActive(true);

  // Advance past break time
  harness_.AdvanceTime(12s);

  // Break should trigger
  EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);
}

// Test: DND state changes are tracked
TEST_F(DndIntegrationTest, DndStateChangeTracking)
{
  // Initial state
  EXPECT_FALSE(harness_.GetDndDetector().IsDndActive());

  // Enable various DND states
  harness_.SetDndState(platform::DndState::PresentationMode);
  EXPECT_TRUE(harness_.GetDndDetector().IsDndActive());

  harness_.SetDndState(platform::DndState::Busy);
  EXPECT_TRUE(harness_.GetDndDetector().IsDndActive());

  harness_.SetDndState(platform::DndState::FullScreenD3D);
  EXPECT_TRUE(harness_.GetDndDetector().IsDndActive());

  harness_.SetDndState(platform::DndState::AcceptsNotifications);
  EXPECT_FALSE(harness_.GetDndDetector().IsDndActive());
}

// Test: DND disabled in config allows breaks
TEST_F(DndIntegrationTest, DndDisabledInConfigAllowsBreaks)
{
  AppConfig config = harness_.GetConfig();
  config.notification.respect_dnd = false;
  harness_.UpdateConfig(config);

  harness_.SetDndActive(true);

  // Advance past break time
  harness_.AdvanceTime(12s);

  // Break should trigger despite DND being active
  EXPECT_EQ(harness_.GetCurrentState(), State::kBreakActive);

  auto stats = harness_.GetStats();
  EXPECT_EQ(stats.dnd_suppressions, 0);
}

// Test: DND toggling during countdown
TEST_F(DndIntegrationTest, DndTogglingDuringCountdown)
{
  AppConfig config = harness_.GetConfig();
  config.short_break.interval = Duration(20);
  harness_.UpdateConfig(config);

  // Advance 10 seconds
  harness_.AdvanceTime(10s);

  // Enable DND
  harness_.SetDndActive(true);

  // Advance 5 seconds (still running, DND active)
  harness_.AdvanceTime(5s);

  // Disable DND
  harness_.SetDndActive(false);

  // Advance past break time
  harness_.AdvanceTime(10s);

  // Break should trigger now that DND is off
  auto stats = harness_.GetStats();
  EXPECT_GE(stats.short_breaks_triggered, 1);
}

// Test: Multiple DND suppressions tracked
TEST_F(DndIntegrationTest, MultipleDndSuppressionsTracked)
{
  harness_.SetDndActive(true);

  // Multiple break times pass
  harness_.AdvanceTime(12s);  // First suppression
  harness_.AdvanceTime(12s);  // Second suppression
  harness_.AdvanceTime(12s);  // Third suppression

  auto stats = harness_.GetStats();
  EXPECT_GE(stats.dnd_suppressions, 3);
  EXPECT_EQ(stats.short_breaks_triggered, 0);
}

// Test: DND callback is triggered
TEST_F(DndIntegrationTest, DndCallbackTriggered)
{
  bool callback_triggered = false;
  bool last_state = false;

  harness_.GetDndDetector().SetOnDndChange(
      [&](bool is_active)
      {
        callback_triggered = true;
        last_state = is_active;
      });

  harness_.SetDndActive(true);
  EXPECT_TRUE(callback_triggered);
  EXPECT_TRUE(last_state);

  callback_triggered = false;
  harness_.SetDndActive(false);
  EXPECT_TRUE(callback_triggered);
  EXPECT_FALSE(last_state);
}

// Test: Presentation mode is detected as DND
TEST_F(DndIntegrationTest, PresentationModeDetectedAsDnd)
{
  harness_.SetDndState(platform::DndState::PresentationMode);
  EXPECT_TRUE(harness_.GetDndDetector().IsDndActive());

  harness_.AdvanceTime(12s);

  auto stats = harness_.GetStats();
  EXPECT_GE(stats.dnd_suppressions, 1);
}

// Test: Quiet time is detected as DND
TEST_F(DndIntegrationTest, QuietTimeDetectedAsDnd)
{
  harness_.SetDndState(platform::DndState::QuietTime);
  EXPECT_TRUE(harness_.GetDndDetector().IsDndActive());
}

// Test: Not present state is detected as DND
TEST_F(DndIntegrationTest, NotPresentDetectedAsDnd)
{
  harness_.SetDndState(platform::DndState::NotPresent);
  EXPECT_TRUE(harness_.GetDndDetector().IsDndActive());
}

// Test: DND and fullscreen combined
TEST_F(DndIntegrationTest, DndAndFullscreenCombined)
{
  AppConfig config = harness_.GetConfig();
  config.notification.respect_dnd = true;
  config.notification.respect_fullscreen = true;
  harness_.UpdateConfig(config);

  // Both DND and fullscreen active
  harness_.SetDndActive(true);
  harness_.SetFullScreenActive(true);

  harness_.AdvanceTime(12s);

  auto stats = harness_.GetStats();
  EXPECT_GE(stats.dnd_suppressions, 1);
  EXPECT_EQ(stats.short_breaks_triggered, 0);
}

}  // namespace blinkbreak::testing
