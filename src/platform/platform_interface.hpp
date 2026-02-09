/// @file platform_interface.hpp
/// @brief Platform abstraction interfaces.

#ifndef BLINKBREAK_PLATFORM_INTERFACE_HPP
#define BLINKBREAK_PLATFORM_INTERFACE_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace blinkbreak {
namespace platform {

/// @brief Menu item for system tray context menu.
struct MenuItem {
    std::string text;                      ///< Display text.
    std::function<void()> callback;        ///< Action callback.
    bool enabled = true;                   ///< Whether item is enabled.
    bool checked = false;                  ///< Whether item is checked.
    bool is_separator = false;             ///< Whether this is a separator.
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

}  // namespace platform
}  // namespace blinkbreak

#endif  // BLINKBREAK_PLATFORM_INTERFACE_HPP