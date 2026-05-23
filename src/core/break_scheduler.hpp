/// @file break_scheduler.hpp
/// @brief Break scheduling logic.

#ifndef BLINKBREAK_CORE_BREAK_SCHEDULER_HPP
#define BLINKBREAK_CORE_BREAK_SCHEDULER_HPP

#include "config_types.hpp"
#include "message_provider.hpp"
#include "timer.hpp"

#include <blinkbreak/types.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace blinkbreak {

/// @brief Information about an upcoming or active break.
struct BreakInfo {
    BreakType type;           ///< Type of break.
    Duration duration;        ///< Duration of the break.
    std::string message;      ///< Message to display.
    bool can_skip;            ///< Whether the break can be skipped.
    bool can_snooze;          ///< Whether the break can be snoozed.
    Duration snooze_duration; ///< Duration of snooze if allowed.
};

/// @brief Callback type for break events.
/// @param info Information about the break.
using BreakCallback = std::function<void(const BreakInfo& info)>;

/// @brief Callback type for warning events.
/// @param type Type of upcoming break.
/// @param time_until Time until the break starts.
using WarningCallback = std::function<void(BreakType type, Duration time_until)>;

/// @brief Schedules and manages short and long breaks.
///
/// The BreakScheduler coordinates the timing of short and long breaks,
/// ensuring they don't overlap and providing callbacks for break events.
///
/// @code
/// BreakScheduler scheduler(config.short_break, config.long_break, config.overlay);
/// scheduler.SetOnBreakStart([](const BreakInfo& info) { ... });
/// scheduler.Start();
/// @endcode
class BreakScheduler {
public:
    /// @brief Constructs a break scheduler.
    /// @param short_break Short break configuration.
    /// @param long_break Long break configuration.
    /// @param overlay Overlay configuration (for skip/snooze settings).
    BreakScheduler(const BreakConfig& short_break,
                   const BreakConfig& long_break,
                   const OverlayConfig& overlay);

    /// @brief Destructor.
    ~BreakScheduler();

    // Non-copyable, non-movable
    BreakScheduler(const BreakScheduler&) = delete;
    BreakScheduler& operator=(const BreakScheduler&) = delete;
    BreakScheduler(BreakScheduler&&) = delete;
    BreakScheduler& operator=(BreakScheduler&&) = delete;

    /// @brief Starts the break scheduler.
    void Start();

    /// @brief Pauses the break scheduler.
    void Pause();

    /// @brief Resumes the break scheduler.
    void Resume();

    /// @brief Resets all timers to their initial values.
    void Reset();

    /// @brief Skips the current or next break.
    void SkipBreak();

    /// @brief Resets all timers without triggering OnBreakEnd callback.
    /// Used for DND suppression when a break should be silently skipped.
    void ResetTimers();

    /// @brief Resets the short break timer without triggering OnBreakEnd callback.
    void ResetShortTimer();

    /// @brief Resets the long break timer without triggering OnBreakEnd callback.
    void ResetLongTimer();

    /// @brief Snoozes the current break.
    /// @param duration Optional custom snooze duration.
    void SnoozeBreak(std::optional<Duration> duration = std::nullopt);

    /// @brief Completes the current break and restarts timers.
    void CompleteBreak();

    /// @brief Updates the scheduler. Call this periodically.
    /// @param delta_time Time elapsed since last update.
    void Update(DurationMs delta_time);

    /// @brief Gets the time until the next break.
    /// @return Time until next break, or nullopt if not running.
    [[nodiscard]] std::optional<Duration> GetTimeUntilNextBreak() const;

    /// @brief Gets the time until the next short break.
    /// @return Time until the next short break, or nullopt if not running.
    [[nodiscard]] std::optional<Duration> GetTimeUntilShortBreak() const;

    /// @brief Gets the time until the next long break.
    /// @return Time until the next long break, or nullopt if not running.
    [[nodiscard]] std::optional<Duration> GetTimeUntilLongBreak() const;

    /// @brief Gets the effective total interval for short breaks (includes snooze extensions).
    /// @return Total short-break interval.
    [[nodiscard]] Duration GetShortIntervalTotal() const;

    /// @brief Gets the effective total interval for long breaks (includes snooze extensions).
    /// @return Total long-break interval.
    [[nodiscard]] Duration GetLongIntervalTotal() const;

    /// @brief Gets the remaining time on the active break.
    /// @return Remaining break time, or nullopt if no break is active.
    [[nodiscard]] std::optional<Duration> GetTimeUntilBreakEnds() const;

    /// @brief Gets the type of the next break.
    /// @return The type of the next scheduled break.
    [[nodiscard]] BreakType GetNextBreakType() const;

    /// @brief Gets the upcoming message for the specified break type.
    /// @param type The break type.
    /// @return The next message without advancing the rotation.
    [[nodiscard]] std::string GetUpcomingMessage(BreakType type) const;

    /// @brief Checks if a break is currently active.
    /// @return True if a break is active.
    [[nodiscard]] bool IsBreakActive() const;

    /// @brief Checks if the scheduler is running.
    /// @return True if running.
    [[nodiscard]] bool IsRunning() const;

    /// @brief Sets the callback for break start events.
    /// @param callback The callback function.
    void SetOnBreakStart(BreakCallback callback);

    /// @brief Sets the callback for break end events.
    /// @param callback The callback function.
    void SetOnBreakEnd(BreakCallback callback);

    /// @brief Sets the callback for warning events.
    /// @param callback The callback function.
    /// @param warning_time Time before break to trigger warning.
    void SetOnWarning(WarningCallback callback, Duration warning_time);

    /// @brief Updates configuration.
    /// @param short_break New short break configuration.
    /// @param long_break New long break configuration.
    /// @param overlay New overlay configuration.
    void UpdateConfig(const BreakConfig& short_break,
                      const BreakConfig& long_break,
                      const OverlayConfig& overlay);

private:
    /// @brief Triggers a break of the specified type.
    /// @param type The type of break to trigger.
    void TriggerBreak(BreakType type);

    /// @brief Checks and triggers warnings if needed.
    void CheckWarnings();

    std::unique_ptr<Timer> short_timer_;   ///< Timer for short breaks.
    std::unique_ptr<Timer> long_timer_;    ///< Timer for long breaks.
    std::unique_ptr<Timer> break_timer_;   ///< Timer for break duration.
    std::unique_ptr<Timer> snooze_timer_;  ///< Timer for snooze duration.

    std::unique_ptr<MessageProvider> short_messages_;  ///< Short break messages.
    std::unique_ptr<MessageProvider> long_messages_;   ///< Long break messages.

    BreakConfig short_config_;     ///< Short break configuration.
    BreakConfig long_config_;      ///< Long break configuration.
    OverlayConfig overlay_config_; ///< Overlay configuration.

    Duration short_interval_total_;  ///< Short-break interval with snooze extensions.
    Duration long_interval_total_;   ///< Long-break interval with snooze extensions.

    bool is_running_;             ///< Whether scheduler is running.
    bool break_active_;           ///< Whether a break is currently active.
    BreakType active_break_type_; ///< Type of currently active break.

    BreakCallback on_break_start_;  ///< Break start callback.
    BreakCallback on_break_end_;    ///< Break end callback.
    WarningCallback on_warning_;    ///< Warning callback.
    Duration warning_time_;         ///< Time before break for warning.
    bool warning_sent_;             ///< Whether warning was sent for current cycle.
};

}  // namespace blinkbreak

#endif  // BLINKBREAK_CORE_BREAK_SCHEDULER_HPP
