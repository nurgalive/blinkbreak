/// @file platform_interface.hpp
/// @brief Platform abstraction interfaces.

#ifndef BLINKBREAK_PLATFORM_INTERFACE_HPP
#define BLINKBREAK_PLATFORM_INTERFACE_HPP

#include <chrono>
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

}  // namespace platform
}  // namespace blinkbreak

#endif  // BLINKBREAK_PLATFORM_INTERFACE_HPP