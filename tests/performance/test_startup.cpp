/// @file test_startup.cpp
/// @brief Startup time and initialization performance tests for BlinkBreak.

#include <chrono>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "core/break_scheduler.hpp"
#include "core/config_manager.hpp"
#include "core/message_provider.hpp"
#include "core/state_machine.hpp"
#include "core/timer.hpp"

using namespace blinkbreak;
using namespace std::chrono_literals;

class StartupTest : public ::testing::Test {
protected:
    void RecordTiming(const std::string& name, std::chrono::microseconds duration) {
        RecordProperty(name + "_Microseconds", static_cast<int64_t>(duration.count()));
        RecordProperty(name + "_Milliseconds", static_cast<int64_t>(duration.count() / 1000));
    }
};

// Test: Measure core component initialization time
TEST_F(StartupTest, CoreComponentInitialization) {
    auto start = std::chrono::high_resolution_clock::now();

    // Initialize state machine
    auto sm_start = std::chrono::high_resolution_clock::now();
    StateMachine state_machine;
    state_machine.SetOnStateChange([](State, State, const Event&) {});
    auto sm_end = std::chrono::high_resolution_clock::now();

    // Initialize break scheduler
    auto sched_start = std::chrono::high_resolution_clock::now();
    BreakConfig short_break;
    short_break.interval = Duration(1200);
    short_break.duration = Duration(20);
    short_break.enabled = true;

    BreakConfig long_break;
    long_break.interval = Duration(3600);
    long_break.duration = Duration(300);
    long_break.enabled = true;

    OverlayConfig overlay;

    BreakScheduler scheduler(short_break, long_break, overlay);
    scheduler.SetOnBreakStart([](const BreakInfo&) {});
    scheduler.SetOnBreakEnd([](const BreakInfo&) {});
    scheduler.SetOnWarning([](BreakType, Duration) {}, Duration(30));  // Requires warning_time
    auto sched_end = std::chrono::high_resolution_clock::now();

    // Initialize message provider
    auto msg_start = std::chrono::high_resolution_clock::now();
    std::vector<std::string> messages = {
        "Take a break!",
        "Rest your eyes!",
        "Look away from the screen!",
    };
    MessageProvider msg_provider(messages);
    auto msg_end = std::chrono::high_resolution_clock::now();

    auto end = std::chrono::high_resolution_clock::now();

    auto sm_duration = std::chrono::duration_cast<std::chrono::microseconds>(sm_end - sm_start);
    auto sched_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(sched_end - sched_start);
    auto msg_duration = std::chrono::duration_cast<std::chrono::microseconds>(msg_end - msg_start);
    auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    RecordTiming("StateMachineInit", sm_duration);
    RecordTiming("SchedulerInit", sched_duration);
    RecordTiming("MessageProviderInit", msg_duration);
    RecordTiming("TotalCoreInit", total_duration);

    // Core initialization should be very fast (< 10ms)
    EXPECT_LT(total_duration.count(), 10000)
        << "Core initialization took " << total_duration.count() << " us";
}

// Test: Configuration loading time
TEST_F(StartupTest, ConfigurationLoading) {
    // Create a temporary config file using correct Glaze JSON field names
    std::string json = R"({
        "short_break": {
            "interval_seconds": 1200,
            "duration_seconds": 20,
            "enabled": true,
            "messages": [
                "Take a break and look away from the screen.",
                "Rest your eyes by looking at something 20 feet away.",
                "Blink several times to refresh your eyes."
            ],
            "rotate_messages": true
        },
        "long_break": {
            "interval_seconds": 3600,
            "duration_seconds": 300,
            "enabled": true,
            "messages": ["Time for a longer break!"],
            "rotate_messages": true
        },
        "auto_start": true,
        "idle": {
            "enabled": true,
            "threshold_seconds": 300,
            "pause_on_idle": true,
            "reset_on_idle": true,
            "reset_threshold_seconds": 1200,
            "show_timer": false
        },
        "notification": {
            "enabled": true,
            "warning_seconds": 30,
            "respect_dnd": true,
            "respect_fullscreen": false
        },
        "overlay": {
            "opacity": 0.9,
            "show_on_all_monitors": true,
            "allow_skip": true,
            "allow_snooze": true,
            "snooze_duration_seconds": 300
        },
        "theme": {
            "follow_system": true,
            "dark_mode": false
        },
        "start_minimized": false
    })";

    auto temp_path = std::filesystem::temp_directory_path() / "blinkbreak_perf_test_config.json";

    // Write config file
    {
        std::ofstream file(temp_path);
        ASSERT_TRUE(file.is_open());
        file << json;
    }

    // Measure load time
    ConfigManager config_manager;  // Non-static method requires instance
    auto start = std::chrono::high_resolution_clock::now();
    auto result = config_manager.Load(temp_path);
    auto end = std::chrono::high_resolution_clock::now();

    ASSERT_TRUE(result.has_value()) << "Config load failed: " << result.error().message;

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    RecordTiming("ConfigLoad", duration);

    // Config loading should be fast (< 50ms including file I/O)
    EXPECT_LT(duration.count(), 50000) << "Config loading took " << duration.count() << " us";

    // Cleanup
    std::filesystem::remove(temp_path);
}

// Test: Configuration parsing time (no file I/O)
TEST_F(StartupTest, ConfigurationParsingOnly) {
    // Use correct Glaze JSON field names
    std::string json = R"({
        "short_break": {"interval_seconds": 1200, "duration_seconds": 20, "enabled": true},
        "long_break": {"interval_seconds": 3600, "duration_seconds": 300, "enabled": true},
        "auto_start": true
    })";

    ConfigManager config_manager;  // Non-static method requires instance
    auto start = std::chrono::high_resolution_clock::now();
    auto result = config_manager.ParseJson(json);
    auto end = std::chrono::high_resolution_clock::now();

    ASSERT_TRUE(result.has_value());

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    RecordTiming("ConfigParse", duration);

    // Parsing alone should be very fast (< 5ms)
    EXPECT_LT(duration.count(), 5000) << "Config parsing took " << duration.count() << " us";
}

// Test: Default configuration creation time
TEST_F(StartupTest, DefaultConfigCreation) {
    auto start = std::chrono::high_resolution_clock::now();
    AppConfig config;  // Default constructor sets all defaults
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    RecordTiming("DefaultConfigCreation", duration);

    // Default config creation should be instant (< 1ms)
    EXPECT_LT(duration.count(), 1000)
        << "Default config creation took " << duration.count() << " us";

    // Verify config has expected default values
    // Note: AppConfig default constructor sets Duration::zero() for interval/duration,
    // and enabled = true for short_break. These are struct defaults, not "sensible" defaults.
    // Sensible defaults are typically loaded from a config file or set explicitly.
    EXPECT_TRUE(config.short_break.enabled);
    // Default interval is zero (must be set by app or loaded from config)
    EXPECT_EQ(config.short_break.interval.count(), 0);
}

// Test: Scheduler start time
TEST_F(StartupTest, SchedulerStartTime) {
    BreakConfig short_break;
    short_break.interval = Duration(1200);
    short_break.duration = Duration(20);
    short_break.enabled = true;

    BreakConfig long_break;
    long_break.interval = Duration(3600);
    long_break.duration = Duration(300);
    long_break.enabled = true;

    OverlayConfig overlay;

    BreakScheduler scheduler(short_break, long_break, overlay);
    scheduler.SetOnBreakStart([](const BreakInfo&) {});
    scheduler.SetOnBreakEnd([](const BreakInfo&) {});

    auto start = std::chrono::high_resolution_clock::now();
    scheduler.Start();
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    RecordTiming("SchedulerStart", duration);

    // Scheduler start should be instant (< 1ms)
    EXPECT_LT(duration.count(), 1000) << "Scheduler start took " << duration.count() << " us";

    scheduler.Pause();  // BreakScheduler doesn't have Stop(), use Pause()
}

// Test: Multiple component initialization sequence
TEST_F(StartupTest, FullInitializationSequence) {
    auto total_start = std::chrono::high_resolution_clock::now();

    // 1. Create default config
    AppConfig config;

    // 2. Initialize state machine
    StateMachine state_machine;
    state_machine.SetOnStateChange([](State, State, const Event&) {});

    // 3. Initialize scheduler
    BreakScheduler scheduler(config.short_break, config.long_break, config.overlay);
    scheduler.SetOnBreakStart([](const BreakInfo&) {});
    scheduler.SetOnBreakEnd([](const BreakInfo&) {});
    scheduler.SetOnWarning([](BreakType, Duration) {}, Duration(30));  // Requires warning_time

    // 4. Initialize message providers
    MessageProvider short_msg_provider(config.short_break.messages);
    MessageProvider long_msg_provider(config.long_break.messages);

    // 5. Start scheduler
    scheduler.Start();

    auto total_end = std::chrono::high_resolution_clock::now();

    auto total_duration =
        std::chrono::duration_cast<std::chrono::microseconds>(total_end - total_start);
    RecordTiming("FullInitSequence", total_duration);

    // Full initialization should be fast (< 50ms)
    EXPECT_LT(total_duration.count(), 50000)
        << "Full initialization took " << total_duration.count() << " us";

    scheduler.Pause();  // BreakScheduler doesn't have Stop(), use Pause()
}

// Test: Timer creation and start time
TEST_F(StartupTest, TimerCreationAndStart) {
    auto start = std::chrono::high_resolution_clock::now();

    Timer timer(Duration(1200));
    timer.SetOnExpired([]() {});       // Correct method name
    timer.SetOnTick([](Duration) {});  // Correct method name (takes Duration)
    timer.Start();

    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    RecordTiming("TimerCreateAndStart", duration);

    // Timer creation and start should be instant (< 1ms)
    EXPECT_LT(duration.count(), 1000)
        << "Timer creation and start took " << duration.count() << " us";
}

// Test: State machine initialization with all possible callbacks
TEST_F(StartupTest, StateMachineFullSetup) {
    auto start = std::chrono::high_resolution_clock::now();

    StateMachine sm;

    // Register callback
    sm.SetOnStateChange([](State from, State to, const Event& event) {
        (void)from;
        (void)to;
        (void)event;
    });

    // Pre-check all possible transitions
    (void)sm.CanTransition(StartEvent{});
    (void)sm.CanTransition(PauseEvent{});
    (void)sm.CanTransition(ResumeEvent{});
    (void)sm.CanTransition(ResetEvent{});
    (void)sm.CanTransition(TimerExpiredEvent{BreakType::kShort});
    (void)sm.CanTransition(BreakCompletedEvent{});
    (void)sm.CanTransition(SkipEvent{});
    (void)sm.CanTransition(SnoozeEvent{Duration(300)});

    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    RecordTiming("StateMachineFullSetup", duration);

    // Full state machine setup should be instant (< 1ms)
    EXPECT_LT(duration.count(), 1000)
        << "State machine full setup took " << duration.count() << " us";
}
