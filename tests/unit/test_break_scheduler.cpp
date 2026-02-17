/// @file test_break_scheduler.cpp
/// @brief Unit tests for the BreakScheduler class.

#include "core/break_scheduler.hpp"

#include <gtest/gtest.h>

namespace blinkbreak {
namespace {

using namespace std::chrono_literals;

/// @brief Test fixture for BreakScheduler tests.
class BreakSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        short_config_.enabled = true;
        short_config_.interval = 10s;
        short_config_.duration = 2s;
        short_config_.messages = {"Short break!"};
        short_config_.rotate_messages = true;

        long_config_.enabled = true;
        long_config_.interval = 60s;
        long_config_.duration = 5s;
        long_config_.messages = {"Long break!"};
        long_config_.rotate_messages = true;

        overlay_config_.allow_skip = true;
        overlay_config_.allow_snooze = true;
        overlay_config_.snooze_duration = 5s;

        scheduler_ = std::make_unique<BreakScheduler>(
            short_config_, long_config_, overlay_config_);
    }

    BreakConfig short_config_;
    BreakConfig long_config_;
    OverlayConfig overlay_config_;
    std::unique_ptr<BreakScheduler> scheduler_;
};

// ============================================================================
// Initial State Tests
// ============================================================================

/// @test Scheduler starts in not running state.
TEST_F(BreakSchedulerTest, StartsNotRunning) {
    EXPECT_FALSE(scheduler_->IsRunning());
    EXPECT_FALSE(scheduler_->IsBreakActive());
}

/// @test Start sets running state.
TEST_F(BreakSchedulerTest, StartSetsRunning) {
    scheduler_->Start();
    EXPECT_TRUE(scheduler_->IsRunning());
}

// ============================================================================
// Timer Tests
// ============================================================================

/// @test GetTimeUntilNextBreak returns correct time.
TEST_F(BreakSchedulerTest, GetTimeUntilNextBreakReturnsCorrectTime) {
    scheduler_->Start();
    auto time = scheduler_->GetTimeUntilNextBreak();

    ASSERT_TRUE(time.has_value());
    EXPECT_EQ(*time, 10s);  // Short break interval
}

/// @test Timer counts down on Update.
TEST_F(BreakSchedulerTest, TimerCountsDownOnUpdate) {
    scheduler_->Start();
    scheduler_->Update(5s);

    auto time = scheduler_->GetTimeUntilNextBreak();
    ASSERT_TRUE(time.has_value());
    EXPECT_EQ(*time, 5s);
}

/// @test Break triggers when timer expires.
TEST_F(BreakSchedulerTest, BreakTriggersWhenTimerExpires) {
    bool break_started = false;
    scheduler_->SetOnBreakStart([&break_started](const BreakInfo&) {
        break_started = true;
    });

    scheduler_->Start();
    scheduler_->Update(10s);

    EXPECT_TRUE(break_started);
    EXPECT_TRUE(scheduler_->IsBreakActive());
}

/// @test Break remaining time is available while active.
/// @details Ensures GetTimeUntilBreakEnds reports the active break countdown.
TEST_F(BreakSchedulerTest, BreakRemainingTimeAvailableDuringActiveBreak) {
    scheduler_->Start();
    scheduler_->Update(10s);  // Trigger short break

    auto remaining = scheduler_->GetTimeUntilBreakEnds();
    ASSERT_TRUE(remaining.has_value());
    EXPECT_EQ(*remaining, 2s);

    scheduler_->Update(1s);
    remaining = scheduler_->GetTimeUntilBreakEnds();
    ASSERT_TRUE(remaining.has_value());
    EXPECT_EQ(*remaining, 1s);
}

// ============================================================================
// Break Type Tests
// ============================================================================

/// @test GetNextBreakType returns short break first.
TEST_F(BreakSchedulerTest, GetNextBreakTypeReturnsShortFirst) {
    scheduler_->Start();
    EXPECT_EQ(scheduler_->GetNextBreakType(), BreakType::kShort);
}

/// @test When timers align, long break should be selected.
TEST_F(BreakSchedulerTest, GetNextBreakTypePrefersLongOnTie) {
    short_config_.interval = 10s;
    long_config_.interval = 10s;
    scheduler_ = std::make_unique<BreakScheduler>(
        short_config_, long_config_, overlay_config_);

    scheduler_->Start();
    EXPECT_EQ(scheduler_->GetNextBreakType(), BreakType::kLong);
}

/// @test When timers expire together, long break should trigger.
TEST_F(BreakSchedulerTest, LongBreakTriggersWhenTimersAlign) {
    short_config_.interval = 10s;
    long_config_.interval = 10s;
    scheduler_ = std::make_unique<BreakScheduler>(
        short_config_, long_config_, overlay_config_);

    BreakType triggered_type = BreakType::kShort;
    scheduler_->SetOnBreakStart([&triggered_type](const BreakInfo& info) {
        triggered_type = info.type;
    });

    scheduler_->Start();
    scheduler_->Update(10s);

    EXPECT_EQ(triggered_type, BreakType::kLong);
    EXPECT_TRUE(scheduler_->IsBreakActive());
}

/// @test Long break triggers at correct interval.
TEST_F(BreakSchedulerTest, LongBreakTriggersAtCorrectInterval) {
    short_config_.enabled = false;
    scheduler_ = std::make_unique<BreakScheduler>(
        short_config_, long_config_, overlay_config_);

    BreakType triggered_type = BreakType::kShort;
    scheduler_->SetOnBreakStart([&triggered_type](const BreakInfo& info) {
        triggered_type = info.type;
    });

    scheduler_->Start();
    scheduler_->Update(60s);

    EXPECT_EQ(triggered_type, BreakType::kLong);
}

/// @test Short break completion does not reset long timer progress.
TEST_F(BreakSchedulerTest, ShortBreakDoesNotResetLongTimer) {
    scheduler_->Start();
    scheduler_->Update(10s);  // Trigger short break

    EXPECT_TRUE(scheduler_->IsBreakActive());

    scheduler_->Update(2s);  // Complete short break duration

    auto long_time = scheduler_->GetTimeUntilLongBreak();
    ASSERT_TRUE(long_time.has_value());
    EXPECT_EQ(*long_time, 50s);
}

/// @test Long break triggers even with periodic short breaks.
TEST_F(BreakSchedulerTest, LongBreakTriggersWithShortBreaksActive) {
    long_config_.interval = 25s;
    long_config_.duration = 5s;
    scheduler_ = std::make_unique<BreakScheduler>(
        short_config_, long_config_, overlay_config_);

    BreakType triggered_type = BreakType::kShort;
    scheduler_->SetOnBreakStart([&triggered_type](const BreakInfo& info) {
        triggered_type = info.type;
    });

    scheduler_->Start();

    scheduler_->Update(10s);  // Short break at t=10
    scheduler_->Update(2s);   // Complete short break

    scheduler_->Update(10s);  // Short break at t=20
    scheduler_->Update(2s);   // Complete short break

    scheduler_->Update(5s);   // Long break at t=25

    EXPECT_EQ(triggered_type, BreakType::kLong);
    EXPECT_TRUE(scheduler_->IsBreakActive());
}

// ============================================================================
// Pause/Resume Tests
// ============================================================================

/// @test Pause stops timer.
TEST_F(BreakSchedulerTest, PauseStopsTimer) {
    scheduler_->Start();
    scheduler_->Update(5s);
    scheduler_->Pause();
    scheduler_->Update(5s);

    EXPECT_FALSE(scheduler_->IsRunning());
    auto time = scheduler_->GetTimeUntilNextBreak();
    EXPECT_FALSE(time.has_value());
}

/// @test Resume continues timer.
TEST_F(BreakSchedulerTest, ResumeContinuesTimer) {
    scheduler_->Start();
    scheduler_->Update(5s);
    scheduler_->Pause();
    scheduler_->Resume();

    EXPECT_TRUE(scheduler_->IsRunning());
    auto time = scheduler_->GetTimeUntilNextBreak();
    EXPECT_EQ(*time, 5s);
}

// ============================================================================
// Skip/Snooze Tests
// ============================================================================

/// @test SkipBreak ends active break.
TEST_F(BreakSchedulerTest, SkipBreakEndsActiveBreak) {
    scheduler_->Start();
    scheduler_->Update(10s);  // Trigger break

    EXPECT_TRUE(scheduler_->IsBreakActive());
    scheduler_->SkipBreak();
    EXPECT_FALSE(scheduler_->IsBreakActive());
}

/// @test SnoozeBreak delays break.
TEST_F(BreakSchedulerTest, SnoozeBreakDelaysBreak) {
    scheduler_->Start();
    scheduler_->Update(10s);  // Trigger break

    scheduler_->SnoozeBreak();
    EXPECT_FALSE(scheduler_->IsBreakActive());
}

/// @test Snooze extends both short and long timers.
TEST_F(BreakSchedulerTest, SnoozeExtendsTimers) {
    scheduler_->Start();
    scheduler_->Update(10s);  // Trigger short break

    scheduler_->SnoozeBreak(5s);

    auto short_remaining = scheduler_->GetTimeUntilShortBreak();
    auto long_remaining = scheduler_->GetTimeUntilLongBreak();

    ASSERT_TRUE(short_remaining.has_value());
    ASSERT_TRUE(long_remaining.has_value());
    EXPECT_EQ(*short_remaining, 5s);
    EXPECT_EQ(*long_remaining, 55s);
}

/// @test Snooze updates interval totals for progress calculations.
TEST_F(BreakSchedulerTest, SnoozeUpdatesIntervalTotals) {
    scheduler_->Start();
    scheduler_->Update(10s);  // Trigger short break

    scheduler_->SnoozeBreak(5s);

    EXPECT_EQ(scheduler_->GetShortIntervalTotal(), 15s);
    EXPECT_EQ(scheduler_->GetLongIntervalTotal(), 65s);
}

// ============================================================================
// Callback Tests
// ============================================================================

/// @test BreakInfo contains correct data.
TEST_F(BreakSchedulerTest, BreakInfoContainsCorrectData) {
    BreakInfo received_info{};
    scheduler_->SetOnBreakStart([&received_info](const BreakInfo& info) {
        received_info = info;
    });

    scheduler_->Start();
    scheduler_->Update(10s);

    EXPECT_EQ(received_info.type, BreakType::kShort);
    EXPECT_EQ(received_info.duration, 2s);
    EXPECT_EQ(received_info.message, "Short break!");
    EXPECT_TRUE(received_info.can_skip);
    EXPECT_TRUE(received_info.can_snooze);
}

/// @test Warning callback fires at correct time.
TEST_F(BreakSchedulerTest, WarningCallbackFiresAtCorrectTime) {
    bool warning_received = false;
    scheduler_->SetOnWarning([&warning_received](BreakType, Duration) {
        warning_received = true;
    }, 3s);

    scheduler_->Start();
    scheduler_->Update(7s);  // 3s remaining

    EXPECT_TRUE(warning_received);
}

// ============================================================================
// Reset Tests
// ============================================================================

/// @test Reset stops scheduler and resets timers.
TEST_F(BreakSchedulerTest, ResetStopsAndResetsTimers) {
    scheduler_->Start();
    scheduler_->Update(5s);
    scheduler_->Reset();

    EXPECT_FALSE(scheduler_->IsRunning());
    EXPECT_FALSE(scheduler_->IsBreakActive());
}

// ============================================================================
// Configuration Update Tests
// ============================================================================

/// @test UpdateConfig changes messages.
TEST_F(BreakSchedulerTest, UpdateConfigChangesMessages) {
    short_config_.messages = {"New message!"};
    scheduler_->UpdateConfig(short_config_, long_config_, overlay_config_);

    BreakInfo received_info{};
    scheduler_->SetOnBreakStart([&received_info](const BreakInfo& info) {
        received_info = info;
    });

    scheduler_->Start();
    scheduler_->Update(10s);

    EXPECT_EQ(received_info.message, "New message!");
}

}  // namespace
}  // namespace blinkbreak