/// @file app_controller.hpp
/// @brief Main application controller connecting UI and core logic.

#ifndef BLINKBREAK_UI_APP_CONTROLLER_HPP
#define BLINKBREAK_UI_APP_CONTROLLER_HPP

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <slint.h>
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
class INotificationManager;
class IDndDetector;
enum class NotificationAction;
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
    /// @brief Gets the current state snapshot.
    /// @return Current state.
    [[nodiscard]] State GetCurrentStateSnapshot() const;

    /// @brief Timer thread function.
    void TimerThreadFunc();

    /// @brief Updates UI bindings (called from timer thread).
    void UpdateUI();

    /// @brief Shows the main window from the Slint event loop.
    void ShowMainWindow();

    /// @brief Handles user idle event from idle detector.
    void OnUserIdle();

    /// @brief Handles user active event from idle detector.
    void OnUserActive();

    /// @brief Handles user actions from notifications.
    void OnNotificationAction(platform::NotificationAction action);

    /// @brief Shows a pre-break toast notification.
    void ShowPreBreakNotification(BreakType type, Duration time_until);

    /// @brief Ensures the notification backend exists and is initialized.
    /// @return True when notifications can be shown.
    [[nodiscard]] bool EnsureNotificationManagerInitialized();

    /// @brief Starts or stops live DND monitoring based on current needs.
    /// @param should_enable Whether DND polling is required.
    void EnsureDndDetectorState(bool should_enable);

    /// @brief Applies the configured theme to the main window.
    void ApplyThemeToMainWindow();

    /// @brief Applies the configured theme to the settings dialog.
    void ApplyThemeToSettingsDialog();

    /// @brief Resolves the current live DND state and suppression policy.
    /// @param respect_dnd Whether actual DND/Focus states should be respected.
    /// @param respect_fullscreen Whether fullscreen/presentation states should be respected.
    /// @return Latest DND state plus whether BlinkBreak should suppress UI.
    [[nodiscard]] std::pair<platform::DndState, bool> EvaluateDndSuppression(
        bool respect_dnd, bool respect_fullscreen);



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
    std::unique_ptr<platform::INotificationManager> notification_manager_;
    std::unique_ptr<platform::IDndDetector> dnd_detector_;
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
    int short_idle_skipped_count_;
    int long_idle_skipped_count_;
    bool skip_in_progress_;
    std::optional<BreakType> pending_notification_break_;
    std::optional<platform::NotificationAction> pending_notification_action_;
    int64_t active_toast_id_;
    std::string status_text_;
    State current_state_;
    bool is_running_;
    bool is_paused_by_idle_;
    bool show_idle_timer_;
    std::string idle_time_;
    bool reset_short_on_idle_triggered_;  ///< Tracks if short reset on idle was triggered.
    bool reset_long_on_idle_triggered_;   ///< Tracks if long reset on idle was triggered.
};

}  // namespace blinkbreak

#endif  // BLINKBREAK_UI_APP_CONTROLLER_HPP
