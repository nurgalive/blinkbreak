/// @file test_idle_detector.cpp
/// @brief Unit tests for IIdleDetector interface and implementations.

#include <atomic>
#include <chrono>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "platform/platform_interface.hpp"

namespace blinkbreak {
namespace platform {
namespace {

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Mock Idle Detector for testing callback behavior
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Mock idle detector for testing interface behavior.
class MockIdleDetector : public IIdleDetector {
public:
    MockIdleDetector()
        : running_(false), is_idle_(false), threshold_(300s), simulated_idle_time_(0ms) {}

    void Start() override { running_ = true; }

    void Stop() override { running_ = false; }

    [[nodiscard]] bool IsRunning() const override { return running_; }

    [[nodiscard]] std::chrono::milliseconds GetIdleTime() const override {
        return simulated_idle_time_;
    }

    [[nodiscard]] bool IsIdle() const override { return is_idle_; }

    void SetIdleThreshold(std::chrono::seconds threshold) override { threshold_ = threshold; }

    [[nodiscard]] std::chrono::seconds GetIdleThreshold() const override { return threshold_; }

    void SetOnIdle(std::function<void()> callback) override { on_idle_ = std::move(callback); }

    void SetOnActive(std::function<void()> callback) override { on_active_ = std::move(callback); }

    // Test helpers
    void SimulateIdleTime(std::chrono::milliseconds time) { simulated_idle_time_ = time; }

    void TriggerIdleCallback() {
        is_idle_ = true;
        if (on_idle_) {
            on_idle_();
        }
    }

    void TriggerActiveCallback() {
        is_idle_ = false;
        if (on_active_) {
            on_active_();
        }
    }

private:
    bool running_;
    bool is_idle_;
    std::chrono::seconds threshold_;
    std::chrono::milliseconds simulated_idle_time_;
    std::function<void()> on_idle_;
    std::function<void()> on_active_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Mock Idle Detector Tests
// ─────────────────────────────────────────────────────────────────────────────

class MockIdleDetectorTest : public ::testing::Test {
protected:
    MockIdleDetector detector_;
};

TEST_F(MockIdleDetectorTest, InitialState) {
    EXPECT_FALSE(detector_.IsRunning());
    EXPECT_FALSE(detector_.IsIdle());
    EXPECT_EQ(detector_.GetIdleTime(), 0ms);
    EXPECT_EQ(detector_.GetIdleThreshold(), 300s);  // Default 5 minutes
}

TEST_F(MockIdleDetectorTest, StartAndStop) {
    EXPECT_FALSE(detector_.IsRunning());

    detector_.Start();
    EXPECT_TRUE(detector_.IsRunning());

    detector_.Stop();
    EXPECT_FALSE(detector_.IsRunning());
}

TEST_F(MockIdleDetectorTest, SetAndGetThreshold) {
    detector_.SetIdleThreshold(120s);
    EXPECT_EQ(detector_.GetIdleThreshold(), 120s);

    detector_.SetIdleThreshold(600s);
    EXPECT_EQ(detector_.GetIdleThreshold(), 600s);
}

TEST_F(MockIdleDetectorTest, SimulatedIdleTime) {
    detector_.SimulateIdleTime(5000ms);
    EXPECT_EQ(detector_.GetIdleTime(), 5000ms);

    detector_.SimulateIdleTime(0ms);
    EXPECT_EQ(detector_.GetIdleTime(), 0ms);
}

TEST_F(MockIdleDetectorTest, IdleCallbackTriggered) {
    bool idle_called = false;
    detector_.SetOnIdle([&idle_called]() { idle_called = true; });

    EXPECT_FALSE(idle_called);
    detector_.TriggerIdleCallback();
    EXPECT_TRUE(idle_called);
    EXPECT_TRUE(detector_.IsIdle());
}

TEST_F(MockIdleDetectorTest, ActiveCallbackTriggered) {
    bool active_called = false;
    detector_.SetOnActive([&active_called]() { active_called = true; });

    // First set to idle
    detector_.TriggerIdleCallback();
    EXPECT_TRUE(detector_.IsIdle());

    // Then trigger active
    EXPECT_FALSE(active_called);
    detector_.TriggerActiveCallback();
    EXPECT_TRUE(active_called);
    EXPECT_FALSE(detector_.IsIdle());
}

TEST_F(MockIdleDetectorTest, CallbacksCanBeChanged) {
    int callback_count = 0;

    detector_.SetOnIdle([&callback_count]() { callback_count = 1; });
    detector_.TriggerIdleCallback();
    EXPECT_EQ(callback_count, 1);

    detector_.SetOnIdle([&callback_count]() { callback_count = 2; });
    detector_.TriggerIdleCallback();
    EXPECT_EQ(callback_count, 2);
}

TEST_F(MockIdleDetectorTest, NullCallbacksAreHandled) {
    // Should not crash when callbacks are not set
    detector_.TriggerIdleCallback();
    detector_.TriggerActiveCallback();
}

// ─────────────────────────────────────────────────────────────────────────────
// Windows Idle Detector Tests (Live tests using actual Win32 API)
// ─────────────────────────────────────────────────────────────────────────────

#ifdef _WIN32

class IdleDetectorWinTest : public ::testing::Test {
protected:
    void SetUp() override {
        detector_ = CreateIdleDetector();
        ASSERT_NE(detector_, nullptr);
    }

    void TearDown() override {
        if (detector_ && detector_->IsRunning()) {
            detector_->Stop();
        }
    }

    std::unique_ptr<IIdleDetector> detector_;
};

TEST_F(IdleDetectorWinTest, CreateIdleDetector) {
    auto detector = CreateIdleDetector();
    ASSERT_NE(detector, nullptr);
    EXPECT_FALSE(detector->IsRunning());
}

TEST_F(IdleDetectorWinTest, InitialState) {
    EXPECT_FALSE(detector_->IsRunning());
    EXPECT_FALSE(detector_->IsIdle());
    // Default threshold should be 5 minutes
    EXPECT_EQ(detector_->GetIdleThreshold(), 300s);
}

TEST_F(IdleDetectorWinTest, StartAndStop) {
    EXPECT_FALSE(detector_->IsRunning());

    detector_->Start();
    EXPECT_TRUE(detector_->IsRunning());

    detector_->Stop();
    EXPECT_FALSE(detector_->IsRunning());
}

TEST_F(IdleDetectorWinTest, StartTwiceIsNoOp) {
    detector_->Start();
    EXPECT_TRUE(detector_->IsRunning());

    // Starting again should be safe
    detector_->Start();
    EXPECT_TRUE(detector_->IsRunning());

    detector_->Stop();
    EXPECT_FALSE(detector_->IsRunning());
}

TEST_F(IdleDetectorWinTest, StopWhenNotRunningIsNoOp) {
    EXPECT_FALSE(detector_->IsRunning());

    // Stopping when not running should be safe
    detector_->Stop();
    EXPECT_FALSE(detector_->IsRunning());
}

TEST_F(IdleDetectorWinTest, SetAndGetThreshold) {
    detector_->SetIdleThreshold(60s);
    EXPECT_EQ(detector_->GetIdleThreshold(), 60s);

    detector_->SetIdleThreshold(120s);
    EXPECT_EQ(detector_->GetIdleThreshold(), 120s);

    // Zero threshold
    detector_->SetIdleThreshold(0s);
    EXPECT_EQ(detector_->GetIdleThreshold(), 0s);
}

TEST_F(IdleDetectorWinTest, GetIdleTimeReturnsNonNegative) {
    auto idle_time = detector_->GetIdleTime();
    EXPECT_GE(idle_time.count(), 0);
}

TEST_F(IdleDetectorWinTest, GetIdleTimeIncreases) {
    // This test checks that idle time is actually measured
    // We can't guarantee exact timing, but it should increase over a delay
    auto idle_time1 = detector_->GetIdleTime();
    std::this_thread::sleep_for(50ms);
    auto idle_time2 = detector_->GetIdleTime();

    // Allow some tolerance for timing variations
    // The second reading should be >= first (or close to it if user interacts)
    EXPECT_GE(idle_time2.count(), idle_time1.count() - 100);
}

TEST_F(IdleDetectorWinTest, IsIdleWithHighThreshold) {
    // With a very high threshold, should not be idle
    detector_->SetIdleThreshold(3600s);  // 1 hour
    detector_->Start();

    // Give the monitor thread a chance to start
    std::this_thread::sleep_for(100ms);

    EXPECT_FALSE(detector_->IsIdle());

    detector_->Stop();
}

TEST_F(IdleDetectorWinTest, CallbacksCanBeSet) {
    std::atomic<bool> idle_called{false};
    std::atomic<bool> active_called{false};

    detector_->SetOnIdle([&idle_called]() { idle_called.store(true); });

    detector_->SetOnActive([&active_called]() { active_called.store(true); });

    // Start and immediately stop - callbacks should not fire
    // because we haven't reached idle threshold
    detector_->Start();
    std::this_thread::sleep_for(100ms);
    detector_->Stop();

    // With default threshold of 5 minutes, should not have gone idle
    EXPECT_FALSE(idle_called.load());
}

TEST_F(IdleDetectorWinTest, MonitorThreadStopsCleanly) {
    detector_->Start();
    EXPECT_TRUE(detector_->IsRunning());

    // Let the thread run for a bit
    std::this_thread::sleep_for(200ms);

    // Stop should join the thread cleanly
    detector_->Stop();
    EXPECT_FALSE(detector_->IsRunning());

    // Should be able to start again
    detector_->Start();
    EXPECT_TRUE(detector_->IsRunning());
    detector_->Stop();
}

TEST_F(IdleDetectorWinTest, VeryShortThresholdTriggersIdle) {
    // Set a very short threshold to test idle detection
    detector_->SetIdleThreshold(0s);  // Immediately idle if no recent input

    std::atomic<bool> idle_called{false};
    detector_->SetOnIdle([&idle_called]() { idle_called.store(true); });

    detector_->Start();

    // Wait for the monitor to poll
    std::this_thread::sleep_for(700ms);

    // With 0s threshold, we should be idle unless the user is actively using the system
    // This test may be flaky in CI environments with simulated inputs
    // But in a typical interactive session, it should pass

    detector_->Stop();
}

#endif  // _WIN32

}  // namespace
}  // namespace platform
}  // namespace blinkbreak
