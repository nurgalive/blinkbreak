/// @file idle_detector_win.hpp
/// @brief Windows-specific idle detection implementation.

#ifndef BLINKBREAK_PLATFORM_WINDOWS_IDLE_DETECTOR_WIN_HPP
#define BLINKBREAK_PLATFORM_WINDOWS_IDLE_DETECTOR_WIN_HPP

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>

#include "platform/platform_interface.hpp"

namespace blinkbreak
{
namespace platform
{

/// @brief Windows implementation of idle detection using GetLastInputInfo.
///
/// This implementation polls the system at regular intervals to check the
/// time since last user input. When the idle time exceeds the configured
/// threshold, the on_idle callback is invoked. When user activity resumes,
/// the on_active callback is invoked.
class IdleDetectorWin : public IIdleDetector
{
public:
  /// @brief Constructs the idle detector.
  IdleDetectorWin();

  /// @brief Destructor - stops monitoring thread.
  ~IdleDetectorWin() override;

  // Non-copyable, non-movable
  IdleDetectorWin(const IdleDetectorWin&) = delete;
  IdleDetectorWin& operator=(const IdleDetectorWin&) = delete;
  IdleDetectorWin(IdleDetectorWin&&) = delete;
  IdleDetectorWin& operator=(IdleDetectorWin&&) = delete;

  /// @brief Starts monitoring for idle state.
  void Start() override;

  /// @brief Stops monitoring.
  void Stop() override;

  /// @brief Checks if the detector is currently monitoring.
  /// @return True if monitoring is active.
  [[nodiscard]] bool IsRunning() const override;

  /// @brief Gets current idle time in milliseconds.
  /// @return The duration since last user input.
  [[nodiscard]] std::chrono::milliseconds GetIdleTime() const override;

  /// @brief Checks if the user is currently considered idle.
  /// @return True if idle time exceeds the configured threshold.
  [[nodiscard]] bool IsIdle() const override;

  /// @brief Sets the idle threshold duration.
  /// @param threshold Duration of inactivity before user is considered idle.
  void SetIdleThreshold(std::chrono::seconds threshold) override;

  /// @brief Gets the current idle threshold.
  /// @return The configured idle threshold.
  [[nodiscard]] std::chrono::seconds GetIdleThreshold() const override;

  /// @brief Sets callback for when user becomes idle.
  /// @param callback Function called when user transitions to idle state.
  void SetOnIdle(std::function<void()> callback) override;

  /// @brief Sets callback for when user becomes active.
  /// @param callback Function called when user transitions to active state.
  void SetOnActive(std::function<void()> callback) override;

private:
  /// @brief Monitoring thread function.
  void MonitorThreadFunc();

  std::thread monitor_thread_;       ///< Background monitoring thread.
  std::atomic<bool> running_;        ///< Whether monitoring is active.
  std::atomic<bool> is_idle_;        ///< Current idle state.
  mutable std::mutex mutex_;         ///< Protects callbacks and threshold.
  std::chrono::seconds threshold_;   ///< Idle threshold duration.
  std::function<void()> on_idle_;    ///< Callback when user becomes idle.
  std::function<void()> on_active_;  ///< Callback when user becomes active.
};

}  // namespace platform
}  // namespace blinkbreak

#endif  // BLINKBREAK_PLATFORM_WINDOWS_IDLE_DETECTOR_WIN_HPP
