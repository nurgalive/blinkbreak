/// @file test_state_machine.cpp
/// @brief Unit tests for the StateMachine class.

#include <memory>

#include <gtest/gtest.h>

#include "core/state_machine.hpp"

namespace blinkbreak {
namespace {

using namespace std::chrono_literals;

/// @brief Test fixture for StateMachine tests.
class StateMachineTest : public ::testing::Test {
protected:
    void SetUp() override { sm_ = std::make_unique<StateMachine>(); }

    std::unique_ptr<StateMachine> sm_;
};

// ============================================================================
// Initial State Tests
// ============================================================================

/// @test StateMachine initializes to Idle state.
TEST_F(StateMachineTest, InitializesToIdleState) {
    EXPECT_EQ(sm_->GetCurrentState(), State::kIdle);
}

// ============================================================================
// Valid Transition Tests
// ============================================================================

/// @test Idle -> Running on StartEvent.
TEST_F(StateMachineTest, IdleToRunningOnStart) {
    auto result = sm_->ProcessEvent(StartEvent{});

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.new_state, State::kRunning);
    EXPECT_EQ(sm_->GetCurrentState(), State::kRunning);
}

/// @test Running -> Paused on PauseEvent.
TEST_F(StateMachineTest, RunningToPausedOnPause) {
    sm_->ProcessEvent(StartEvent{});
    auto result = sm_->ProcessEvent(PauseEvent{});

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.new_state, State::kPaused);
}

/// @test Paused -> Running on ResumeEvent.
TEST_F(StateMachineTest, PausedToRunningOnResume) {
    sm_->ProcessEvent(StartEvent{});
    sm_->ProcessEvent(PauseEvent{});
    auto result = sm_->ProcessEvent(ResumeEvent{});

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.new_state, State::kRunning);
}

/// @test Running -> BreakActive on TimerExpiredEvent.
TEST_F(StateMachineTest, RunningToBreakActiveOnTimerExpired) {
    sm_->ProcessEvent(StartEvent{});
    auto result = sm_->ProcessEvent(TimerExpiredEvent{BreakType::kShort});

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.new_state, State::kBreakActive);
}

/// @test BreakActive -> Running on BreakCompletedEvent.
TEST_F(StateMachineTest, BreakActiveToRunningOnBreakCompleted) {
    sm_->ProcessEvent(StartEvent{});
    sm_->ProcessEvent(TimerExpiredEvent{BreakType::kShort});
    auto result = sm_->ProcessEvent(BreakCompletedEvent{});

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.new_state, State::kRunning);
}

/// @test BreakActive -> Running on SkipEvent.
TEST_F(StateMachineTest, BreakActiveToRunningOnSkip) {
    sm_->ProcessEvent(StartEvent{});
    sm_->ProcessEvent(TimerExpiredEvent{BreakType::kShort});
    auto result = sm_->ProcessEvent(SkipEvent{});

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.new_state, State::kRunning);
}

/// @test BreakActive -> Snoozed on SnoozeEvent.
TEST_F(StateMachineTest, BreakActiveToSnoozedOnSnooze) {
    sm_->ProcessEvent(StartEvent{});
    sm_->ProcessEvent(TimerExpiredEvent{BreakType::kShort});
    auto result = sm_->ProcessEvent(SnoozeEvent{5min});

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.new_state, State::kSnoozed);
}

/// @test Snoozed -> BreakActive on SnoozeExpiredEvent.
TEST_F(StateMachineTest, SnoozedToBreakActiveOnSnoozeExpired) {
    sm_->ProcessEvent(StartEvent{});
    sm_->ProcessEvent(TimerExpiredEvent{BreakType::kShort});
    sm_->ProcessEvent(SnoozeEvent{5min});
    auto result = sm_->ProcessEvent(SnoozeExpiredEvent{});

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.new_state, State::kBreakActive);
}

/// @test Running -> Idle on ResetEvent.
TEST_F(StateMachineTest, RunningToIdleOnReset) {
    sm_->ProcessEvent(StartEvent{});
    auto result = sm_->ProcessEvent(ResetEvent{});

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.new_state, State::kIdle);
}

/// @test Running -> Paused on UserIdleEvent.
TEST_F(StateMachineTest, RunningToPausedOnUserIdle) {
    sm_->ProcessEvent(StartEvent{});
    auto result = sm_->ProcessEvent(UserIdleEvent{});

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.new_state, State::kPaused);
}

/// @test Paused -> Running on UserActiveEvent.
TEST_F(StateMachineTest, PausedToRunningOnUserActive) {
    sm_->ProcessEvent(StartEvent{});
    sm_->ProcessEvent(UserIdleEvent{});
    auto result = sm_->ProcessEvent(UserActiveEvent{});

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.new_state, State::kRunning);
}

// ============================================================================
// Invalid Transition Tests
// ============================================================================

/// @test Cannot pause from Idle state.
TEST_F(StateMachineTest, CannotPauseFromIdle) {
    auto result = sm_->ProcessEvent(PauseEvent{});

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.new_state, State::kIdle);
    EXPECT_TRUE(result.error.has_value());
}

/// @test Cannot start from Running state.
TEST_F(StateMachineTest, CannotStartFromRunning) {
    sm_->ProcessEvent(StartEvent{});
    auto result = sm_->ProcessEvent(StartEvent{});

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.new_state, State::kRunning);
}

/// @test Cannot reset from Idle state.
TEST_F(StateMachineTest, CannotResetFromIdle) {
    auto result = sm_->ProcessEvent(ResetEvent{});

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.new_state, State::kIdle);
}

/// @test Cannot snooze from Running state.
TEST_F(StateMachineTest, CannotSnoozeFromRunning) {
    sm_->ProcessEvent(StartEvent{});
    auto result = sm_->ProcessEvent(SnoozeEvent{5min});

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.new_state, State::kRunning);
}

// ============================================================================
// CanTransition Tests
// ============================================================================

/// @test CanTransition returns true for valid transitions.
TEST_F(StateMachineTest, CanTransitionReturnsTrueForValid) {
    EXPECT_TRUE(sm_->CanTransition(StartEvent{}));
    EXPECT_FALSE(sm_->CanTransition(PauseEvent{}));
}

// ============================================================================
// Callback Tests
// ============================================================================

/// @test State change callback is invoked.
TEST_F(StateMachineTest, StateChangeCallbackInvoked) {
    bool callback_called = false;
    State received_old_state = State::kIdle;
    State received_new_state = State::kIdle;

    sm_->SetOnStateChange([&](State old_state, State new_state, const Event&) {
        callback_called = true;
        received_old_state = old_state;
        received_new_state = new_state;
    });

    sm_->ProcessEvent(StartEvent{});

    EXPECT_TRUE(callback_called);
    EXPECT_EQ(received_old_state, State::kIdle);
    EXPECT_EQ(received_new_state, State::kRunning);
}

/// @test Callback not called on invalid transition.
TEST_F(StateMachineTest, CallbackNotCalledOnInvalidTransition) {
    bool callback_called = false;

    sm_->SetOnStateChange([&](State, State, const Event&) { callback_called = true; });

    sm_->ProcessEvent(PauseEvent{});  // Invalid from Idle

    EXPECT_FALSE(callback_called);
}

// ============================================================================
// ForceState Tests
// ============================================================================

/// @test ForceState changes state without validation.
TEST_F(StateMachineTest, ForceStateChangesWithoutValidation) {
    sm_->ForceState(State::kBreakActive);
    EXPECT_EQ(sm_->GetCurrentState(), State::kBreakActive);
}

// ============================================================================
// StateToString Tests
// ============================================================================

/// @test StateToString returns correct strings.
TEST_F(StateMachineTest, StateToStringReturnsCorrectStrings) {
    EXPECT_EQ(StateToString(State::kIdle), "Idle");
    EXPECT_EQ(StateToString(State::kRunning), "Running");
    EXPECT_EQ(StateToString(State::kPaused), "Paused");
    EXPECT_EQ(StateToString(State::kSnoozed), "Snoozed");
    EXPECT_EQ(StateToString(State::kBreakActive), "BreakActive");
}

// ============================================================================
// Complex Workflow Tests
// ============================================================================

/// @test Full break workflow: Idle -> Running -> BreakActive -> Running.
TEST_F(StateMachineTest, FullBreakWorkflow) {
    EXPECT_TRUE(sm_->ProcessEvent(StartEvent{}).success);
    EXPECT_EQ(sm_->GetCurrentState(), State::kRunning);

    EXPECT_TRUE(sm_->ProcessEvent(TimerExpiredEvent{BreakType::kShort}).success);
    EXPECT_EQ(sm_->GetCurrentState(), State::kBreakActive);

    EXPECT_TRUE(sm_->ProcessEvent(BreakCompletedEvent{}).success);
    EXPECT_EQ(sm_->GetCurrentState(), State::kRunning);
}

/// @test Snooze workflow: BreakActive -> Snoozed -> BreakActive.
TEST_F(StateMachineTest, SnoozeWorkflow) {
    sm_->ProcessEvent(StartEvent{});
    sm_->ProcessEvent(TimerExpiredEvent{BreakType::kShort});

    EXPECT_TRUE(sm_->ProcessEvent(SnoozeEvent{5min}).success);
    EXPECT_EQ(sm_->GetCurrentState(), State::kSnoozed);

    EXPECT_TRUE(sm_->ProcessEvent(SnoozeExpiredEvent{}).success);
    EXPECT_EQ(sm_->GetCurrentState(), State::kBreakActive);
}

}  // namespace
}  // namespace blinkbreak