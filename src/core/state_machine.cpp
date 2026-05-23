/// @file state_machine.cpp
/// @brief Implementation of the StateMachine class.

#include "state_machine.hpp"

#include <format>
#include <type_traits>
#include <utility>

#include <spdlog/spdlog.h>

namespace blinkbreak
{

StateMachine::StateMachine() : current_state_(State::kIdle), on_change_(nullptr)
{
  spdlog::debug("StateMachine initialized in {} state", StateToString(current_state_));
}

StateMachine::~StateMachine() = default;

State StateMachine::GetCurrentState() const
{
  return current_state_;
}

TransitionResult StateMachine::ProcessEvent(const Event& event)
{
  auto next_state = GetNextState(event);

  if (!next_state.has_value())
  {
    std::string error =
        std::format("Invalid transition from {} state", StateToString(current_state_));
    spdlog::warn("{}", error);
    return {false, current_state_, error};
  }

  State old_state = current_state_;
  current_state_ = next_state.value();

  spdlog::info("State transition: {} -> {}", StateToString(old_state),
               StateToString(current_state_));

  if (on_change_)
  {
    on_change_(old_state, current_state_, event);
  }

  return {true, current_state_, std::nullopt};
}

bool StateMachine::CanTransition(const Event& event) const
{
  return GetNextState(event).has_value();
}

void StateMachine::SetOnStateChange(StateChangeCallback callback)
{
  on_change_ = std::move(callback);
}

void StateMachine::ForceState(State state)
{
  spdlog::warn("Forcing state transition: {} -> {}", StateToString(current_state_),
               StateToString(state));
  current_state_ = state;
}

std::optional<State> StateMachine::GetNextState(const Event& event) const
{
  return std::visit(
      [this](const auto& e) -> std::optional<State>
      {
        using T = std::decay_t<decltype(e)>;

        if constexpr (std::is_same_v<T, StartEvent>)
        {
          if (current_state_ == State::kIdle)
          {
            return State::kRunning;
          }
        }
        else if constexpr (std::is_same_v<T, PauseEvent>)
        {
          if (current_state_ == State::kRunning || current_state_ == State::kSnoozed)
          {
            return State::kPaused;
          }
        }
        else if constexpr (std::is_same_v<T, ResumeEvent>)
        {
          if (current_state_ == State::kPaused)
          {
            return State::kRunning;
          }
        }
        else if constexpr (std::is_same_v<T, ResetEvent>)
        {
          if (current_state_ != State::kIdle)
          {
            return State::kIdle;
          }
        }
        else if constexpr (std::is_same_v<T, TimerExpiredEvent>)
        {
          if (current_state_ == State::kRunning)
          {
            return State::kBreakActive;
          }
        }
        else if constexpr (std::is_same_v<T, BreakCompletedEvent>)
        {
          if (current_state_ == State::kBreakActive)
          {
            return State::kRunning;
          }
        }
        else if constexpr (std::is_same_v<T, SkipEvent>)
        {
          if (current_state_ == State::kBreakActive)
          {
            return State::kRunning;
          }
        }
        else if constexpr (std::is_same_v<T, SnoozeEvent>)
        {
          if (current_state_ == State::kBreakActive)
          {
            return State::kSnoozed;
          }
        }
        else if constexpr (std::is_same_v<T, SnoozeExpiredEvent>)
        {
          if (current_state_ == State::kSnoozed)
          {
            return State::kBreakActive;
          }
        }
        else if constexpr (std::is_same_v<T, UserIdleEvent>)
        {
          if (current_state_ == State::kRunning)
          {
            return State::kPaused;
          }
        }
        else if constexpr (std::is_same_v<T, UserActiveEvent>)
        {
          if (current_state_ == State::kPaused)
          {
            return State::kRunning;
          }
        }

        return std::nullopt;
      },
      event);
}

}  // namespace blinkbreak
