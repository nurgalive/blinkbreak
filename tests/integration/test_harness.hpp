/// @file test_harness.hpp
/// @brief Integration test harness for end-to-end workflow testing.

#ifndef BLINKBREAK_TESTS_TEST_HARNESS_HPP
#define BLINKBREAK_TESTS_TEST_HARNESS_HPP

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <blinkbreak/events.hpp>
#include <blinkbreak/types.hpp>

#include "core/break_scheduler.hpp"
#include "core/config_types.hpp"
#include "core/state_machine.hpp"
#include "mock_platform.hpp"

namespace blinkbreak::testing
{

/// @brief Statistics collected during test execution.
struct TestStats
{
  int short_breaks_triggered = 0;      ///< Number of short breaks triggered.
  int long_breaks_triggered = 0;       ///< Number of long breaks triggered.
  int breaks_skipped = 0;              ///< Number of breaks skipped.
  int breaks_snoozed = 0;              ///< Number of breaks snoozed.
  int notifications_shown = 0;         ///< Number of notifications shown.
  int idle_pauses = 0;                 ///< Number of times paused due to idle.
  int dnd_suppressions = 0;            ///< Number of breaks suppressed due to DND.
  DurationMs total_break_time{0};      ///< Total time spent in breaks.
  DurationMs total_simulated_time{0};  ///< Total time simulated.
};

/// @brief Integration test harness for end-to-end workflow testing.
///
/// This harness provides a controlled environment for testing the complete
/// BlinkBreak workflow without requiring real time delays or user interaction.
/// It uses mock platform components and a virtual clock for time simulation.
///
/// @code
/// TestHarness harness;
/// harness.InitializeWithDefaults();
/// harness.AdvanceTime(std::chrono::minutes(20));
/// EXPECT_EQ(harness.GetCurrentState(), State::kBreakActive);
/// @endcode
class TestHarness
{
public:
  TestHarness();
  ~TestHarness();

  // Non-copyable, non-movable
  TestHarness(const TestHarness&) = delete;
  TestHarness& operator=(const TestHarness&) = delete;
  TestHarness(TestHarness&&) = delete;
  TestHarness& operator=(TestHarness&&) = delete;

  /// @brief Initialize the harness with a configuration.
  /// @param config The application configuration.
  void Initialize(const AppConfig& config);

  /// @brief Initialize with default test configuration.
  ///
  /// Defaults: 20-minute short breaks (20s duration),
  ///           60-minute long breaks (5-minute duration),
  ///           notifications enabled with 30s warning.
  void InitializeWithDefaults();

  // --- Time Simulation ---

  /// @brief Advance simulated time and process all timers.
  /// @param duration Amount of time to advance.
  /// @param step_size Granularity of time steps (default 100ms).
  void AdvanceTime(DurationMs duration, DurationMs step_size = DurationMs(100));

  /// @brief Fast-forward to the next scheduled event.
  /// @return The time advanced.
  DurationMs AdvanceToNextEvent();

  /// @brief Get the current simulated time.
  /// @return Simulated time since start.
  [[nodiscard]] DurationMs GetSimulatedTime() const { return simulated_time_; }

  // --- User Simulation ---

  /// @brief Simulate user becoming idle.
  /// @param idle_duration Duration user has been idle.
  void SimulateIdle(DurationMs idle_duration);

  /// @brief Simulate user activity (ends idle state).
  void SimulateActivity();

  /// @brief Simulate user skipping a break.
  void SimulateSkip();

  /// @brief Simulate user snoozing a break.
  void SimulateSnooze();

  /// @brief Simulate user clicking notification body.
  void SimulateNotificationClick();

  /// @brief Simulate notification action button press.
  /// @param action The action triggered.
  void SimulateNotificationAction(platform::NotificationAction action);

  // --- System Simulation ---

  /// @brief Enable/disable DND mode.
  /// @param active Whether DND should be active.
  void SetDndActive(bool active);

  /// @brief Set DND state to a specific value.
  /// @param state The DND state.
  void SetDndState(platform::DndState state);

  /// @brief Enable/disable fullscreen app detection.
  /// @param active Whether a fullscreen app is detected.
  void SetFullScreenActive(bool active);

  /// @brief Simulate monitor configuration change.
  /// @param monitors The new monitor configuration.
  void SimulateMonitorChange(std::vector<platform::MonitorInfo> monitors);

  // --- State Queries ---

  /// @brief Get the current application state.
  /// @return The current state.
  [[nodiscard]] State GetCurrentState() const;

  /// @brief Check if a break is currently active.
  /// @return True if a break is active.
  [[nodiscard]] bool IsBreakActive() const;

  /// @brief Check if the scheduler is paused.
  /// @return True if paused.
  [[nodiscard]] bool IsPaused() const;

  /// @brief Check if paused due to idle detection.
  /// @return True if paused by idle.
  [[nodiscard]] bool IsPausedByIdle() const { return is_paused_by_idle_; }

  /// @brief Get remaining time until next break.
  /// @return Time until next break.
  [[nodiscard]] std::optional<Duration> GetTimeUntilBreak() const;

  /// @brief Get the type of the next scheduled break.
  /// @return The break type.
  [[nodiscard]] BreakType GetNextBreakType() const;

  /// @brief Get test statistics.
  /// @return Statistics collected during test.
  [[nodiscard]] const TestStats& GetStats() const { return stats_; }

  /// @brief Reset statistics to zero.
  void ResetStats() { stats_ = TestStats{}; }

  // --- Configuration ---

  /// @brief Update configuration during test.
  /// @param config The new configuration.
  void UpdateConfig(const AppConfig& config);

  /// @brief Get current configuration.
  /// @return The current configuration.
  [[nodiscard]] const AppConfig& GetConfig() const { return current_config_; }

  // --- Component Access (for advanced testing) ---

  /// @brief Get the mock idle detector.
  [[nodiscard]] MockIdleDetector& GetIdleDetector() { return *idle_detector_; }

  /// @brief Get the mock DND detector.
  [[nodiscard]] MockDndDetector& GetDndDetector() { return *dnd_detector_; }

  /// @brief Get the mock notification manager.
  [[nodiscard]] MockNotificationManager& GetNotificationManager() { return *notification_manager_; }

  /// @brief Get the mock monitor manager.
  [[nodiscard]] MockMonitorManager& GetMonitorManager() { return *monitor_manager_; }

  /// @brief Get the state machine.
  [[nodiscard]] StateMachine& GetStateMachine() { return *state_machine_; }

  /// @brief Get the break scheduler.
  [[nodiscard]] BreakScheduler& GetScheduler() { return *scheduler_; }

private:
  void SetupCallbacks();
  void ProcessTimerTick(DurationMs elapsed);
  void OnBreakStart(const BreakInfo& info);
  void OnBreakEnd(const BreakInfo& info);
  void OnWarning(BreakType type, Duration time_until);
  void CheckIdleState();
  void CheckDndState();

  DurationMs simulated_time_{0};
  TestStats stats_;
  AppConfig current_config_;

  std::unique_ptr<MockIdleDetector> idle_detector_;
  std::unique_ptr<MockDndDetector> dnd_detector_;
  std::unique_ptr<MockNotificationManager> notification_manager_;
  std::unique_ptr<MockMonitorManager> monitor_manager_;
  std::unique_ptr<StateMachine> state_machine_;
  std::unique_ptr<BreakScheduler> scheduler_;

  bool is_paused_by_idle_ = false;
  bool reset_short_on_idle_triggered_ = false;
  bool reset_long_on_idle_triggered_ = false;
  bool break_in_progress_ = false;
  BreakInfo current_break_info_;
};

}  // namespace blinkbreak::testing

#endif  // BLINKBREAK_TESTS_TEST_HARNESS_HPP
