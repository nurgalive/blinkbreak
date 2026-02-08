/// @file timer.cpp
/// @brief Implementation of the Timer class.

#include "timer.hpp"

#include <spdlog/spdlog.h>

namespace blinkbreak {

Timer::Timer(Duration duration)
    : total_duration_(duration),
      remaining_(duration),
      remainder_ms_(DurationMs::zero()),
      is_running_(false),
      on_expired_(nullptr),
      on_tick_(nullptr) {
    spdlog::debug("Timer created with duration: {}s", duration.count());
}

Timer::~Timer() = default;

Timer::Timer(Timer&& other) noexcept
    : total_duration_(other.total_duration_),
      remaining_(other.remaining_),
      remainder_ms_(other.remainder_ms_),
      is_running_(other.is_running_.load()),
      on_expired_(std::move(other.on_expired_)),
      on_tick_(std::move(other.on_tick_)) {}

Timer& Timer::operator=(Timer&& other) noexcept {
    if (this != &other) {
        total_duration_ = other.total_duration_;
        remaining_ = other.remaining_;
        remainder_ms_ = other.remainder_ms_;
        is_running_.store(other.is_running_.load());
        on_expired_ = std::move(other.on_expired_);
        on_tick_ = std::move(other.on_tick_);
    }
    return *this;
}

void Timer::Start() {
    is_running_.store(true);
    spdlog::debug("Timer started");
}

void Timer::Pause() {
    is_running_.store(false);
    spdlog::debug("Timer paused with {}s remaining", remaining_.count());
}

void Timer::Reset() {
    remaining_ = total_duration_;
    remainder_ms_ = DurationMs::zero();
    is_running_.store(false);
    spdlog::debug("Timer reset to {}s", total_duration_.count());
}

bool Timer::IsRunning() const {
    return is_running_.load();
}

Duration Timer::GetRemaining() const {
    return remaining_;
}

Duration Timer::GetTotalDuration() const {
    return total_duration_;
}

void Timer::SetOnExpired(ExpiredCallback callback) {
    on_expired_ = std::move(callback);
}

void Timer::SetOnTick(TickCallback callback) {
    on_tick_ = std::move(callback);
}

void Timer::Update(DurationMs delta_time) {
    if (!is_running_.load()) {
        return;
    }

    remainder_ms_ += delta_time;
    auto delta_seconds = std::chrono::duration_cast<Duration>(remainder_ms_);
    if (delta_seconds.count() == 0) {
        return;
    }
    remainder_ms_ -= std::chrono::duration_cast<DurationMs>(delta_seconds);

    if (delta_seconds >= remaining_) {
        remaining_ = Duration::zero();
        remainder_ms_ = DurationMs::zero();
        is_running_.store(false);
        spdlog::debug("Timer expired");
        if (on_expired_) {
            on_expired_();
        }
    } else {
        remaining_ -= delta_seconds;
        if (on_tick_) {
            on_tick_(remaining_);
        }
    }
}

}  // namespace blinkbreak