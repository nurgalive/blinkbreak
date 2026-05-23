/// @file events.hpp
/// @brief Event definitions for the state machine.

#ifndef BLINKBREAK_EVENTS_HPP
#define BLINKBREAK_EVENTS_HPP

#include <variant>

#include <blinkbreak/types.hpp>

namespace blinkbreak
{

/// @brief Event triggered when the start action is requested.
struct StartEvent
{
};

/// @brief Event triggered when the pause action is requested.
struct PauseEvent
{
};

/// @brief Event triggered when the resume action is requested.
struct ResumeEvent
{
};

/// @brief Event triggered when the reset action is requested.
struct ResetEvent
{
};

/// @brief Event triggered when a break timer expires.
struct TimerExpiredEvent
{
  BreakType break_type;  ///< Type of break that expired.
};

/// @brief Event triggered when a break is completed.
struct BreakCompletedEvent
{
};

/// @brief Event triggered when a break is skipped.
struct SkipEvent
{
};

/// @brief Event triggered when a break is snoozed.
struct SnoozeEvent
{
  Duration duration;  ///< Duration to snooze.
};

/// @brief Event triggered when snooze timer expires.
struct SnoozeExpiredEvent
{
};

/// @brief Event triggered when user becomes idle.
struct UserIdleEvent
{
};

/// @brief Event triggered when user becomes active.
struct UserActiveEvent
{
};

/// @brief Variant type holding all possible events.
using Event = std::variant<StartEvent, PauseEvent, ResumeEvent, ResetEvent, TimerExpiredEvent,
                           BreakCompletedEvent, SkipEvent, SnoozeEvent, SnoozeExpiredEvent,
                           UserIdleEvent, UserActiveEvent>;

}  // namespace blinkbreak

#endif  // BLINKBREAK_EVENTS_HPP
