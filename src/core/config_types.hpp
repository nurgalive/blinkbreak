/// @file config_types.hpp
/// @brief Configuration data structures.

#pragma once

#include <blinkbreak/types.hpp>

#include <glaze/glaze.hpp>
#include <string>
#include <vector>

namespace blinkbreak {

/// @brief Configuration for a break type (short or long).
struct BreakConfig {
    bool enabled = true;                   ///< Whether this break type is enabled.
    Duration interval = Duration::zero();  ///< Time between breaks.
    Duration duration = Duration::zero();  ///< Break duration.
    std::vector<std::string> messages;     ///< Messages to display during break.
    bool rotate_messages = true;           ///< Whether to rotate through messages.
};

/// @brief Configuration for idle detection.
struct IdleConfig {
    bool enabled = true;                    ///< Whether idle detection is enabled.
    Duration threshold = Duration::zero();  ///< Time before considered idle.
    bool pause_on_idle = true;              ///< Pause timer when idle.
    bool reset_on_idle = false;             ///< Reset timer when idle.
    bool show_timer = false;                ///< Show idle timer in UI.
};

/// @brief Configuration for notifications.
struct NotificationConfig {
    bool enabled = true;                   ///< Whether notifications are enabled.
    Duration warning_time = Duration(30);  ///< Time before break to show warning.
    bool respect_dnd = true;               ///< Respect Do Not Disturb mode.
    bool respect_fullscreen = false;       ///< Respect full-screen app detection (QUNS_BUSY).
};

/// @brief Configuration for the break overlay.
struct OverlayConfig {
    float opacity = 0.7f;                      ///< Overlay opacity (0.0 - 1.0).
    bool show_on_all_monitors = true;          ///< Show overlay on all monitors.
    bool allow_skip = true;                    ///< Allow skipping breaks.
    bool allow_snooze = true;                  ///< Allow snoozing breaks.
    Duration snooze_duration = Duration(300);  ///< Default snooze duration.
};

/// @brief Configuration for application theming.
struct ThemeConfig {
    bool follow_system = true;  ///< Follow the operating system theme.
    bool dark_mode = false;     ///< Use dark mode when not following the system.
};

/// @brief Complete application configuration.
struct AppConfig {
    BreakConfig short_break;          ///< Short break configuration.
    BreakConfig long_break;           ///< Long break configuration.
    IdleConfig idle;                  ///< Idle detection configuration.
    NotificationConfig notification;  ///< Notification configuration.
    OverlayConfig overlay;            ///< Overlay configuration.
    ThemeConfig theme;                ///< Application theme configuration.
    bool start_minimized = false;     ///< Start in system tray.
    bool auto_start = false;          ///< Auto-start timer on launch.
};

}  // namespace blinkbreak

template <>
struct glz::meta<blinkbreak::BreakConfig> {
    using T = blinkbreak::BreakConfig;
    static constexpr auto value =
        glz::object("enabled", &T::enabled, "interval_seconds", &T::interval, "duration_seconds",
                    &T::duration, "messages", &T::messages, "rotate_messages", &T::rotate_messages);
};

template <>
struct glz::meta<blinkbreak::IdleConfig> {
    using T = blinkbreak::IdleConfig;
    static constexpr auto value = glz::object(
        "enabled", &T::enabled, "threshold_seconds", &T::threshold, "pause_on_idle",
        &T::pause_on_idle, "reset_on_idle", &T::reset_on_idle, "show_timer", &T::show_timer);
};

template <>
struct glz::meta<blinkbreak::NotificationConfig> {
    using T = blinkbreak::NotificationConfig;
    static constexpr auto value =
        glz::object("enabled", &T::enabled, "warning_seconds", &T::warning_time, "respect_dnd",
                    &T::respect_dnd, "respect_fullscreen", &T::respect_fullscreen);
};

template <>
struct glz::meta<blinkbreak::OverlayConfig> {
    using T = blinkbreak::OverlayConfig;
    static constexpr auto value =
        glz::object("opacity", &T::opacity, "show_on_all_monitors", &T::show_on_all_monitors,
                    "allow_skip", &T::allow_skip, "allow_snooze", &T::allow_snooze,
                    "snooze_duration_seconds", &T::snooze_duration);
};

template <>
struct glz::meta<blinkbreak::ThemeConfig> {
    using T = blinkbreak::ThemeConfig;
    static constexpr auto value =
        glz::object("follow_system", &T::follow_system, "dark_mode", &T::dark_mode);
};

template <>
struct glz::meta<blinkbreak::AppConfig> {
    using T = blinkbreak::AppConfig;
    static constexpr auto value =
        glz::object("short_break", &T::short_break, "long_break", &T::long_break, "idle", &T::idle,
                    "notification", &T::notification, "overlay", &T::overlay, "theme", &T::theme,
                    "start_minimized", &T::start_minimized, "auto_start", &T::auto_start);
};
