/// @file break_scheduler.cpp
/// @brief Implementation of the BreakScheduler class.

#include "break_scheduler.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>


namespace blinkbreak {

BreakScheduler::BreakScheduler(const BreakConfig& short_break, const BreakConfig& long_break,
                               const OverlayConfig& overlay)
    : short_config_(short_break),
      long_config_(long_break),
      overlay_config_(overlay),
      short_interval_total_(short_break.interval),
      long_interval_total_(long_break.interval),
      is_running_(false),
      break_active_(false),
      active_break_type_(BreakType::kShort),
      warning_time_(Duration::zero()),
      warning_sent_(false) {
    short_timer_ = std::make_unique<Timer>(short_break.interval);
    long_timer_ = std::make_unique<Timer>(long_break.interval);
    break_timer_ = std::make_unique<Timer>(Duration::zero());
    snooze_timer_ = std::make_unique<Timer>(overlay.snooze_duration);

    short_messages_ =
        std::make_unique<MessageProvider>(short_break.messages, short_break.rotate_messages);

    long_messages_ =
        std::make_unique<MessageProvider>(long_break.messages, long_break.rotate_messages);

    spdlog::info(
        "BreakScheduler config: short[enabled={}, interval={}s, duration={}s, messages={}, "
        "rotate={}] "
        "long[enabled={}, interval={}s, duration={}s, messages={}, rotate={}]",
        short_break.enabled, short_break.interval.count(), short_break.duration.count(),
        short_break.messages.size(), short_break.rotate_messages, long_break.enabled,
        long_break.interval.count(), long_break.duration.count(), long_break.messages.size(),
        long_break.rotate_messages);

    short_timer_->SetOnExpired([this]() {
        if (!break_active_ && short_config_.enabled) {
            TriggerBreak(BreakType::kShort);
        }
    });

    long_timer_->SetOnExpired([this]() {
        if (!break_active_ && long_config_.enabled) {
            TriggerBreak(BreakType::kLong);
        }
    });

    break_timer_->SetOnExpired([this]() { CompleteBreak(); });

    snooze_timer_->SetOnExpired([this]() { TriggerBreak(active_break_type_); });

    spdlog::debug("BreakScheduler created");
}

BreakScheduler::~BreakScheduler() = default;

void BreakScheduler::Start() {
    if (is_running_) {
        return;
    }

    is_running_ = true;
    warning_sent_ = false;

    if (short_config_.enabled) {
        short_timer_->Start();
    }
    if (long_config_.enabled) {
        long_timer_->Start();
    }

    spdlog::info("BreakScheduler started");
}

void BreakScheduler::Pause() {
    if (!is_running_) {
        return;
    }

    is_running_ = false;
    short_timer_->Pause();
    long_timer_->Pause();
    break_timer_->Pause();
    snooze_timer_->Pause();

    spdlog::info("BreakScheduler paused");
}

void BreakScheduler::Resume() {
    if (is_running_) {
        return;
    }

    is_running_ = true;

    if (short_config_.enabled && !break_active_) {
        short_timer_->Start();
    }
    if (long_config_.enabled && !break_active_) {
        long_timer_->Start();
    }
    if (break_active_) {
        break_timer_->Start();
    }

    spdlog::info("BreakScheduler resumed");
}

void BreakScheduler::Reset() {
    is_running_ = false;
    break_active_ = false;
    warning_sent_ = false;

    short_timer_->Reset();
    long_timer_->Reset();
    break_timer_->Reset();
    snooze_timer_->Reset();
    short_interval_total_ = short_config_.interval;
    long_interval_total_ = long_config_.interval;

    short_messages_->Reset();
    long_messages_->Reset();

    spdlog::info("BreakScheduler reset");
}

void BreakScheduler::SkipBreak() {
    if (!break_active_) {
        spdlog::debug("No break to skip");
        return;
    }

    break_active_ = false;
    break_timer_->Reset();
    snooze_timer_->Reset();
    warning_sent_ = false;

    if (short_config_.enabled) {
        short_timer_->Reset();
        short_interval_total_ = short_config_.interval;
        short_timer_->Start();
    }
    if (long_config_.enabled) {
        if (active_break_type_ == BreakType::kLong) {
            long_timer_->Reset();
            long_interval_total_ = long_config_.interval;
        }
        long_timer_->Start();
    }

    spdlog::info("Break skipped");

    if (on_break_end_) {
        BreakInfo info{.type = active_break_type_,
                       .duration = Duration::zero(),
                       .message = "",
                       .can_skip = false,
                       .can_snooze = false,
                       .snooze_duration = Duration::zero()};
        on_break_end_(info);
    }
}

void BreakScheduler::ResetTimers() {
    // Reset break-related state without triggering OnBreakEnd callback
    // Used for DND suppression when a break should be silently skipped
    break_active_ = false;
    break_timer_->Reset();
    snooze_timer_->Reset();
    warning_sent_ = false;

    // Reset and restart short timer if enabled
    if (short_config_.enabled) {
        short_timer_->Reset();
        short_interval_total_ = short_config_.interval;
        if (is_running_) {
            short_timer_->Start();
        }
    }

    // Reset and restart long timer if enabled
    if (long_config_.enabled) {
        long_timer_->Reset();
        long_interval_total_ = long_config_.interval;
        if (is_running_) {
            long_timer_->Start();
        }
    }

    spdlog::info("Timers reset (DND suppression)");
    // Note: We intentionally do NOT call on_break_end_ here
}

void BreakScheduler::SnoozeBreak(std::optional<Duration> duration) {
    if (!break_active_) {
        spdlog::debug("No break to snooze");
        return;
    }

    break_active_ = false;
    break_timer_->Pause();
    warning_sent_ = false;

    Duration snooze_dur = duration.value_or(overlay_config_.snooze_duration);
    if (snooze_dur <= Duration::zero()) {
        spdlog::warn("Snooze duration is non-positive; ignoring snooze");
        return;
    }

    snooze_timer_->Reset();

    if (short_config_.enabled) {
        short_timer_->Extend(snooze_dur);
        short_interval_total_ += snooze_dur;
        if (is_running_) {
            short_timer_->Start();
        }
    }
    if (long_config_.enabled) {
        long_timer_->Extend(snooze_dur);
        long_interval_total_ += snooze_dur;
        if (is_running_) {
            long_timer_->Start();
        }
    }

    spdlog::info("Break snoozed for {}s; timers extended", snooze_dur.count());
}

void BreakScheduler::CompleteBreak() {
    if (!break_active_) {
        return;
    }

    break_active_ = false;
    warning_sent_ = false;

    spdlog::info("{} break completed", BreakTypeToString(active_break_type_));

    if (on_break_end_) {
        BreakInfo info{.type = active_break_type_,
                       .duration = active_break_type_ == BreakType::kShort ? short_config_.duration
                                                                           : long_config_.duration,
                       .message = "",
                       .can_skip = false,
                       .can_snooze = false,
                       .snooze_duration = Duration::zero()};
        on_break_end_(info);
    }

    if (short_config_.enabled) {
        short_timer_->Reset();
        short_interval_total_ = short_config_.interval;
        short_timer_->Start();
    }
    if (long_config_.enabled) {
        if (active_break_type_ == BreakType::kLong) {
            long_timer_->Reset();
            long_interval_total_ = long_config_.interval;
        }
        long_timer_->Start();
    }
}

void BreakScheduler::Update(DurationMs delta_time) {
    if (!is_running_) {
        return;
    }

    if (break_active_) {
        break_timer_->Update(delta_time);
    } else if (snooze_timer_->IsRunning()) {
        snooze_timer_->Update(delta_time);
    } else {
        long_timer_->Update(delta_time);
        short_timer_->Update(delta_time);
        CheckWarnings();
    }
}

std::optional<Duration> BreakScheduler::GetTimeUntilNextBreak() const {
    if (!is_running_ || break_active_) {
        return std::nullopt;
    }

    Duration short_remaining =
        short_config_.enabled ? short_timer_->GetRemaining() : Duration::max();
    Duration long_remaining = long_config_.enabled ? long_timer_->GetRemaining() : Duration::max();

    return std::min(short_remaining, long_remaining);
}

std::optional<Duration> BreakScheduler::GetTimeUntilShortBreak() const {
    if (!is_running_ || break_active_ || !short_config_.enabled) {
        return std::nullopt;
    }

    return short_timer_->GetRemaining();
}

std::optional<Duration> BreakScheduler::GetTimeUntilLongBreak() const {
    if (!is_running_ || break_active_ || !long_config_.enabled) {
        return std::nullopt;
    }

    return long_timer_->GetRemaining();
}

Duration BreakScheduler::GetShortIntervalTotal() const {
    return short_interval_total_;
}

Duration BreakScheduler::GetLongIntervalTotal() const {
    return long_interval_total_;
}

std::optional<Duration> BreakScheduler::GetTimeUntilBreakEnds() const {
    if (!break_active_) {
        return std::nullopt;
    }

    return break_timer_->GetRemaining();
}

BreakType BreakScheduler::GetNextBreakType() const {
    if (!short_config_.enabled) {
        return BreakType::kLong;
    }
    if (!long_config_.enabled) {
        return BreakType::kShort;
    }

    return short_timer_->GetRemaining() < long_timer_->GetRemaining() ? BreakType::kShort
                                                                      : BreakType::kLong;
}

std::string BreakScheduler::GetUpcomingMessage(BreakType type) const {
    const MessageProvider* provider =
        type == BreakType::kShort ? short_messages_.get() : long_messages_.get();
    if (!provider) {
        return "Take a break";
    }

    return std::string(provider->GetCurrent());
}

bool BreakScheduler::IsBreakActive() const {
    return break_active_;
}

bool BreakScheduler::IsRunning() const {
    return is_running_;
}

void BreakScheduler::SetOnBreakStart(BreakCallback callback) {
    on_break_start_ = std::move(callback);
}

void BreakScheduler::SetOnBreakEnd(BreakCallback callback) {
    on_break_end_ = std::move(callback);
}

void BreakScheduler::SetOnWarning(WarningCallback callback, Duration warning_time) {
    on_warning_ = std::move(callback);
    warning_time_ = warning_time;
}

void BreakScheduler::UpdateConfig(const BreakConfig& short_break, const BreakConfig& long_break,
                                  const OverlayConfig& overlay) {
    // Determine which timer-relevant settings changed before updating configs.
    const bool short_interval_changed = (short_break.interval != short_config_.interval);
    const bool long_interval_changed = (long_break.interval != long_config_.interval);
    const bool snooze_duration_changed =
        (overlay.snooze_duration != overlay_config_.snooze_duration);
    const bool short_enabled_changed = (short_break.enabled != short_config_.enabled);
    const bool long_enabled_changed = (long_break.enabled != long_config_.enabled);
    const bool timers_need_reset = short_interval_changed || long_interval_changed;

    // Always update all config fields and messages.
    short_config_ = short_break;
    long_config_ = long_break;
    overlay_config_ = overlay;

    short_messages_->SetMessages(short_break.messages);
    short_messages_->SetRotate(short_break.rotate_messages);
    long_messages_->SetMessages(long_break.messages);
    long_messages_->SetRotate(long_break.rotate_messages);

    if (timers_need_reset) {
        // Interval durations changed: recreate timers from scratch.
        const bool was_running = is_running_;

        is_running_ = false;
        break_active_ = false;
        warning_sent_ = false;
        active_break_type_ = BreakType::kShort;

        short_interval_total_ = short_break.interval;
        long_interval_total_ = long_break.interval;

        short_timer_ = std::make_unique<Timer>(short_break.interval);
        long_timer_ = std::make_unique<Timer>(long_break.interval);
        break_timer_ = std::make_unique<Timer>(Duration::zero());
        snooze_timer_ = std::make_unique<Timer>(overlay.snooze_duration);

        short_timer_->SetOnExpired([this]() {
            if (!break_active_ && short_config_.enabled) {
                TriggerBreak(BreakType::kShort);
            }
        });

        long_timer_->SetOnExpired([this]() {
            if (!break_active_ && long_config_.enabled) {
                TriggerBreak(BreakType::kLong);
            }
        });

        break_timer_->SetOnExpired([this]() { CompleteBreak(); });
        snooze_timer_->SetOnExpired([this]() { TriggerBreak(active_break_type_); });

        if (was_running) {
            is_running_ = true;
            if (short_config_.enabled) {
                short_timer_->Start();
            }
            if (long_config_.enabled) {
                long_timer_->Start();
            }
        }

        spdlog::info(
            "BreakScheduler timers reset due to interval change: "
            "short[interval={}s] long[interval={}s]",
            short_break.interval.count(), long_break.interval.count());
    } else if (snooze_duration_changed) {
        // Only snooze duration changed: recreate the snooze timer without
        // disturbing the running short/long break timers.
        snooze_timer_ = std::make_unique<Timer>(overlay.snooze_duration);
        snooze_timer_->SetOnExpired([this]() { TriggerBreak(active_break_type_); });

        spdlog::debug("Snooze timer updated to {}s", overlay.snooze_duration.count());
    }

    if (!timers_need_reset && (short_enabled_changed || long_enabled_changed)) {
        const bool can_start_timers = is_running_ && !break_active_ && !snooze_timer_->IsRunning();

        if (short_enabled_changed) {
            if (short_config_.enabled) {
                short_timer_->Reset();
                short_interval_total_ = short_config_.interval;
                if (can_start_timers) {
                    short_timer_->Start();
                }
            } else {
                short_timer_->Pause();
            }
        }

        if (long_enabled_changed) {
            if (long_config_.enabled) {
                long_timer_->Reset();
                long_interval_total_ = long_config_.interval;
                if (can_start_timers) {
                    long_timer_->Start();
                }
            } else {
                long_timer_->Pause();
            }
        }
    }

    spdlog::info(
        "BreakScheduler configuration updated: short[enabled={}, interval={}s, duration={}s, "
        "messages={}, rotate={}] "
        "long[enabled={}, interval={}s, duration={}s, messages={}, rotate={}]",
        short_break.enabled, short_break.interval.count(), short_break.duration.count(),
        short_break.messages.size(), short_break.rotate_messages, long_break.enabled,
        long_break.interval.count(), long_break.duration.count(), long_break.messages.size(),
        long_break.rotate_messages);
}

void BreakScheduler::TriggerBreak(BreakType type) {
    break_active_ = true;
    active_break_type_ = type;
    warning_sent_ = false;

    const BreakConfig& config = type == BreakType::kShort ? short_config_ : long_config_;
    MessageProvider& messages = type == BreakType::kShort ? *short_messages_ : *long_messages_;

    break_timer_ = std::make_unique<Timer>(config.duration);
    break_timer_->SetOnExpired([this]() { CompleteBreak(); });
    break_timer_->Start();

    BreakInfo info{.type = type,
                   .duration = config.duration,
                   .message = messages.GetNext(),
                   .can_skip = overlay_config_.allow_skip,
                   .can_snooze = overlay_config_.allow_snooze,
                   .snooze_duration = overlay_config_.snooze_duration};

    const auto short_remaining = short_config_.enabled ? short_timer_->GetRemaining().count() : -1;
    const auto long_remaining = long_config_.enabled ? long_timer_->GetRemaining().count() : -1;

    spdlog::info("{} break triggered: '{}' (duration={}s, short_remaining={}s, long_remaining={}s)",
                 BreakTypeToString(type), info.message, config.duration.count(), short_remaining,
                 long_remaining);

    if (on_break_start_) {
        on_break_start_(info);
    }
}

void BreakScheduler::CheckWarnings() {
    if (!on_warning_ || warning_sent_ || warning_time_ == Duration::zero()) {
        return;
    }

    auto time_until = GetTimeUntilNextBreak();
    if (!time_until) {
        return;
    }

    if (*time_until <= warning_time_) {
        warning_sent_ = true;
        on_warning_(GetNextBreakType(), *time_until);
        spdlog::debug("Warning sent: {} break in {}s", BreakTypeToString(GetNextBreakType()),
                      time_until->count());
    }
}

}  // namespace blinkbreak
