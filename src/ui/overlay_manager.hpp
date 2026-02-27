/// @file overlay_manager.hpp
/// @brief Overlay window manager for break overlays.

#ifndef BLINKBREAK_UI_OVERLAY_MANAGER_HPP
#define BLINKBREAK_UI_OVERLAY_MANAGER_HPP

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "core/break_scheduler.hpp"
#include "platform/platform_interface.hpp"


namespace slint {
template <typename T>
class ComponentHandle;
}

class BreakOverlay;

namespace blinkbreak {

/// @brief Manages break overlay windows across multiple monitors.
///
/// Creates one Slint BreakOverlay window per target monitor and positions
/// each window to cover the full monitor area using physical pixel
/// coordinates from Win32 monitor enumeration.
class OverlayManager {
public:
    /// @brief Constructs an overlay manager.
    OverlayManager();

    /// @brief Destructor.
    ~OverlayManager();

    // Non-copyable, non-movable
    OverlayManager(const OverlayManager&) = delete;
    OverlayManager& operator=(const OverlayManager&) = delete;
    OverlayManager(OverlayManager&&) = delete;
    OverlayManager& operator=(OverlayManager&&) = delete;

    /// @brief Sets the monitor manager used for multi-monitor enumeration.
    /// @param monitor_manager Shared pointer to a monitor manager.
    void SetMonitorManager(std::shared_ptr<platform::IMonitorManager> monitor_manager);

    /// @brief Sets whether overlays should appear on all monitors or primary only.
    /// @param all_monitors True for all monitors, false for primary only.
    void SetShowOnAllMonitors(bool all_monitors);

    /// @brief Gets whether overlays appear on all monitors.
    [[nodiscard]] bool GetShowOnAllMonitors() const;

    /// @brief Shows the overlay with the provided break info.
    /// @param info Break information for message and buttons.
    void Show(const BreakInfo& info);

    /// @brief Hides all overlay windows.
    void Hide();

    /// @brief Updates the countdown text on all visible overlays.
    /// @param time_remaining Formatted time string.
    void UpdateTimeRemaining(const std::string& time_remaining);

    /// @brief Updates the break message text on all visible overlays.
    /// @param message Message to display.
    void UpdateMessage(const std::string& message);

    /// @brief Updates overlay button availability.
    /// @param can_skip Whether skip is available.
    /// @param can_snooze Whether snooze is available.
    void UpdateActions(bool can_skip, bool can_snooze);

    /// @brief Updates overlay opacity.
    /// @param opacity Background opacity (0.0 - 1.0).
    void UpdateOpacity(float opacity);

    /// @brief Updates the snooze button label.
    /// @param label Label to display (e.g., "Snooze (5 min)").
    void UpdateSnoozeLabel(const std::string& label);

    /// @brief Sets the callback for skip action.
    /// @param callback Invoked when the user clicks Skip.
    void SetOnSkip(std::function<void()> callback);

    /// @brief Sets the callback for snooze action.
    /// @param callback Invoked when the user clicks Snooze.
    void SetOnSnooze(std::function<void()> callback);

    /// @brief Gets whether any overlay is currently visible.
    /// @return True if visible.
    [[nodiscard]] bool IsVisible() const;

private:
    /// @brief Holds one overlay window and the monitor it covers.
    struct OverlayInstance {
        std::unique_ptr<slint::ComponentHandle<BreakOverlay>> handle;
        platform::MonitorInfo monitor;
    };

    /// @brief Creates overlay instances for the configured monitors.
    /// Must be called from the Slint event loop thread.
    void CreateOverlays();

    /// @brief Applies pending properties to an overlay.
    void ApplyProperties(OverlayInstance& instance);

    /// @brief Wires callbacks to an overlay handle.
    void WireCallbacks(const slint::ComponentHandle<BreakOverlay>& handle);

    /// @brief Positions and sizes an overlay to cover its monitor.
    void PositionOverlay(OverlayInstance& instance);

    std::vector<OverlayInstance> overlays_;
    std::shared_ptr<platform::IMonitorManager> monitor_manager_;
    bool show_on_all_monitors_ = true;

    bool visible_ = false;
    bool pending_show_ = false;
    std::string pending_message_;
    std::string pending_break_type_;
    std::string pending_time_remaining_;
    bool pending_can_skip_ = false;
    bool pending_can_snooze_ = false;
    std::string pending_snooze_label_;
    std::optional<float> pending_opacity_;
    std::function<void()> on_skip_;
    std::function<void()> on_snooze_;
    mutable std::mutex overlay_mutex_;
};

}  // namespace blinkbreak

#endif  // BLINKBREAK_UI_OVERLAY_MANAGER_HPP