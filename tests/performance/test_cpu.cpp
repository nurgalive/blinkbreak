/// @file test_cpu.cpp
/// @brief CPU usage benchmark tests for BlinkBreak core components.

#include <chrono>
#include <numeric>
#include <vector>

#include <gtest/gtest.h>

#include "core/break_scheduler.hpp"
#include "core/config_manager.hpp"
#include "core/message_provider.hpp"
#include "core/state_machine.hpp"
#include "core/timer.hpp"

using namespace blinkbreak;
using namespace std::chrono_literals;

namespace {

/// @brief Measure execution time of a function.
/// @tparam Func Callable type.
/// @param func Function to measure.
/// @param iterations Number of iterations to run.
/// @return Total execution time in microseconds.
template <typename Func>
std::chrono::microseconds MeasureTime(Func&& func, int iterations = 10000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start);
}

/// @brief Measure execution time with warmup.
/// @tparam Func Callable type.
/// @param func Function to measure.
/// @param iterations Number of iterations to run.
/// @param warmup_iterations Number of warmup iterations.
/// @return Total execution time in microseconds.
template <typename Func>
std::chrono::microseconds MeasureTimeWithWarmup(Func&& func, int iterations = 10000,
                                                int warmup_iterations = 100) {
    // Warmup phase
    for (int i = 0; i < warmup_iterations; ++i) {
        func();
    }

    return MeasureTime(std::forward<Func>(func), iterations);
}

}  // namespace

class CpuBenchmark : public ::testing::Test {
protected:
    void RecordBenchmark(const std::string& name, std::chrono::microseconds duration,
                         int iterations) {
        double per_op_ns = (duration.count() * 1000.0) / iterations;
        RecordProperty(name + "_TotalMicroseconds", static_cast<int64_t>(duration.count()));
        RecordProperty(name + "_PerOpNanoseconds", static_cast<int64_t>(per_op_ns));
        RecordProperty(name + "_Iterations", iterations);
    }
};

// Benchmark: Timer creation and destruction
TEST_F(CpuBenchmark, TimerCreationPerformance) {
    constexpr int kIterations = 100000;

    auto duration = MeasureTimeWithWarmup(
        []() {
            Timer timer(Duration(60));
            (void)timer.IsRunning();
        },
        kIterations);

    // 100000 timer creations should complete in < 500ms
    EXPECT_LT(duration.count(), 500000)
        << "Timer creation took " << duration.count() << " us for " << kIterations << " iterations";

    RecordBenchmark("TimerCreation", duration, kIterations);
}

// Benchmark: Timer update operations
TEST_F(CpuBenchmark, TimerUpdatePerformance) {
    constexpr int kIterations = 100000;

    Timer timer(Duration(3600));  // 1 hour timer
    timer.Start();

    auto duration = MeasureTimeWithWarmup([&]() { timer.Update(DurationMs(100)); }, kIterations);

    // 100000 updates should complete in < 200ms
    EXPECT_LT(duration.count(), 200000)
        << "Timer update took " << duration.count() << " us for " << kIterations << " iterations";

    RecordBenchmark("TimerUpdate", duration, kIterations);
}

// Benchmark: Timer with callback
TEST_F(CpuBenchmark, TimerWithCallbackPerformance) {
    constexpr int kIterations = 100000;

    // Use a very long timer (100000 seconds) so it won't expire during the test.
    // With 100K iterations at 100ms each = 10000 seconds simulated.
    // Timer must be > 10000s to avoid expiration.
    Timer timer(Duration(100000));
    int callback_count = 0;
    // SetOnTick takes Duration (seconds) not DurationMs
    timer.SetOnTick([&callback_count](Duration) { ++callback_count; });
    timer.Start();

    auto duration = MeasureTimeWithWarmup([&]() { timer.Update(DurationMs(100)); }, kIterations);

    // Timer tick callback fires once per accumulated second (10 updates at 100ms = 1 tick).
    // Warmup: 100 iterations = 10 seconds = 10 ticks
    // Main: 100000 iterations = 10000 seconds = 10000 ticks
    // Total: ~10010 ticks
    EXPECT_GE(callback_count, 9500);   // Allow some tolerance
    EXPECT_LE(callback_count, 10500);  // Allow some tolerance

    // Callback overhead should be minimal
    EXPECT_LT(duration.count(), 300000) << "Timer with callback took " << duration.count()
                                        << " us for " << kIterations << " iterations";

    RecordBenchmark("TimerWithCallback", duration, kIterations);
}

// Benchmark: State machine single transition
TEST_F(CpuBenchmark, StateMachineSingleTransitionPerformance) {
    constexpr int kIterations = 100000;

    StateMachine sm;

    auto duration = MeasureTimeWithWarmup(
        [&]() {
            sm.ProcessEvent(StartEvent{});
            sm.ProcessEvent(ResetEvent{});
        },
        kIterations);

    // 100000 transition pairs should complete in < 1 second
    // (Relaxed threshold to account for spdlog logging overhead in state machine)
    EXPECT_LT(duration.count(), 1000000) << "State machine transitions took " << duration.count()
                                         << " us for " << kIterations << " iterations";

    RecordBenchmark("StateMachineSingleTransition", duration, kIterations);
}

// Benchmark: State machine full cycle
TEST_F(CpuBenchmark, StateMachineFullCyclePerformance) {
    constexpr int kIterations = 50000;

    auto duration = MeasureTimeWithWarmup(
        []() {
            StateMachine sm;
            sm.ProcessEvent(StartEvent{});                          // Idle -> Running
            sm.ProcessEvent(TimerExpiredEvent{BreakType::kShort});  // Running -> BreakActive
            sm.ProcessEvent(BreakCompletedEvent{});                 // BreakActive -> Running
            sm.ProcessEvent(PauseEvent{});                          // Running -> Paused
            sm.ProcessEvent(ResumeEvent{});                         // Paused -> Running
            sm.ProcessEvent(ResetEvent{});                          // Running -> Idle
        },
        kIterations);

    // 50000 full cycles should complete in < 2 seconds
    // (Relaxed threshold to account for spdlog logging overhead in state machine)
    EXPECT_LT(duration.count(), 2000000) << "State machine full cycles took " << duration.count()
                                         << " us for " << kIterations << " iterations";

    RecordBenchmark("StateMachineFullCycle", duration, kIterations);
}

// Benchmark: State machine with callback
TEST_F(CpuBenchmark, StateMachineWithCallbackPerformance) {
    constexpr int kIterations = 50000;

    int callback_count = 0;

    auto duration = MeasureTimeWithWarmup(
        [&]() {
            StateMachine sm;
            sm.SetOnStateChange([&](State, State, const Event&) { ++callback_count; });
            sm.ProcessEvent(StartEvent{});
            sm.ProcessEvent(ResetEvent{});
        },
        kIterations);

    // Callback overhead should be minimal (relaxed threshold due to spdlog logging in transitions)
    EXPECT_LT(duration.count(), 1000000) << "State machine with callback took " << duration.count()
                                         << " us for " << kIterations << " iterations";

    RecordBenchmark("StateMachineWithCallback", duration, kIterations);
}

// Benchmark: BreakScheduler update
TEST_F(CpuBenchmark, BreakSchedulerUpdatePerformance) {
    constexpr int kIterations = 100000;

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
    scheduler.Start();

    auto duration =
        MeasureTimeWithWarmup([&]() { scheduler.Update(DurationMs(100)); }, kIterations);

    // 100000 updates should complete in < 500ms
    EXPECT_LT(duration.count(), 500000) << "Scheduler update took " << duration.count()
                                        << " us for " << kIterations << " iterations";

    RecordBenchmark("BreakSchedulerUpdate", duration, kIterations);
}

// Benchmark: Configuration parsing
TEST_F(CpuBenchmark, ConfigParsingPerformance) {
    constexpr int kIterations = 10000;

    // Use correct Glaze JSON field names: interval_seconds, duration_seconds, etc.
    std::string json = R"({
        "short_break": {"interval_seconds": 1200, "duration_seconds": 20, "enabled": true},
        "long_break": {"interval_seconds": 3600, "duration_seconds": 300, "enabled": true},
        "auto_start": true,
        "idle": {"enabled": true, "threshold_seconds": 300, "pause_on_idle": true},
        "notification": {"enabled": true, "warning_seconds": 30},
        "overlay": {"opacity": 0.9, "show_on_all_monitors": true},
        "theme": {"follow_system": true}
    })";

    ConfigManager config_manager;  // Non-static method requires instance

    auto duration = MeasureTimeWithWarmup(
        [&]() {
            auto result = config_manager.ParseJson(json);
            (void)result;
        },
        kIterations);

    // 10000 parses should complete in < 1 second
    EXPECT_LT(duration.count(), 1000000)
        << "Config parsing took " << duration.count() << " us for " << kIterations << " iterations";

    RecordBenchmark("ConfigParsing", duration, kIterations);
}

// Benchmark: Configuration serialization
TEST_F(CpuBenchmark, ConfigSerializationPerformance) {
    constexpr int kIterations = 10000;

    AppConfig config;
    config.short_break.interval = Duration(1200);
    config.short_break.duration = Duration(20);
    config.short_break.enabled = true;
    config.short_break.messages = {"Message 1", "Message 2", "Message 3"};
    config.long_break.interval = Duration(3600);
    config.long_break.duration = Duration(300);
    config.long_break.enabled = true;

    ConfigManager config_manager;  // Non-static method requires instance

    auto duration = MeasureTimeWithWarmup(
        [&]() {
            std::string json = config_manager.ToJson(config);
            (void)json;
        },
        kIterations);

    // 10000 serializations should complete in < 1 second
    EXPECT_LT(duration.count(), 1000000) << "Config serialization took " << duration.count()
                                         << " us for " << kIterations << " iterations";

    RecordBenchmark("ConfigSerialization", duration, kIterations);
}

// Benchmark: MessageProvider message retrieval
TEST_F(CpuBenchmark, MessageProviderPerformance) {
    constexpr int kIterations = 100000;

    std::vector<std::string> messages = {
        "Take a break and look away from the screen.",
        "Rest your eyes by looking at something 20 feet away.",
        "Blink several times to refresh your eyes.",
        "Stand up and stretch your body.",
        "Take a few deep breaths and relax.",
    };

    MessageProvider provider(messages);

    auto duration = MeasureTimeWithWarmup(
        [&]() {
            auto msg = provider.GetNext();  // Correct method name
            (void)msg;
        },
        kIterations);

    // 100000 message retrievals should complete in < 200ms
    EXPECT_LT(duration.count(), 200000) << "Message retrieval took " << duration.count()
                                        << " us for " << kIterations << " iterations";

    RecordBenchmark("MessageRetrieval", duration, kIterations);
}

// Benchmark: CanTransition check
TEST_F(CpuBenchmark, CanTransitionCheckPerformance) {
    constexpr int kIterations = 100000;

    StateMachine sm;

    auto duration = MeasureTimeWithWarmup(
        [&]() {
            (void)sm.CanTransition(StartEvent{});
            (void)sm.CanTransition(PauseEvent{});
            (void)sm.CanTransition(ResetEvent{});
        },
        kIterations);

    // 100000 * 3 checks should complete in < 200ms
    EXPECT_LT(duration.count(), 200000) << "CanTransition checks took " << duration.count()
                                        << " us for " << kIterations << " iterations";

    RecordBenchmark("CanTransitionCheck", duration, kIterations);
}

// Benchmark: Timer remaining time query
TEST_F(CpuBenchmark, TimerRemainingTimePerformance) {
    constexpr int kIterations = 100000;

    Timer timer(Duration(3600));
    timer.Start();

    auto duration = MeasureTimeWithWarmup(
        [&]() {
            auto remaining = timer.GetRemaining();
            (void)remaining;
        },
        kIterations);

    // 100000 queries should complete in < 100ms
    EXPECT_LT(duration.count(), 100000)
        << "GetRemaining took " << duration.count() << " us for " << kIterations << " iterations";

    RecordBenchmark("TimerRemainingTime", duration, kIterations);
}
