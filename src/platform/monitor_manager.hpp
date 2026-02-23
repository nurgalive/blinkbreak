/// @file monitor_manager.hpp
/// @brief Monitor enumeration interface.

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace blinkbreak::platform {

/// @brief Information about a display monitor.
struct MonitorInfo {
    int id = 0;               ///< Monitor identifier.
    std::string name;         ///< Display name.
    int x = 0;                ///< Left position.
    int y = 0;                ///< Top position.
    int width = 0;            ///< Width in pixels.
    int height = 0;           ///< Height in pixels.
    float scale_factor = 1.0f;  ///< Per-monitor scale factor (DPI / 96.0f).
    bool is_primary = false;  ///< Whether this is the primary monitor.
};

/// @brief Interface for monitor management.
class IMonitorManager {
public:
    virtual ~IMonitorManager() = default;

    /// @brief Gets all available monitors.
    /// @return Vector of monitor info entries.
    virtual std::vector<MonitorInfo> GetMonitors() = 0;

    /// @brief Gets the primary monitor.
    /// @return Monitor info for the primary display.
    virtual MonitorInfo GetPrimaryMonitor() = 0;

    /// @brief Sets a callback for monitor configuration changes.
    /// @param callback Invoked when monitors are added/removed or changed.
    virtual void SetOnMonitorChange(std::function<void()> callback) = 0;
};

/// @brief Creates a platform-specific monitor manager.
/// @return Unique pointer to the monitor manager implementation.
std::unique_ptr<IMonitorManager> CreateMonitorManager();

}  // namespace blinkbreak::platform