/// @file app_controller.hpp
/// @brief Main application controller connecting UI and core logic.

#ifndef BLINKBREAK_UI_APP_CONTROLLER_HPP
#define BLINKBREAK_UI_APP_CONTROLLER_HPP

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "core/break_scheduler.hpp"
#include "core/config_manager.hpp"
#include "core/state_machine.hpp"
#include "overlay_manager.hpp"
#include "tray_manager.hpp"

// Forward declaration of Slint generated types
namespace slint {
template <typename T>
class ComponentHandle;

template <typename T>
class ComponentWeakHandle;
}  // namespace slint

class MainWindow;
class SettingsDialog;

namespace blinkbreak {

namespace platform {
class IMonitorManager;
class IIdleDetector;
}  // namespace platform

/// @brief Main application controller.
///
/// The AppController orchestrates the interaction between the UI,
/// state machine, break scheduler, and configuration. It manages
/// the timer thread and ensures thread-safe updates to the UI.
class AppController {
public:
    /// @brief Constructs the application controller.
    AppController();

    /// @brief Destructor - stops all threads.
    ~AppController();

    // Non-copyable, non-movable
    AppController(const AppController&) = delete;
    AppController& operator=(const AppController&) = delete;
    AppController(AppController&&) = delete;
    AppController& operator=(AppController&&) = delete;

    /// @brief Initializes the controller with configuration.
    /// @return True if initialization succeeded.
    bool Initialize();

    /// @brief Runs the main application loop.
    /// @return Exit code.
    int Run();

    /// @brief Handles the start/resume action.
    void OnStart();

    /// @brief Handles the pause action.
    void OnPause();

    /// @brief Handles the skip action.
    void OnSkip();

    /// @brief Handles the snooze action.
    void OnSnooze();

    /// @brief Handles the reset action.
    void OnReset();

    /// @brief Opens the settings dialog.
    void OnOpenSettings();

    /// @brief Handles quit action from tray menu.
    void OnQuit();

    /// @brief Gets formatted time remaining string.
    /// @return Time in "MM:SS" format.
    [[nodiscard]] std::string GetTimeRemainingString() const;

    /// @brief Gets formatted time remaining until the next short break.
    /// @return Time in "MM:SS" format.
    [[nodiscard]] std::string GetTimeUntilShortBreakString() const;

    /// @brief Gets formatted time remaining until the next long break.
    /// @return Time in "MM:SS" format.
    [[nodiscard]] std::string GetTimeUntilLongBreakString() const;

    /// @brief Gets formatted tracked time string.
    /// @return Time in "MM:SS" format.
    [[nodiscard]] std::string GetTrackedTimeString() const;

    /// @brief Gets the current short-break progress (0.0 - 1.0).
    /// @return Progress value.
    [[nodiscard]] float GetShortProgress() const;

    /// @brief Gets the current long-break progress (0.0 - 1.0).
    /// @return Progress value.
    [[nodiscard]] float GetLongProgress() const;

    /// @brief Gets the count of completed short breaks.
    /// @return Count of short breaks.
    [[nodiscard]] int GetShortBreakCount() const;

    /// @brief Gets the count of completed long breaks.
    /// @return Count of long breaks.
    [[nodiscard]] int GetLongBreakCount() const;

    /// @brief Gets the count of skipped short breaks.
    /// @return Count of skipped short breaks.
    [[nodiscard]] int GetShortSkippedCount() const;

    /// @brief Gets the count of skipped long breaks.
    /// @return Count of skipped long breaks.
    [[nodiscard]] int GetLongSkippedCount() const;

    /// @brief Gets the current status text.
    /// @return Status string.
    [[nodiscard]] std::string GetStatusText() const;

private:
    /// @brief Timer thread function.
    void TimerThreadFunc();

    /// @brief Updates UI bindings (called from timer thread).
    void UpdateUI();

    /// @brief Handles user idle event from idle detector.
    void OnUserIdle();

    /// @brief Handles user active event from idle detector.
    void OnUserActive();

    /// @brief Formats duration as MM:SS string.
    /// @param duration The duration to format.
    /// @return Formatted string.
    [[nodiscard]] static std::string FormatDuration(Duration duration);

    std::unique_ptr<ConfigManager> config_manager_;
    std::unique_ptr<StateMachine> state_machine_;
    std::unique_ptr<BreakScheduler> scheduler_;
    std::unique_ptr<TrayManager> tray_manager_;
    std::unique_ptr<OverlayManager> overlay_manager_;
    std::shared_ptr<platform::IMonitorManager> monitor_manager_;
    std::unique_ptr<platform::IIdleDetector> idle_detector_;
    AppConfig config_;
    std::filesystem::path config_path_;

    std::unique_ptr<slint::ComponentHandle<MainWindow>> main_window_;
    std::unique_ptr<slint::ComponentWeakHandle<MainWindow>> main_window_weak_;
    std::unique_ptr<slint::ComponentHandle<SettingsDialog>> settings_dialog_;

    std::thread timer_thread_;
    std::atomic<bool> running_;
    mutable std::mutex mutex_;
    mutable std::mutex scheduler_mutex_;

    // Cached UI state
    std::string time_until_short_;
    std::string time_until_long_;
    std::string tracked_time_;
    Duration tracked_duration_;
    DurationMs tracked_duration_ms_;
    float short_progress_;
    float long_progress_;
    int short_break_count_;
    int long_break_count_;
    int short_skipped_count_;
    int long_skipped_count_;
    bool skip_in_progress_;
    std::string status_text_;
    bool is_running_;
    bool is_paused_by_idle_;
};

}  // namespace blinkbreak

#endif  // BLINKBREAK_UI_APP_CONTROLLER_HPP