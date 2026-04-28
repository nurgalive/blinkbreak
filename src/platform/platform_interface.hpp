/// @file platform_interface.hpp
/// @brief Platform abstraction interfaces.

#ifndef BLINKBREAK_PLATFORM_INTERFACE_HPP
#define BLINKBREAK_PLATFORM_INTERFACE_HPP

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace blinkbreak {
namespace platform {

/// @brief Menu item for system tray context menu.
struct MenuItem {
    std::string text;                ///< Display text.
    std::function<void()> callback;  ///< Action callback.
    bool enabled = true;             ///< Whether item is enabled.
    bool checked = false;            ///< Whether item is checked.
    bool is_separator = false;       ///< Whether this is a separator.
};

/// @brief Interface for system tray functionality.
class ITrayIcon {
public:
    virtual ~ITrayIcon() = default;

    /// @brief Shows the tray icon.
    /// @return True if successful.
    virtual bool Show() = 0;

    /// @brief Hides the tray icon.
    virtual void Hide() = 0;

    /// @brief Sets the tooltip text.
    /// @param tooltip The tooltip text.
    virtual void SetTooltip(const std::string& tooltip) = 0;

    /// @brief Sets the context menu items.
    /// @param items The menu items.
    virtual void SetMenu(const std::vector<MenuItem>& items) = 0;

    /// @brief Sets the callback for left-click.
    /// @param callback The callback function.
    virtual void SetOnClick(std::function<void()> callback) = 0;

    /// @brief Sets the callback for double-click.
    /// @param callback The callback function.
    virtual void SetOnDoubleClick(std::function<void()> callback) = 0;

    /// @brief Sets the native host window used for tray callbacks.
    /// @param native_window_handle Platform window handle, or 0 if unavailable.
    virtual void SetHostWindow(std::uintptr_t native_window_handle) = 0;

    /// @brief Updates the icon (e.g., to show different states).
    /// @param icon_id Identifier for the icon state.
    virtual void SetIcon(int icon_id) = 0;
};

/// @brief Creates platform-specific tray icon implementation.
/// @return Unique pointer to the tray icon implementation.
std::unique_ptr<ITrayIcon> CreateTrayIcon();

// ─────────────────────────────────────────────────────────────────────────────
// Monitor Management
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Display orientation.
enum class MonitorOrientation {
    kLandscape,         ///< Normal landscape (0°).
    kPortrait,          ///< Portrait (90°).
    kLandscapeFlipped,  ///< Landscape flipped (180°).
    kPortraitFlipped,   ///< Portrait flipped (270°).
};

/// @brief Converts a MonitorOrientation to a human-readable string.
[[nodiscard]] constexpr const char* OrientationToString(MonitorOrientation o) {
    switch (o) {
        case MonitorOrientation::kLandscape:
            return "Landscape";
        case MonitorOrientation::kPortrait:
            return "Portrait";
        case MonitorOrientation::kLandscapeFlipped:
            return "Landscape (flipped)";
        case MonitorOrientation::kPortraitFlipped:
            return "Portrait (flipped)";
    }
    return "Unknown";
}

/// @brief Information about a display monitor.
struct MonitorInfo {
    int id = 0;               ///< Monitor identifier (index).
    std::string name;         ///< Display device name.
    int x = 0;                ///< Left edge in virtual-screen pixels.
    int y = 0;                ///< Top edge in virtual-screen pixels.
    int width = 0;            ///< Width in pixels.
    int height = 0;           ///< Height in pixels.
    bool is_primary = false;  ///< Whether this is the primary monitor.
    MonitorOrientation orientation = MonitorOrientation::kLandscape;
    unsigned int dpi = 96;  ///< Effective DPI.
};

/// @brief Interface for monitor management.
class IMonitorManager {
public:
    virtual ~IMonitorManager() = default;

    /// @brief Refreshes the internal monitor list from the OS.
    virtual void RefreshMonitors() = 0;

    /// @brief Gets all connected monitors.
    /// @return Vector of MonitorInfo structs.
    [[nodiscard]] virtual std::vector<MonitorInfo> GetMonitors() const = 0;

    /// @brief Gets the primary monitor.
    /// @return Primary monitor info.
    [[nodiscard]] virtual MonitorInfo GetPrimaryMonitor() const = 0;

    /// @brief Gets the number of connected monitors.
    /// @return Monitor count.
    [[nodiscard]] virtual int GetMonitorCount() const = 0;

    /// @brief Registers a callback invoked when the monitor configuration changes.
    /// @param callback The callback (called from any thread).
    virtual void SetOnMonitorChange(std::function<void()> callback) = 0;
};

/// @brief Creates platform-specific monitor manager implementation.
/// @return Unique pointer to the monitor manager implementation.
std::unique_ptr<IMonitorManager> CreateMonitorManager();

// ─────────────────────────────────────────────────────────────────────────────
// Idle Detection
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Interface for idle detection.
///
/// Monitors user activity and provides callbacks when the user becomes idle
/// or returns to active state. The idle threshold determines how long the user
/// must be inactive before being considered idle.
class IIdleDetector {
public:
    virtual ~IIdleDetector() = default;

    /// @brief Starts monitoring for idle state.
    virtual void Start() = 0;

    /// @brief Stops monitoring.
    virtual void Stop() = 0;

    /// @brief Checks if the detector is currently monitoring.
    /// @return True if monitoring is active.
    [[nodiscard]] virtual bool IsRunning() const = 0;

    /// @brief Gets current idle time in milliseconds.
    /// @return The duration since last user input.
    [[nodiscard]] virtual std::chrono::milliseconds GetIdleTime() const = 0;

    /// @brief Checks if the user is currently considered idle.
    /// @return True if idle time exceeds the configured threshold.
    [[nodiscard]] virtual bool IsIdle() const = 0;

    /// @brief Sets the idle threshold duration.
    /// @param threshold Duration of inactivity before user is considered idle.
    virtual void SetIdleThreshold(std::chrono::seconds threshold) = 0;

    /// @brief Gets the current idle threshold.
    /// @return The configured idle threshold.
    [[nodiscard]] virtual std::chrono::seconds GetIdleThreshold() const = 0;

    /// @brief Sets callback for when user becomes idle.
    /// @param callback Function called when user transitions to idle state.
    virtual void SetOnIdle(std::function<void()> callback) = 0;

    /// @brief Sets callback for when user becomes active.
    /// @param callback Function called when user transitions to active state.
    virtual void SetOnActive(std::function<void()> callback) = 0;
};

/// @brief Creates platform-specific idle detector implementation.
/// @return Unique pointer to the idle detector implementation.
std::unique_ptr<IIdleDetector> CreateIdleDetector();

// ─────────────────────────────────────────────────────────────────────────────
// Notifications
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Describes the user's response to a toast notification.
enum class NotificationAction {
    Clicked,     ///< User clicked the notification body.
    Dismissed,   ///< User dismissed or notification timed out.
    SkipBreak,   ///< User clicked the "Skip break" action button.
    SnoozeBreak  ///< User clicked the "Snooze break" action button.
};

/// @brief Interface for managing OS toast notifications.
class INotificationManager {
public:
    virtual ~INotificationManager() = default;

    /// @brief Initializes the notification backend.
    /// Must be called from the main (STA) thread after COM is initialized.
    /// @return True if initialization succeeded.
    virtual bool Initialize() = 0;

    /// @brief Shows a toast notification with action buttons.
    /// @param title The notification title (first line).
    /// @param message The notification body text (second line).
    /// @return A non-negative toast ID on success, or -1 on failure.
    virtual int64_t Show(const std::string& title, const std::string& message) = 0;

    /// @brief Hides (dismisses) a previously shown notification.
    /// @param toast_id The ID returned by Show().
    virtual void Hide(int64_t toast_id) = 0;

    /// @brief Sets the callback for notification action events.
    /// @param callback The callback function.
    virtual void SetOnAction(std::function<void(NotificationAction)> callback) = 0;

    /// @brief Checks whether the notification backend is available.
    /// @return True if the system supports toast notifications.
    [[nodiscard]] virtual bool IsSupported() const = 0;
};

/// @brief Factory function for creating the platform notification manager.
/// @return A unique_ptr to the platform-specific INotificationManager.
std::unique_ptr<INotificationManager> CreateNotificationManager();

// ─────────────────────────────────────────────────────────────────────────────
// Do Not Disturb Detection
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Describes the current Do Not Disturb state.
///
/// These values map to Windows QUERY_USER_NOTIFICATION_STATE values.
enum class DndState {
    NotPresent,            ///< Screen saver, locked, or Fast User Switching active.
    Busy,                  ///< Full-screen app or Presentation Settings enabled.
    FullScreenD3D,         ///< Full-screen exclusive D3D application running.
    PresentationMode,      ///< Windows Presentation Settings explicitly enabled.
    FocusActive,           ///< Windows 11 Focus session is currently active.
    DoNotDisturbActive,    ///< Windows 11 Do Not Disturb is active.
    AcceptsNotifications,  ///< Normal state — DND is OFF, notifications allowed.
    QuietTime,             ///< Quiet Time period (first hour after login).
    WindowsStoreApp        ///< Full-screen Windows Store app (Win8+).
};

/// @brief Converts DndState to a human-readable string.
/// @param state The DND state.
/// @return String representation (e.g., "Busy", "AcceptsNotifications").
[[nodiscard]] constexpr const char* DndStateToString(DndState state) {
    switch (state) {
        case DndState::NotPresent:
            return "NotPresent";
        case DndState::Busy:
            return "Busy";
        case DndState::FullScreenD3D:
            return "FullScreenD3D";
        case DndState::PresentationMode:
            return "PresentationMode";
        case DndState::FocusActive:
            return "FocusActive";
        case DndState::DoNotDisturbActive:
            return "DoNotDisturbActive";
        case DndState::AcceptsNotifications:
            return "AcceptsNotifications";
        case DndState::QuietTime:
            return "QuietTime";
        case DndState::WindowsStoreApp:
            return "WindowsStoreApp";
    }
    return "Unknown";
}

/// @brief Interface for Do Not Disturb detection.
///
/// Monitors the system's DND/Focus Assist state and provides callbacks when
/// the state changes. This allows the application to suppress break overlays
/// and notifications when the user is in a focus session, presentation mode,
/// or running a full-screen application.
class IDndDetector {
public:
    virtual ~IDndDetector() = default;

    /// @brief Starts monitoring for DND state changes.
    virtual void Start() = 0;

    /// @brief Stops monitoring.
    virtual void Stop() = 0;

    /// @brief Checks if the detector is currently monitoring.
    /// @return True if monitoring is active.
    [[nodiscard]] virtual bool IsRunning() const = 0;

    /// @brief Gets the current DND state.
    /// @return The current DndState enum value.
    [[nodiscard]] virtual DndState GetState() const = 0;

    /// @brief Forces an immediate state refresh.
    /// @return The latest DND state observed from the operating system.
    [[nodiscard]] virtual DndState RefreshState() = 0;

    /// @brief Checks if DND is currently active.
    ///
    /// DND is considered active for any state except AcceptsNotifications.
    /// @return True if notifications/overlays should be suppressed.
    [[nodiscard]] virtual bool IsDndActive() const = 0;

    /// @brief Checks if the current state is due to full-screen app detection.
    ///
    /// This is separate from actual DND/Focus modes. Full-screen detection
    /// (QUNS_BUSY) can trigger when maximized apps are running, which some
    /// users may not want to treat as DND.
    /// @return True if the state is Busy (full-screen app detected).
    [[nodiscard]] virtual bool IsFullScreenDetected() const = 0;

    /// @brief Sets the callback for DND state changes.
    /// @param callback Function called with (is_dnd_active) when state changes.
    virtual void SetOnDndChange(std::function<void(bool)> callback) = 0;

    /// @brief Sets the polling interval for state checks.
    /// @param interval Polling interval (default: 1 second).
    virtual void SetPollingInterval(std::chrono::milliseconds interval) = 0;

    /// @brief Gets the current polling interval.
    /// @return The configured polling interval.
    [[nodiscard]] virtual std::chrono::milliseconds GetPollingInterval() const = 0;
};

/// @brief Creates platform-specific DND detector implementation.
/// @return Unique pointer to the DND detector implementation.
std::unique_ptr<IDndDetector> CreateDndDetector();

}  // namespace platform
}  // namespace blinkbreak

#endif  // BLINKBREAK_PLATFORM_INTERFACE_HPP
