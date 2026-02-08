/// @file timer.hpp
/// @brief Timer class for tracking break intervals.

#ifndef BLINKBREAK_CORE_TIMER_HPP
#define BLINKBREAK_CORE_TIMER_HPP

#include <blinkbreak/types.hpp>

#include <atomic>
#include <chrono>
#include <functional>

namespace blinkbreak {

/// @brief A countdown timer for tracking break intervals.
///
/// The Timer class provides functionality to count down from a specified
/// duration and notify when the timer expires. It is designed to be
/// thread-safe and can be used from multiple threads.
///
/// @code
/// Timer timer(std::chrono::seconds(60));
/// timer.SetOnExpired([]() { std::cout << "Timer expired!" << std::endl; });
/// timer.Start();
/// @endcode
class Timer {
public:
    /// @brief Callback type for timer expiration.
    using ExpiredCallback = std::function<void()>;

    /// @brief Callback type for timer tick updates.
    /// @param remaining The remaining duration on the timer.
    using TickCallback = std::function<void(Duration remaining)>;

    /// @brief Constructs a timer with the specified duration.
    /// @param duration The countdown duration.
    explicit Timer(Duration duration);

    /// @brief Destructor.
    ~Timer();

    // Non-copyable
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    // Movable
    Timer(Timer&&) noexcept;
    Timer& operator=(Timer&&) noexcept;

    /// @brief Starts or resumes the timer countdown.
    void Start();

    /// @brief Pauses the timer countdown.
    void Pause();

    /// @brief Resets the timer to its initial duration.
    void Reset();

    /// @brief Checks if the timer is currently running.
    /// @return True if the timer is running, false otherwise.
    [[nodiscard]] bool IsRunning() const;

    /// @brief Gets the remaining time on the timer.
    /// @return The remaining duration.
    [[nodiscard]] Duration GetRemaining() const;

    /// @brief Gets the total duration of the timer.
    /// @return The total duration.
    [[nodiscard]] Duration GetTotalDuration() const;

    /// @brief Sets the callback to be called when the timer expires.
    /// @param callback The callback function.
    void SetOnExpired(ExpiredCallback callback);

    /// @brief Sets the callback to be called on each timer tick.
    /// @param callback The callback function.
    void SetOnTick(TickCallback callback);

    /// @brief Updates the timer state. Call this periodically.
    /// @param delta_time The time elapsed since the last update.
    void Update(DurationMs delta_time);

private:
    Duration total_duration_;       ///< The total countdown duration.
    Duration remaining_;            ///< The remaining time.
    DurationMs remainder_ms_;       ///< Accumulated sub-second time.
    std::atomic<bool> is_running_;  ///< Whether the timer is currently running.
    ExpiredCallback on_expired_;    ///< Callback for timer expiration.
    TickCallback on_tick_;          ///< Callback for timer ticks.
};

}  // namespace blinkbreak

#endif  // BLINKBREAK_CORE_TIMER_HPP