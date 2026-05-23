/// @file test_dnd_detector.cpp
/// @brief Unit tests for IDndDetector interface and implementations.

#include <atomic>
#include <chrono>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "platform/platform_interface.hpp"

namespace blinkbreak
{
namespace platform
{
namespace
{

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Mock DND Detector for testing callback behavior
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Mock DND detector for testing interface behavior.
class MockDndDetector : public IDndDetector
{
public:
  MockDndDetector()
      : running_(false), current_state_(DndState::AcceptsNotifications), polling_interval_(1000ms)
  {
  }

  void Start() override { running_ = true; }

  void Stop() override { running_ = false; }

  [[nodiscard]] bool IsRunning() const override { return running_; }

  [[nodiscard]] DndState GetState() const override { return current_state_; }

  [[nodiscard]] DndState RefreshState() override { return current_state_; }

  [[nodiscard]] bool IsDndActive() const override
  {
    return current_state_ != DndState::AcceptsNotifications;
  }

  [[nodiscard]] bool IsFullScreenDetected() const override
  {
    return current_state_ == DndState::Busy || current_state_ == DndState::FullScreenD3D ||
           current_state_ == DndState::WindowsStoreApp;
  }

  void SetOnDndChange(std::function<void(bool)> callback) override
  {
    on_dnd_change_ = std::move(callback);
  }

  void SetPollingInterval(std::chrono::milliseconds interval) override
  {
    polling_interval_ = interval;
  }

  [[nodiscard]] std::chrono::milliseconds GetPollingInterval() const override
  {
    return polling_interval_;
  }

  // Test helpers
  void SetState(DndState state)
  {
    const bool old_active = IsDndActive();
    current_state_ = state;
    const bool new_active = IsDndActive();

    if (old_active != new_active && on_dnd_change_)
    {
      on_dnd_change_(new_active);
    }
  }

  void TriggerCallback(bool is_active)
  {
    if (on_dnd_change_)
    {
      on_dnd_change_(is_active);
    }
  }

private:
  bool running_;
  DndState current_state_;
  std::chrono::milliseconds polling_interval_;
  std::function<void(bool)> on_dnd_change_;
};

// ─────────────────────────────────────────────────────────────────────────────
// DndStateToString Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(DndStateToStringTest, AllValues)
{
  EXPECT_STREQ(DndStateToString(DndState::NotPresent), "NotPresent");
  EXPECT_STREQ(DndStateToString(DndState::Busy), "Busy");
  EXPECT_STREQ(DndStateToString(DndState::FullScreenD3D), "FullScreenD3D");
  EXPECT_STREQ(DndStateToString(DndState::PresentationMode), "PresentationMode");
  EXPECT_STREQ(DndStateToString(DndState::FocusActive), "FocusActive");
  EXPECT_STREQ(DndStateToString(DndState::AcceptsNotifications), "AcceptsNotifications");
  EXPECT_STREQ(DndStateToString(DndState::QuietTime), "QuietTime");
  EXPECT_STREQ(DndStateToString(DndState::WindowsStoreApp), "WindowsStoreApp");
}

// ─────────────────────────────────────────────────────────────────────────────
// Mock DND Detector Tests
// ─────────────────────────────────────────────────────────────────────────────

class MockDndDetectorTest : public ::testing::Test
{
protected:
  MockDndDetector detector_;
};

TEST_F(MockDndDetectorTest, InitialState)
{
  EXPECT_FALSE(detector_.IsRunning());
  EXPECT_EQ(detector_.GetState(), DndState::AcceptsNotifications);
  EXPECT_FALSE(detector_.IsDndActive());
  EXPECT_EQ(detector_.GetPollingInterval(), 1000ms);
}

TEST_F(MockDndDetectorTest, StartAndStop)
{
  EXPECT_FALSE(detector_.IsRunning());

  detector_.Start();
  EXPECT_TRUE(detector_.IsRunning());

  detector_.Stop();
  EXPECT_FALSE(detector_.IsRunning());
}

TEST_F(MockDndDetectorTest, SetAndGetState)
{
  EXPECT_EQ(detector_.GetState(), DndState::AcceptsNotifications);

  detector_.SetState(DndState::Busy);
  EXPECT_EQ(detector_.GetState(), DndState::Busy);

  detector_.SetState(DndState::PresentationMode);
  EXPECT_EQ(detector_.GetState(), DndState::PresentationMode);
}

TEST_F(MockDndDetectorTest, IsDndActiveForEachState)
{
  // AcceptsNotifications is the only state where DND is NOT active
  detector_.SetState(DndState::AcceptsNotifications);
  EXPECT_FALSE(detector_.IsDndActive());

  // All other states should report DND as active
  detector_.SetState(DndState::NotPresent);
  EXPECT_TRUE(detector_.IsDndActive());

  detector_.SetState(DndState::Busy);
  EXPECT_TRUE(detector_.IsDndActive());

  detector_.SetState(DndState::FullScreenD3D);
  EXPECT_TRUE(detector_.IsDndActive());

  detector_.SetState(DndState::PresentationMode);
  EXPECT_TRUE(detector_.IsDndActive());

  detector_.SetState(DndState::FocusActive);
  EXPECT_TRUE(detector_.IsDndActive());

  detector_.SetState(DndState::QuietTime);
  EXPECT_TRUE(detector_.IsDndActive());

  detector_.SetState(DndState::WindowsStoreApp);
  EXPECT_TRUE(detector_.IsDndActive());
}

TEST_F(MockDndDetectorTest, CallbackTriggeredOnChange)
{
  std::atomic<int> callback_count{0};
  std::atomic<bool> last_value{false};

  detector_.SetOnDndChange(
      [&](bool is_active)
      {
        callback_count++;
        last_value = is_active;
      });

  // Start from AcceptsNotifications (DND inactive)
  EXPECT_EQ(callback_count, 0);

  // Change to Busy (DND active) - should trigger callback
  detector_.SetState(DndState::Busy);
  EXPECT_EQ(callback_count, 1);
  EXPECT_TRUE(last_value);

  // Change back to AcceptsNotifications - should trigger callback
  detector_.SetState(DndState::AcceptsNotifications);
  EXPECT_EQ(callback_count, 2);
  EXPECT_FALSE(last_value);
}

TEST_F(MockDndDetectorTest, CallbackNotTriggeredOnSameState)
{
  std::atomic<int> callback_count{0};

  detector_.SetOnDndChange([&](bool) { callback_count++; });

  // Start from AcceptsNotifications
  detector_.SetState(DndState::AcceptsNotifications);
  EXPECT_EQ(callback_count, 0);  // No change in DND active state

  // Change to Busy
  detector_.SetState(DndState::Busy);
  EXPECT_EQ(callback_count, 1);

  // Change to PresentationMode (both are DND active)
  detector_.SetState(DndState::PresentationMode);
  EXPECT_EQ(callback_count, 1);  // No change in DND active state

  // Change to FullScreenD3D (still DND active)
  detector_.SetState(DndState::FullScreenD3D);
  EXPECT_EQ(callback_count, 1);  // No change in DND active state
}

TEST_F(MockDndDetectorTest, PollingIntervalCanBeSet)
{
  EXPECT_EQ(detector_.GetPollingInterval(), 1000ms);

  detector_.SetPollingInterval(500ms);
  EXPECT_EQ(detector_.GetPollingInterval(), 500ms);

  detector_.SetPollingInterval(2000ms);
  EXPECT_EQ(detector_.GetPollingInterval(), 2000ms);
}

TEST_F(MockDndDetectorTest, NullCallbackHandled)
{
  // Set a null callback - should not crash
  detector_.SetOnDndChange(nullptr);

  // State changes should not crash even with null callback
  EXPECT_NO_THROW(detector_.SetState(DndState::Busy));
  EXPECT_NO_THROW(detector_.SetState(DndState::AcceptsNotifications));
}

TEST_F(MockDndDetectorTest, IsFullScreenDetectedForEachState)
{
  // These states should report full-screen detected
  detector_.SetState(DndState::Busy);
  EXPECT_TRUE(detector_.IsFullScreenDetected());

  detector_.SetState(DndState::FullScreenD3D);
  EXPECT_TRUE(detector_.IsFullScreenDetected());

  detector_.SetState(DndState::WindowsStoreApp);
  EXPECT_TRUE(detector_.IsFullScreenDetected());

  // These states should NOT report full-screen detected
  detector_.SetState(DndState::AcceptsNotifications);
  EXPECT_FALSE(detector_.IsFullScreenDetected());

  detector_.SetState(DndState::NotPresent);
  EXPECT_FALSE(detector_.IsFullScreenDetected());

  detector_.SetState(DndState::PresentationMode);
  EXPECT_FALSE(detector_.IsFullScreenDetected());

  detector_.SetState(DndState::FocusActive);
  EXPECT_FALSE(detector_.IsFullScreenDetected());

  detector_.SetState(DndState::QuietTime);
  EXPECT_FALSE(detector_.IsFullScreenDetected());
}

// ─────────────────────────────────────────────────────────────────────────────
// DndDetectorWin Tests (Windows-only)
// ─────────────────────────────────────────────────────────────────────────────

#ifdef _WIN32

class DndDetectorWinTest : public ::testing::Test
{
protected:
  void SetUp() override { detector_ = CreateDndDetector(); }

  void TearDown() override
  {
    if (detector_ && detector_->IsRunning())
    {
      detector_->Stop();
    }
  }

  std::unique_ptr<IDndDetector> detector_;
};

TEST_F(DndDetectorWinTest, CreateDndDetector)
{
  auto detector = CreateDndDetector();
  ASSERT_NE(detector, nullptr);
}

TEST_F(DndDetectorWinTest, InitialState)
{
  ASSERT_NE(detector_, nullptr);
  EXPECT_FALSE(detector_->IsRunning());
  // State could be any valid value depending on system state
  auto state = detector_->GetState();
  EXPECT_TRUE(state == DndState::NotPresent || state == DndState::Busy ||
              state == DndState::FullScreenD3D || state == DndState::PresentationMode ||
              state == DndState::FocusActive || state == DndState::AcceptsNotifications ||
              state == DndState::QuietTime || state == DndState::WindowsStoreApp);
}

TEST_F(DndDetectorWinTest, StartAndStop)
{
  ASSERT_NE(detector_, nullptr);

  EXPECT_FALSE(detector_->IsRunning());

  detector_->Start();
  EXPECT_TRUE(detector_->IsRunning());

  detector_->Stop();
  EXPECT_FALSE(detector_->IsRunning());
}

TEST_F(DndDetectorWinTest, StartTwiceIsNoOp)
{
  ASSERT_NE(detector_, nullptr);

  detector_->Start();
  EXPECT_TRUE(detector_->IsRunning());

  // Starting again should not crash
  EXPECT_NO_THROW(detector_->Start());
  EXPECT_TRUE(detector_->IsRunning());

  detector_->Stop();
}

TEST_F(DndDetectorWinTest, StopWhenNotRunningIsNoOp)
{
  ASSERT_NE(detector_, nullptr);

  EXPECT_FALSE(detector_->IsRunning());

  // Stopping when not running should not crash
  EXPECT_NO_THROW(detector_->Stop());
  EXPECT_FALSE(detector_->IsRunning());
}

TEST_F(DndDetectorWinTest, GetStateReturnsValidEnum)
{
  ASSERT_NE(detector_, nullptr);

  auto state = detector_->GetState();

  // State should be a valid enum value
  EXPECT_TRUE(state == DndState::NotPresent || state == DndState::Busy ||
              state == DndState::FullScreenD3D || state == DndState::PresentationMode ||
              state == DndState::FocusActive || state == DndState::AcceptsNotifications ||
              state == DndState::QuietTime || state == DndState::WindowsStoreApp);
}

TEST_F(DndDetectorWinTest, IsDndActiveMatchesState)
{
  ASSERT_NE(detector_, nullptr);

  auto state = detector_->GetState();
  bool is_active = detector_->IsDndActive();

  // IsDndActive should return true for all states except AcceptsNotifications
  if (state == DndState::AcceptsNotifications)
  {
    EXPECT_FALSE(is_active);
  }
  else
  {
    EXPECT_TRUE(is_active);
  }
}

TEST_F(DndDetectorWinTest, IsFullScreenDetectedMatchesState)
{
  ASSERT_NE(detector_, nullptr);

  auto state = detector_->GetState();
  bool is_fullscreen = detector_->IsFullScreenDetected();

  // IsFullScreenDetected should return true for Busy, FullScreenD3D, WindowsStoreApp
  if (state == DndState::Busy || state == DndState::FullScreenD3D ||
      state == DndState::WindowsStoreApp)
  {
    EXPECT_TRUE(is_fullscreen);
  }
  else
  {
    EXPECT_FALSE(is_fullscreen);
  }
}

TEST_F(DndDetectorWinTest, CallbackCanBeSet)
{
  ASSERT_NE(detector_, nullptr);

  // Setting a callback should not crash
  EXPECT_NO_THROW(detector_->SetOnDndChange([](bool) {}));

  // Setting null callback should not crash
  EXPECT_NO_THROW(detector_->SetOnDndChange(nullptr));
}

TEST_F(DndDetectorWinTest, PollingIntervalCanBeSetAndGet)
{
  ASSERT_NE(detector_, nullptr);

  detector_->SetPollingInterval(500ms);
  EXPECT_EQ(detector_->GetPollingInterval(), 500ms);

  detector_->SetPollingInterval(2000ms);
  EXPECT_EQ(detector_->GetPollingInterval(), 2000ms);
}

TEST_F(DndDetectorWinTest, MonitorThreadStopsCleanly)
{
  ASSERT_NE(detector_, nullptr);

  detector_->SetPollingInterval(100ms);
  detector_->Start();
  EXPECT_TRUE(detector_->IsRunning());

  // Let it run for a bit
  std::this_thread::sleep_for(250ms);

  // Stop should complete without hanging
  auto start = std::chrono::steady_clock::now();
  detector_->Stop();
  auto elapsed = std::chrono::steady_clock::now() - start;

  EXPECT_FALSE(detector_->IsRunning());
  // Stop should complete within reasonable time (< 1 second)
  EXPECT_LT(elapsed, 1s);
}

TEST_F(DndDetectorWinTest, DefaultPollingInterval)
{
  ASSERT_NE(detector_, nullptr);

  // Default polling interval should be 1 second
  EXPECT_EQ(detector_->GetPollingInterval(), 1000ms);
}

#endif  // _WIN32

}  // namespace
}  // namespace platform
}  // namespace blinkbreak
