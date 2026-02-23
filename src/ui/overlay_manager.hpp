/// @file overlay_manager.hpp
/// @brief Overlay window manager for break overlays.

#ifndef BLINKBREAK_UI_OVERLAY_MANAGER_HPP
#define BLINKBREAK_UI_OVERLAY_MANAGER_HPP

#include "core/break_scheduler.hpp"
#include "platform/monitor_manager.hpp"

#include <slint.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace slint {
template <typename T>
class ComponentHandle;
}

class BreakOverlay;

namespace blinkbreak::platform {
class IMonitorManager;
}

namespace blinkbreak {

/// @brief Manages the break overlay window lifecycle and updates across monitors.
/// @details Supports multi-monitor overlays, test hooks for monitor enumeration, and UI event loop scheduling.
class OverlayManager {
public:
    /// @brief Constructs an overlay manager.
    OverlayManager();

    /// @brief Constructs an overlay manager with a custom monitor manager (test hook).
    /// @param monitor_manager Monitor manager to use for enumeration and change callbacks.
    explicit OverlayManager(std::unique_ptr<platform::IMonitorManager> monitor_manager);

    /// @brief Constructs an overlay manager with a custom invoker (test hook).
    /// @param monitor_manager Monitor manager to use for enumeration and change callbacks.
    /// @param event_loop_invoker Callback used to schedule work on the UI event loop.
    OverlayManager(std::unique_ptr<platform::IMonitorManager> monitor_manager,
                   std::function<void(std::function<void()>)> event_loop_invoker);

    /// @brief Destructor.
    ~OverlayManager();

    // Non-copyable, non-movable
    OverlayManager(const OverlayManager&) = delete;
    OverlayManager& operator=(const OverlayManager&) = delete;
    OverlayManager(OverlayManager&&) = delete;
    OverlayManager& operator=(OverlayManager&&) = delete;

    /// @brief Shows the overlay with the provided break info.
    /// @param info Break information for message and buttons.
    void Show(const BreakInfo& info);

    /// @brief Hides the overlay window.
    void Hide();

    /// @brief Updates the countdown text.
    /// @param time_remaining Formatted time string.
    void UpdateTimeRemaining(const std::string& time_remaining);

    /// @brief Updates the break message text.
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

    /// @brief Sets whether overlays should appear on all monitors.
    /// @param show_on_all True to show on all monitors.
    void SetShowOnAllMonitors(bool show_on_all);

    /// @brief Handles a monitor configuration change.
    /// @details Recreates overlays and reapplies state using the latest monitor list.
    /// @note UI updates are scheduled on the event loop; safe to call from a monitor-change callback thread.
    void HandleMonitorChange();

    /// @brief Sets the callback for skip action.
    /// @param callback Invoked when the user clicks Skip.
    void SetOnSkip(std::function<void()> callback);

    /// @brief Sets the callback for snooze action.
    /// @param callback Invoked when the user clicks Snooze.
    void SetOnSnooze(std::function<void()> callback);

    /// @brief Gets whether the overlay is currently visible.
    /// @return True if visible.
    [[nodiscard]] bool IsVisible() const;

    /// @brief Gets the number of overlay instances (test hook).
    /// @return Count of overlay windows currently managed.
    [[nodiscard]] std::size_t GetOverlayCount() const;

private:
    void RunOnEventLoop(std::function<void()> task);
    void PrepareOverlaysForLayout();
    void PreSizeOverlaysForMonitors();
    void ScheduleDeferredPositioning();
    void PositionOverlaysOnMonitors();
    void VerifyAndFixOverlays(const std::vector<platform::MonitorInfo>& monitors,
                              std::size_t overlay_count);
    void FinalPortraitPass(const std::vector<platform::MonitorInfo>& monitors,
                           std::size_t overlay_count);

    void EnsureOverlaysForMonitors(std::size_t monitor_count);
    void ApplyStateToOverlays(const std::string& message,
                              const std::string& break_type,
                              const std::string& time_remaining,
                              const std::string& snooze_label,
                              bool can_skip,
                              bool can_snooze,
                              const std::optional<float>& opacity_override,
                              bool should_show);

    std::unique_ptr<platform::IMonitorManager> monitor_manager_;
    std::vector<std::unique_ptr<slint::ComponentHandle<BreakOverlay>>> overlays_;
    bool visible_;
    bool pending_show_;
    std::string pending_message_;
    std::string pending_break_type_;
    std::string pending_time_remaining_;
    bool pending_can_skip_;
    bool pending_can_snooze_;
    std::string pending_snooze_label_;
    std::optional<float> pending_opacity_;
    bool show_on_all_monitors_;
    std::function<void()> on_skip_;
    std::function<void()> on_snooze_;
    std::function<void(std::function<void()>)> event_loop_invoker_;
    std::vector<platform::MonitorInfo> cached_monitors_;
    std::unique_ptr<slint::Timer> position_timer_;
    int positioning_retry_count_ = 0;
    mutable std::mutex overlay_mutex_;
};

}  // namespace blinkbreak

#endif  // BLINKBREAK_UI_OVERLAY_MANAGER_HPP