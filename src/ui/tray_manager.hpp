/// @file tray_manager.hpp
/// @brief System tray management.

#ifndef BLINKBREAK_UI_TRAY_MANAGER_HPP
#define BLINKBREAK_UI_TRAY_MANAGER_HPP

#include <blinkbreak/types.hpp>

#include <functional>
#include <memory>

#include "platform/platform_interface.hpp"

namespace blinkbreak {

/// @brief Manages system tray icon and menu.
class TrayManager {
public:
    /// @brief Callbacks for tray actions.
    struct Callbacks {
        std::function<void()> on_show_window;
        std::function<void()> on_start_pause;
        std::function<void()> on_settings;
        std::function<void()> on_quit;
    };

    /// @brief Constructs a tray manager.
    /// @param callbacks The action callbacks.
    explicit TrayManager(Callbacks callbacks);

    /// @brief Destructor.
    ~TrayManager();

    /// @brief Shows the tray icon.
    /// @return True if successful.
    bool Show();

    /// @brief Sets the native host window used by the platform tray backend.
    /// @param native_window_handle Platform-specific window handle.
    void SetHostWindow(std::uintptr_t native_window_handle);

    /// @brief Hides the tray icon.
    void Hide();

    /// @brief Updates the tooltip with current status.
    /// @param is_running Whether the timer is running.
    /// @param time_until_break Time until next break.
    /// @param break_type Type of next break.
    void UpdateStatus(bool is_running, Duration time_until_break, BreakType break_type);

    /// @brief Updates the menu based on current state.
    /// @param is_running Whether the timer is running.
    void UpdateMenu(bool is_running);

private:
    std::unique_ptr<platform::ITrayIcon> tray_icon_;
    Callbacks callbacks_;
    bool is_running_;
};

}  // namespace blinkbreak

#endif  // BLINKBREAK_UI_TRAY_MANAGER_HPP
