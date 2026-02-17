/// @file config_types.hpp
/// @brief Configuration data structures.

#pragma once

#include <blinkbreak/types.hpp>

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
};

/// @brief Configuration for notifications.
struct NotificationConfig {
    bool enabled = true;                       ///< Whether notifications are enabled.
    Duration warning_time = Duration::zero();  ///< Time before break to show warning.
    bool respect_dnd = true;                   ///< Respect Do Not Disturb mode.
};

/// @brief Configuration for the break overlay.
struct OverlayConfig {
    float opacity = 0.7f;                         ///< Overlay opacity (0.0 - 1.0).
    bool show_on_all_monitors = true;             ///< Show overlay on all monitors.
    bool allow_skip = true;                       ///< Allow skipping breaks.
    bool allow_snooze = true;                     ///< Allow snoozing breaks.
    Duration snooze_duration = Duration(300);  ///< Default snooze duration.
};

/// @brief Complete application configuration.
struct AppConfig {
    BreakConfig short_break;          ///< Short break configuration.
    BreakConfig long_break;           ///< Long break configuration.
    IdleConfig idle;                  ///< Idle detection configuration.
    NotificationConfig notification;  ///< Notification configuration.
    OverlayConfig overlay;            ///< Overlay configuration.
    bool start_minimized = false;     ///< Start in system tray.
    bool auto_start = false;          ///< Auto-start timer on launch.
};

}  // namespace blinkbreak
