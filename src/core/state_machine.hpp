/// @file state_machine.hpp
/// @brief Finite state machine for application state management.

#ifndef BLINKBREAK_CORE_STATE_MACHINE_HPP
#define BLINKBREAK_CORE_STATE_MACHINE_HPP

#include <cstdint>
#include <functional>

#include <blinkbreak/events.hpp>
#include <blinkbreak/types.hpp>

namespace blinkbreak
{

/// @brief Enumeration of all possible application states.
enum class State : std::uint8_t
{
  kIdle,        ///< Application is idle, waiting for user to start.
  kRunning,     ///< Timer is actively counting down.
  kPaused,      ///< Timer is paused by user action.
  kSnoozed,     ///< Break was snoozed, snooze timer counting.
  kBreakActive  ///< Break overlay is displayed.
};

/// @brief Converts State to string representation.
/// @param state The state to convert.
/// @return String representation of the state.
[[nodiscard]] constexpr std::string_view StateToString(State state)
{
  switch (state)
  {
    case State::kIdle:
      return "Idle";
    case State::kRunning:
      return "Running";
    case State::kPaused:
      return "Paused";
    case State::kSnoozed:
      return "Snoozed";
    case State::kBreakActive:
      return "BreakActive";
  }
  return "Unknown";
}

/// @brief Result of a state transition.
struct TransitionResult
{
  bool success;                      ///< Whether the transition succeeded.
  State new_state;                   ///< The new state after transition.
  std::optional<std::string> error;  ///< Error message if transition failed.
};

/// @brief Callback type for state transition notifications.
/// @param old_state The state before transition.
/// @param new_state The state after transition.
/// @param event The event that triggered the transition.
using StateChangeCallback =
    std::function<void(State old_state, State new_state, const Event& event)>;

/// @brief Finite state machine for managing application states.
///
/// The StateMachine class implements a robust FSM pattern that validates
/// state transitions and notifies listeners of state changes. All transitions
/// are logged for debugging purposes.
///
/// State Transition Diagram:
/// ```text
///                    ┌─────────────┐
///           Start    │             │ UserIdle
///     ┌─────────────►│   Running   │◄────────┐
///     │              │             │         │
///     │              └──────┬──────┘         │
///     │                     │                │
///     │                     │ TimerExpired   │
///     │                     ▼                │
/// ┌───┴───┐          ┌─────────────┐         │
/// │       │◄─────────│             │         │
/// │ Idle  │  Reset   │ BreakActive │         │
/// │       │◄─────────│             │         │
/// └───────┘          └──────┬──────┘         │
///     ▲                     │                │
///     │                     │ Snooze         │
///     │                     ▼                │
///     │              ┌─────────────┐         │
///     │   Expired    │             │         │
///     └──────────────│   Snoozed   │         │
///                    │             │         │
///                    └─────────────┘         │
///                                            │
///                    ┌─────────────┐         │
///           Pause    │             │ Resume  │
///     ┌─────────────►│   Paused    │─────────┘
///     │              │             │
///     │              └─────────────┘
/// ```
class StateMachine
{
public:
  /// @brief Constructs a state machine in the Idle state.
  StateMachine();

  /// @brief Destructor.
  ~StateMachine();

  // Non-copyable, non-movable
  StateMachine(const StateMachine&) = delete;
  StateMachine& operator=(const StateMachine&) = delete;
  StateMachine(StateMachine&&) = delete;
  StateMachine& operator=(StateMachine&&) = delete;

  /// @brief Gets the current state.
  /// @return The current state.
  [[nodiscard]] State GetCurrentState() const;

  /// @brief Processes an event and potentially transitions to a new state.
  /// @param event The event to process.
  /// @return The result of the transition attempt.
  TransitionResult ProcessEvent(const Event& event);

  /// @brief Checks if a transition is valid from the current state.
  /// @param event The event to check.
  /// @return True if the transition is valid, false otherwise.
  [[nodiscard]] bool CanTransition(const Event& event) const;

  /// @brief Sets the callback for state change notifications.
  /// @param callback The callback function.
  void SetOnStateChange(StateChangeCallback callback);

  /// @brief Forces a transition to a specific state (for testing/recovery).
  /// @param state The state to transition to.
  /// @note This bypasses normal transition validation.
  void ForceState(State state);

private:
  /// @brief Determines the next state based on current state and event.
  /// @param event The triggering event.
  /// @return The next state, or nullopt if transition is invalid.
  [[nodiscard]] std::optional<State> GetNextState(const Event& event) const;

  State current_state_;            ///< The current state.
  StateChangeCallback on_change_;  ///< Callback for state changes.
};

}  // namespace blinkbreak

#endif  // BLINKBREAK_CORE_STATE_MACHINE_HPP
