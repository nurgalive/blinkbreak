/// @file test_timer.cpp
/// @brief Unit tests for the Timer class.

#include <chrono>
#include <memory>

#include <gtest/gtest.h>

#include "core/timer.hpp"

namespace blinkbreak {
namespace {

using namespace std::chrono_literals;

/// @brief Test fixture for Timer tests.
class TimerTest : public ::testing::Test {
protected:
    void SetUp() override { timer_ = std::make_unique<Timer>(60s); }

    std::unique_ptr<Timer> timer_;
};

/// @test Timer initializes with correct duration.
TEST_F(TimerTest, InitializesWithCorrectDuration) {
    EXPECT_EQ(timer_->GetTotalDuration(), 60s);
    EXPECT_EQ(timer_->GetRemaining(), 60s);
    EXPECT_FALSE(timer_->IsRunning());
}

/// @test Timer starts correctly.
TEST_F(TimerTest, StartsCorrectly) {
    timer_->Start();
    EXPECT_TRUE(timer_->IsRunning());
}

/// @test Timer pauses correctly.
TEST_F(TimerTest, PausesCorrectly) {
    timer_->Start();
    timer_->Pause();
    EXPECT_FALSE(timer_->IsRunning());
}

/// @test Timer resets to initial duration.
TEST_F(TimerTest, ResetsToInitialDuration) {
    timer_->Start();
    timer_->Update(30s);
    timer_->Reset();
    EXPECT_EQ(timer_->GetRemaining(), 60s);
    EXPECT_FALSE(timer_->IsRunning());
}

/// @test Timer decrements when running.
TEST_F(TimerTest, DecrementsWhenRunning) {
    timer_->Start();
    timer_->Update(10s);
    EXPECT_EQ(timer_->GetRemaining(), 50s);
}

/// @test Timer does not decrement when paused.
TEST_F(TimerTest, DoesNotDecrementWhenPaused) {
    timer_->Update(10s);
    EXPECT_EQ(timer_->GetRemaining(), 60s);
}

/// @test Timer calls expired callback when time runs out.
TEST_F(TimerTest, CallsExpiredCallback) {
    bool callback_called = false;
    timer_->SetOnExpired([&callback_called]() { callback_called = true; });

    timer_->Start();
    timer_->Update(60s);

    EXPECT_TRUE(callback_called);
    EXPECT_EQ(timer_->GetRemaining(), Duration::zero());
    EXPECT_FALSE(timer_->IsRunning());
}

/// @test Timer calls tick callback on update.
TEST_F(TimerTest, CallsTickCallback) {
    Duration last_remaining = Duration::zero();
    timer_->SetOnTick([&last_remaining](Duration remaining) { last_remaining = remaining; });

    timer_->Start();
    timer_->Update(10s);

    EXPECT_EQ(last_remaining, 50s);
}

/// @test Timer handles overflow gracefully.
TEST_F(TimerTest, HandlesOverflowGracefully) {
    timer_->Start();
    timer_->Update(120s);  // More than total duration

    EXPECT_EQ(timer_->GetRemaining(), Duration::zero());
    EXPECT_FALSE(timer_->IsRunning());
}

/// @test Timer is movable.
TEST_F(TimerTest, IsMovable) {
    timer_->Start();
    timer_->Update(30s);

    Timer moved_timer = std::move(*timer_);

    EXPECT_EQ(moved_timer.GetRemaining(), 30s);
    EXPECT_TRUE(moved_timer.IsRunning());
}

}  // namespace
}  // namespace blinkbreak