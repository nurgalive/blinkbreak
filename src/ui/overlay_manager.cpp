/// @file overlay_manager.cpp
/// @brief Implementation of the OverlayManager class.

#include "overlay_manager.hpp"

#include "overlay.h"

#include <slint.h>

#include <algorithm>
#include <cstdint>
#include <format>
#include <string>
#include <utility>

namespace blinkbreak {
namespace {

std::string FormatDuration(Duration duration) {
    const auto total_seconds =
        std::max<std::int64_t>(0, static_cast<std::int64_t>(duration.count()));
    const auto minutes = total_seconds / 60;
    const auto seconds = total_seconds % 60;
    return std::format("{:02}:{:02}", minutes, seconds);
}

slint::SharedString ToSharedString(const std::string& value) {
    return slint::SharedString(value);
}

void RunOnEventLoop(std::function<void()> task) {
    slint::invoke_from_event_loop(std::move(task));
}

}  // namespace

OverlayManager::OverlayManager()
    : visible_(false),
      pending_show_(false),
      pending_can_skip_(false),
      pending_can_snooze_(false) {}

OverlayManager::~OverlayManager() {
    Hide();
}

void OverlayManager::Show(const BreakInfo& info) {
    {
        std::lock_guard lock(overlay_mutex_);
        pending_show_ = true;
        pending_message_ = info.message;
        pending_break_type_ = std::string(BreakTypeToString(info.type));
        pending_time_remaining_ = FormatDuration(info.duration);
        pending_can_skip_ = info.can_skip;
        pending_can_snooze_ = info.can_snooze;
        if (info.can_snooze && info.snooze_duration > Duration::zero()) {
            const auto minutes = std::max<std::int64_t>(
                1, (info.snooze_duration.count() + 59) / 60);
            pending_snooze_label_ = std::format("Snooze ({} min)", minutes);
        } else {
            pending_snooze_label_.clear();
        }
    }

    RunOnEventLoop([this]() {
        EnsureOverlay();
        if (!overlay_) {
            return;
        }

        std::string message;
        std::string break_type;
        std::string time_remaining;
        std::string snooze_label;
        bool can_skip = false;
        bool can_snooze = false;
        std::optional<float> opacity_override;
        bool should_show = false;

        {
            std::lock_guard lock(overlay_mutex_);
            message = pending_message_;
            break_type = pending_break_type_;
            time_remaining = pending_time_remaining_;
            snooze_label = pending_snooze_label_;
            can_skip = pending_can_skip_;
            can_snooze = pending_can_snooze_;
            opacity_override = pending_opacity_;
            pending_opacity_.reset();
            should_show = pending_show_;
        }

        auto overlay_handle = *overlay_;
        overlay_handle->set_message(ToSharedString(message));
        overlay_handle->set_break_type(ToSharedString(break_type));
        overlay_handle->set_time_remaining(ToSharedString(time_remaining));
        overlay_handle->set_can_skip(can_skip);
        overlay_handle->set_can_snooze(can_snooze);
        if (!snooze_label.empty()) {
            overlay_handle->set_snooze_label(ToSharedString(snooze_label));
        }

        if (opacity_override) {
            overlay_handle->set_overlay_opacity(std::clamp(*opacity_override, 0.0f, 1.0f));
        }

        if (should_show) {
            overlay_handle->window().set_fullscreen(true);
            overlay_handle->show();
        }

        std::lock_guard lock(overlay_mutex_);
        visible_ = should_show;
    });
}

void OverlayManager::Hide() {
    {
        std::lock_guard lock(overlay_mutex_);
        pending_show_ = false;
    }

    RunOnEventLoop([this]() {
        if (overlay_) {
            auto overlay_handle = *overlay_;
            overlay_handle->hide();
        }
        std::lock_guard lock(overlay_mutex_);
        visible_ = false;
    });
}

void OverlayManager::UpdateTimeRemaining(const std::string& time_remaining) {
    bool has_overlay = false;
    {
        std::lock_guard lock(overlay_mutex_);
        pending_time_remaining_ = time_remaining;
        has_overlay = (overlay_ != nullptr);
    }

    if (!has_overlay) {
        return;
    }

    RunOnEventLoop([this]() {
        if (!overlay_) {
            return;
        }

        std::string value;
        {
            std::lock_guard lock(overlay_mutex_);
            value = pending_time_remaining_;
        }

        auto overlay_handle = *overlay_;
        overlay_handle->set_time_remaining(ToSharedString(value));
    });
}

void OverlayManager::UpdateMessage(const std::string& message) {
    bool has_overlay = false;
    {
        std::lock_guard lock(overlay_mutex_);
        pending_message_ = message;
        has_overlay = (overlay_ != nullptr);
    }

    if (!has_overlay) {
        return;
    }

    RunOnEventLoop([this]() {
        if (!overlay_) {
            return;
        }

        std::string value;
        {
            std::lock_guard lock(overlay_mutex_);
            value = pending_message_;
        }

        auto overlay_handle = *overlay_;
        overlay_handle->set_message(ToSharedString(value));
    });
}

void OverlayManager::UpdateActions(bool can_skip, bool can_snooze) {
    bool has_overlay = false;
    {
        std::lock_guard lock(overlay_mutex_);
        pending_can_skip_ = can_skip;
        pending_can_snooze_ = can_snooze;
        has_overlay = (overlay_ != nullptr);
    }

    if (!has_overlay) {
        return;
    }

    RunOnEventLoop([this]() {
        if (!overlay_) {
            return;
        }

        bool can_skip = false;
        bool can_snooze = false;
        {
            std::lock_guard lock(overlay_mutex_);
            can_skip = pending_can_skip_;
            can_snooze = pending_can_snooze_;
        }

        auto overlay_handle = *overlay_;
        overlay_handle->set_can_skip(can_skip);
        overlay_handle->set_can_snooze(can_snooze);
    });
}

void OverlayManager::UpdateOpacity(float opacity) {
    bool has_overlay = false;
    {
        std::lock_guard lock(overlay_mutex_);
        pending_opacity_ = opacity;
        has_overlay = (overlay_ != nullptr);
    }

    if (!has_overlay) {
        return;
    }

    RunOnEventLoop([this]() {
        if (!overlay_) {
            return;
        }

        std::optional<float> opacity;
        {
            std::lock_guard lock(overlay_mutex_);
            opacity = pending_opacity_;
            pending_opacity_.reset();
        }

        if (!opacity) {
            return;
        }

        auto overlay_handle = *overlay_;
        overlay_handle->set_overlay_opacity(std::clamp(*opacity, 0.0f, 1.0f));
    });
}

void OverlayManager::UpdateSnoozeLabel(const std::string& label) {
    bool has_overlay = false;
    {
        std::lock_guard lock(overlay_mutex_);
        pending_snooze_label_ = label;
        has_overlay = (overlay_ != nullptr);
    }

    if (!has_overlay) {
        return;
    }

    RunOnEventLoop([this]() {
        if (!overlay_) {
            return;
        }

        std::string label;
        {
            std::lock_guard lock(overlay_mutex_);
            label = pending_snooze_label_;
        }

        if (label.empty()) {
            return;
        }

        auto overlay_handle = *overlay_;
        overlay_handle->set_snooze_label(ToSharedString(label));
    });
}

void OverlayManager::SetOnSkip(std::function<void()> callback) {
    bool has_overlay = false;
    {
        std::lock_guard lock(overlay_mutex_);
        on_skip_ = std::move(callback);
        has_overlay = (overlay_ != nullptr);
    }

    if (!has_overlay) {
        return;
    }

    RunOnEventLoop([this]() {
        if (!overlay_) {
            return;
        }
        auto overlay_handle = *overlay_;
        overlay_handle->on_skip_clicked([this] {
            std::function<void()> callback;
            {
                std::lock_guard lock(overlay_mutex_);
                callback = on_skip_;
            }
            if (callback) {
                callback();
            }
        });
    });
}

void OverlayManager::SetOnSnooze(std::function<void()> callback) {
    bool has_overlay = false;
    {
        std::lock_guard lock(overlay_mutex_);
        on_snooze_ = std::move(callback);
        has_overlay = (overlay_ != nullptr);
    }

    if (!has_overlay) {
        return;
    }

    RunOnEventLoop([this]() {
        if (!overlay_) {
            return;
        }
        auto overlay_handle = *overlay_;
        overlay_handle->on_snooze_clicked([this] {
            std::function<void()> callback;
            {
                std::lock_guard lock(overlay_mutex_);
                callback = on_snooze_;
            }
            if (callback) {
                callback();
            }
        });
    });
}

bool OverlayManager::IsVisible() const {
    std::lock_guard lock(overlay_mutex_);
    return visible_;
}

void OverlayManager::EnsureOverlay() {
    if (overlay_) {
        return;
    }

    overlay_ = std::make_unique<slint::ComponentHandle<BreakOverlay>>(BreakOverlay::create());
    auto overlay_handle = *overlay_;

    overlay_handle->on_skip_clicked([this] {
        std::function<void()> callback;
        {
            std::lock_guard lock(overlay_mutex_);
            callback = on_skip_;
        }
        if (callback) {
            callback();
        }
    });

    overlay_handle->on_snooze_clicked([this] {
        std::function<void()> callback;
        {
            std::lock_guard lock(overlay_mutex_);
            callback = on_snooze_;
        }
        if (callback) {
            callback();
        }
    });
}

}  // namespace blinkbreak