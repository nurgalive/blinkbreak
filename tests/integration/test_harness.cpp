/// @file test_harness.cpp
/// @brief Implementation of the integration test harness.

#include "test_harness.hpp"

#include <algorithm>

namespace blinkbreak::testing
{

using namespace std::chrono_literals;

TestHarness::TestHarness()
    : idle_detector_(std::make_unique<MockIdleDetector>()),
      dnd_detector_(std::make_unique<MockDndDetector>()),
      notification_manager_(std::make_unique<MockNotificationManager>()),
      monitor_manager_(std::make_unique<MockMonitorManager>()),
      state_machine_(std::make_unique<StateMachine>())
{
  // Scheduler will be created in Initialize()
}

TestHarness::~TestHarness() = default;

void TestHarness::Initialize(const AppConfig& config)
{
  current_config_ = config;
  simulated_time_ = DurationMs(0);
  stats_ = TestStats{};
  is_paused_by_idle_ = false;
  reset_short_on_idle_triggered_ = false;
  reset_long_on_idle_triggered_ = false;
  break_in_progress_ = false;

  // Create scheduler with config
  scheduler_ =
      std::make_unique<BreakScheduler>(config.short_break, config.long_break, config.overlay);

  // Reset state machine
  state_machine_ = std::make_unique<StateMachine>();

  // Initialize mock components
  idle_detector_->SetIdleThreshold(
      std::chrono::duration_cast<std::chrono::seconds>(config.idle.threshold));
  notification_manager_->Initialize();

  SetupCallbacks();

  // Start if auto_start is enabled
  if (config.auto_start)
  {
    state_machine_->ProcessEvent(StartEvent{});
    scheduler_->Start();
    idle_detector_->Start();
    dnd_detector_->Start();
  }
}

void TestHarness::InitializeWithDefaults()
{
  AppConfig config;

  // Short break: every 20 minutes, 20 seconds duration
  config.short_break.enabled = true;
  config.short_break.interval = Duration(20 * 60);  // 20 minutes
  config.short_break.duration = Duration(20);       // 20 seconds
  config.short_break.messages = {"Look away from the screen", "Blink your eyes"};
  config.short_break.rotate_messages = true;

  // Long break: every 60 minutes, 5 minutes duration
  config.long_break.enabled = true;
  config.long_break.interval = Duration(60 * 60);  // 60 minutes
  config.long_break.duration = Duration(5 * 60);   // 5 minutes
  config.long_break.messages = {"Take a walk", "Stretch your body"};
  config.long_break.rotate_messages = true;

  // Idle detection
  config.idle.enabled = true;
  config.idle.threshold = Duration(5 * 60);  // 5 minutes
  config.idle.pause_on_idle = true;
  config.idle.reset_short_on_idle = true;
  config.idle.reset_short_threshold = Duration(20 * 60);  // 20 minutes
  config.idle.reset_long_on_idle = true;
  config.idle.reset_long_threshold = Duration(20 * 60);  // 20 minutes

  // Notifications
  config.notification.enabled = true;
  config.notification.warning_time = Duration(30);  // 30 seconds
  config.notification.respect_dnd = true;

  // Overlay
  config.overlay.opacity = 0.8f;
  config.overlay.show_on_all_monitors = true;
  config.overlay.allow_skip = true;
  config.overlay.allow_snooze = true;
  config.overlay.snooze_duration = Duration(5 * 60);  // 5 minutes

  // Auto-start for testing
  config.auto_start = true;

  Initialize(config);
}

void TestHarness::SetupCallbacks()
{
  // State machine callback
  state_machine_->SetOnStateChange(
      [this](State /*old_state*/, State /*new_state*/, const Event& /*event*/)
      {
        // Could add state change tracking here if needed
      });

  // Break scheduler callbacks
  scheduler_->SetOnBreakStart([this](const BreakInfo& info) { OnBreakStart(info); });

  scheduler_->SetOnBreakEnd([this](const BreakInfo& info) { OnBreakEnd(info); });

  scheduler_->SetOnWarning([this](BreakType type, Duration time_until)
                           { OnWarning(type, time_until); },
                           current_config_.notification.warning_time);

  // Idle detector callbacks
  idle_detector_->SetOnIdle(
      [this]()
      {
        if (current_config_.idle.enabled && current_config_.idle.pause_on_idle)
        {
          if (state_machine_->GetCurrentState() == State::kRunning)
          {
            state_machine_->ProcessEvent(PauseEvent{});
            scheduler_->Pause();
            is_paused_by_idle_ = true;
            ++stats_.idle_pauses;
          }
        }
      });

  idle_detector_->SetOnActive(
      [this]()
      {
        if (is_paused_by_idle_)
        {
          state_machine_->ProcessEvent(ResumeEvent{});
          scheduler_->Resume();
          is_paused_by_idle_ = false;
        }
        reset_short_on_idle_triggered_ = false;
        reset_long_on_idle_triggered_ = false;
      });

  // DND detector callback
  dnd_detector_->SetOnDndChange(
      [this](bool is_active)
      {
        // Just track state changes; suppression is checked at break time
        (void)is_active;
      });
}

void TestHarness::AdvanceTime(DurationMs duration, DurationMs step_size)
{
  DurationMs remaining = duration;

  while (remaining > DurationMs(0))
  {
    DurationMs step = std::min(remaining, step_size);
    ProcessTimerTick(step);
    remaining -= step;
    simulated_time_ += step;
    stats_.total_simulated_time += step;
  }
}

DurationMs TestHarness::AdvanceToNextEvent()
{
  auto time_until_break = scheduler_->GetTimeUntilNextBreak();
  if (!time_until_break.has_value())
  {
    return DurationMs(0);
  }

  auto ms = std::chrono::duration_cast<DurationMs>(*time_until_break);
  // Add a small buffer to ensure we trigger the event
  ms += DurationMs(100);
  AdvanceTime(ms);
  return ms;
}

void TestHarness::ProcessTimerTick(DurationMs elapsed)
{
  // Check DND state before processing
  CheckDndState();

  // Update scheduler
  scheduler_->Update(elapsed);

  // Advance idle time if user is idle
  if (idle_detector_->IsIdle())
  {
    idle_detector_->AdvanceIdleTime(elapsed);
  }

  if (current_config_.idle.enabled && idle_detector_->IsRunning())
  {
    const auto idle_time = idle_detector_->GetIdleTime();

    if (current_config_.idle.reset_short_on_idle && !reset_short_on_idle_triggered_)
    {
      const auto reset_short_threshold =
          std::chrono::duration_cast<DurationMs>(current_config_.idle.reset_short_threshold);
      if (idle_time >= reset_short_threshold)
      {
        scheduler_->ResetShortTimer();
        reset_short_on_idle_triggered_ = true;
      }
    }

    if (current_config_.idle.reset_long_on_idle && !reset_long_on_idle_triggered_)
    {
      const auto reset_long_threshold =
          std::chrono::duration_cast<DurationMs>(current_config_.idle.reset_long_threshold);
      if (idle_time >= reset_long_threshold)
      {
        scheduler_->ResetLongTimer();
        reset_long_on_idle_triggered_ = true;
      }
    }
  }

  // Track break time
  if (break_in_progress_)
  {
    stats_.total_break_time += elapsed;
  }
}

void TestHarness::OnBreakStart(const BreakInfo& info)
{
  // Check DND suppression
  if (current_config_.notification.respect_dnd && dnd_detector_->IsDndActive())
  {
    ++stats_.dnd_suppressions;
    scheduler_->ResetTimers();
    return;
  }

  // Check fullscreen suppression
  if (current_config_.notification.respect_fullscreen && dnd_detector_->IsFullScreenDetected())
  {
    ++stats_.dnd_suppressions;
    scheduler_->ResetTimers();
    return;
  }

  // Transition state machine based on current state
  // If we're in Snoozed state, use SnoozeExpiredEvent; otherwise use TimerExpiredEvent
  if (state_machine_->GetCurrentState() == State::kSnoozed)
  {
    state_machine_->ProcessEvent(SnoozeExpiredEvent{});
  }
  else
  {
    state_machine_->ProcessEvent(TimerExpiredEvent{info.type});
  }

  break_in_progress_ = true;
  current_break_info_ = info;

  if (info.type == BreakType::kShort)
  {
    ++stats_.short_breaks_triggered;
  }
  else
  {
    ++stats_.long_breaks_triggered;
  }
}

void TestHarness::OnBreakEnd(const BreakInfo& /*info*/)
{
  state_machine_->ProcessEvent(BreakCompletedEvent{});
  break_in_progress_ = false;
}

void TestHarness::OnWarning(BreakType /*type*/, Duration /*time_until*/)
{
  if (!current_config_.notification.enabled)
  {
    return;
  }

  // Check DND for notifications
  if (current_config_.notification.respect_dnd && dnd_detector_->IsDndActive())
  {
    return;
  }

  notification_manager_->Show("Break Coming", "A break is about to start");
  ++stats_.notifications_shown;
}

void TestHarness::CheckIdleState()
{
  // Handled by idle detector callbacks
}

void TestHarness::CheckDndState()
{
  // Handled by dnd detector and checked during break start
}

void TestHarness::SimulateIdle(DurationMs idle_duration)
{
  idle_detector_->SimulateIdle(idle_duration);
}

void TestHarness::SimulateActivity()
{
  idle_detector_->SimulateActivity();
}

void TestHarness::SimulateSkip()
{
  if (break_in_progress_ && current_break_info_.can_skip)
  {
    state_machine_->ProcessEvent(SkipEvent{});
    scheduler_->SkipBreak();
    break_in_progress_ = false;
    ++stats_.breaks_skipped;
  }
}

void TestHarness::SimulateSnooze()
{
  if (break_in_progress_ && current_break_info_.can_snooze)
  {
    state_machine_->ProcessEvent(SnoozeEvent{current_break_info_.snooze_duration});
    scheduler_->SnoozeBreak();
    break_in_progress_ = false;
    ++stats_.breaks_snoozed;
  }
}

void TestHarness::SimulateNotificationClick()
{
  notification_manager_->TriggerAction(platform::NotificationAction::Clicked);
}

void TestHarness::SimulateNotificationAction(platform::NotificationAction action)
{
  notification_manager_->TriggerAction(action);

  // Handle actions
  if (action == platform::NotificationAction::SkipBreak)
  {
    scheduler_->SkipBreak();
    ++stats_.breaks_skipped;
  }
  else if (action == platform::NotificationAction::SnoozeBreak)
  {
    scheduler_->SnoozeBreak();
    ++stats_.breaks_snoozed;
  }
}

void TestHarness::SetDndActive(bool active)
{
  if (active)
  {
    dnd_detector_->EnableDnd();
  }
  else
  {
    dnd_detector_->DisableDnd();
  }
}

void TestHarness::SetDndState(platform::DndState state)
{
  dnd_detector_->SetState(state);
}

void TestHarness::SetFullScreenActive(bool active)
{
  dnd_detector_->SetFullScreen(active);
}

void TestHarness::SimulateMonitorChange(std::vector<platform::MonitorInfo> monitors)
{
  monitor_manager_->SetMonitors(std::move(monitors));
}

State TestHarness::GetCurrentState() const
{
  return state_machine_->GetCurrentState();
}

bool TestHarness::IsBreakActive() const
{
  return break_in_progress_;
}

bool TestHarness::IsPaused() const
{
  return state_machine_->GetCurrentState() == State::kPaused;
}

std::optional<Duration> TestHarness::GetTimeUntilBreak() const
{
  return scheduler_->GetTimeUntilNextBreak();
}

BreakType TestHarness::GetNextBreakType() const
{
  return scheduler_->GetNextBreakType();
}

void TestHarness::UpdateConfig(const AppConfig& config)
{
  current_config_ = config;
  reset_short_on_idle_triggered_ = false;
  reset_long_on_idle_triggered_ = false;

  // Update scheduler
  scheduler_->UpdateConfig(config.short_break, config.long_break, config.overlay);

  // Update idle threshold
  idle_detector_->SetIdleThreshold(
      std::chrono::duration_cast<std::chrono::seconds>(config.idle.threshold));

  // Update warning callback with new timing
  scheduler_->SetOnWarning([this](BreakType type, Duration time_until)
                           { OnWarning(type, time_until); }, config.notification.warning_time);
}

}  // namespace blinkbreak::testing
