/// @file overlay_manager.hpp
/// @brief Overlay window manager for break overlays.

#ifndef BLINKBREAK_UI_OVERLAY_MANAGER_HPP
#define BLINKBREAK_UI_OVERLAY_MANAGER_HPP

#include "core/break_scheduler.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace slint {
template <typename T>
class ComponentHandle;
}

class BreakOverlay;

namespace blinkbreak {

/// @brief Manages the break overlay window lifecycle and updates.
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

    /// @brief Sets the callback for skip action.
    /// @param callback Invoked when the user clicks Skip.
    void SetOnSkip(std::function<void()> callback);

    /// @brief Sets the callback for snooze action.
    /// @param callback Invoked when the user clicks Snooze.
    void SetOnSnooze(std::function<void()> callback);

    /// @brief Gets whether the overlay is currently visible.
    /// @return True if visible.
    [[nodiscard]] bool IsVisible() const;

private:
    void EnsureOverlay();

    std::unique_ptr<slint::ComponentHandle<BreakOverlay>> overlay_;
    bool visible_;
    bool pending_show_;
    std::string pending_message_;
    std::string pending_break_type_;
    std::string pending_time_remaining_;
    bool pending_can_skip_;
    bool pending_can_snooze_;
    std::string pending_snooze_label_;
    std::optional<float> pending_opacity_;
    std::function<void()> on_skip_;
    std::function<void()> on_snooze_;
    mutable std::mutex overlay_mutex_;
};

}  // namespace blinkbreak

#endif  // BLINKBREAK_UI_OVERLAY_MANAGER_HPP