/// @file app_controller.hpp
/// @brief Main application controller connecting UI and core logic.

#ifndef BLINKBREAK_UI_APP_CONTROLLER_HPP
#define BLINKBREAK_UI_APP_CONTROLLER_HPP

#include "core/break_scheduler.hpp"
#include "core/config_manager.hpp"
#include "core/state_machine.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// Forward declaration of Slint generated types
namespace slint {
template <typename T>
class ComponentHandle;
}

class MainWindow;
class SettingsDialog;

namespace blinkbreak {

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

    /// @brief Gets formatted time remaining string.
    /// @return Time in "MM:SS" format.
    [[nodiscard]] std::string GetTimeRemainingString() const;

    /// @brief Gets the current progress (0.0 - 1.0).
    /// @return Progress value.
    [[nodiscard]] float GetProgress() const;

    /// @brief Gets the current status text.
    /// @return Status string.
    [[nodiscard]] std::string GetStatusText() const;

private:
    /// @brief Timer thread function.
    void TimerThreadFunc();

    /// @brief Updates UI bindings (called from timer thread).
    void UpdateUI();

    /// @brief Formats duration as MM:SS string.
    /// @param duration The duration to format.
    /// @return Formatted string.
    [[nodiscard]] static std::string FormatDuration(Duration duration);

    std::unique_ptr<ConfigManager> config_manager_;
    std::unique_ptr<StateMachine> state_machine_;
    std::unique_ptr<BreakScheduler> scheduler_;
    AppConfig config_;

    std::unique_ptr<slint::ComponentHandle<MainWindow>> main_window_;
    std::unique_ptr<slint::ComponentHandle<SettingsDialog>> settings_dialog_;

    std::thread timer_thread_;
    std::atomic<bool> running_;
    mutable std::mutex mutex_;

    // Cached UI state
    std::string time_remaining_;
    float progress_;
    std::string status_text_;
    bool is_running_;
};

}  // namespace blinkbreak

#endif  // BLINKBREAK_UI_APP_CONTROLLER_HPP