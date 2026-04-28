/// @file test_memory.cpp
/// @brief Memory usage profiling tests for BlinkBreak.

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include "core/break_scheduler.hpp"
#include "core/config_manager.hpp"
#include "core/message_provider.hpp"
#include "core/state_machine.hpp"
#include "core/timer.hpp"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#endif

using namespace blinkbreak;
using namespace std::chrono_literals;

namespace {

/// @brief Get current process memory usage in bytes using Windows GlobalMemoryStatusEx.
/// @return Approximate memory usage in bytes, or 0 if unavailable.
/// @note This returns total process memory commit, not precise working set.
size_t GetCurrentMemoryUsage() {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        // Return used virtual memory as a proxy for process memory
        return static_cast<size_t>(memInfo.ullTotalVirtual - memInfo.ullAvailVirtual);
    }
#endif
    return 0;
}

/// @brief Format bytes as human-readable string.
/// @param bytes Number of bytes.
/// @return Formatted string (e.g., "10.5 MB").
std::string FormatBytes(size_t bytes) {
    if (bytes >= 1024ULL * 1024 * 1024) {
        return std::to_string(bytes / (1024ULL * 1024 * 1024)) + " GB";
    } else if (bytes >= 1024 * 1024) {
        return std::to_string(bytes / (1024 * 1024)) + " MB";
    } else if (bytes >= 1024) {
        return std::to_string(bytes / 1024) + " KB";
    }
    return std::to_string(bytes) + " B";
}

}  // namespace

class MemoryTest : public ::testing::Test {
protected:
    void SetUp() override { initial_memory_ = GetCurrentMemoryUsage(); }

    void TearDown() override {
        size_t final_memory = GetCurrentMemoryUsage();
        if (initial_memory_ > 0 && final_memory > 0) {
            size_t growth = (final_memory > initial_memory_) ? (final_memory - initial_memory_) : 0;
            RecordProperty("InitialMemoryBytes", static_cast<int64_t>(initial_memory_));
            RecordProperty("FinalMemoryBytes", static_cast<int64_t>(final_memory));
            RecordProperty("MemoryGrowthBytes", static_cast<int64_t>(growth));
        }
    }

    size_t initial_memory_ = 0;
};

// Test: Timer objects do not leak memory
TEST_F(MemoryTest, TimerMemoryStability) {
    size_t start_memory = GetCurrentMemoryUsage();

    // Create and destroy many timers
    for (int i = 0; i < 10000; ++i) {
        Timer timer(Duration(60));
        timer.Start();
        timer.Update(DurationMs(100));
        timer.Pause();  // Timer doesn't have Stop(), use Pause()
    }

    size_t end_memory = GetCurrentMemoryUsage();

    if (start_memory > 0 && end_memory > 0) {
        size_t growth = (end_memory > start_memory) ? (end_memory - start_memory) : 0;
        // Timer creation/destruction should not accumulate significant memory
        // Allow up to 10 MB growth for allocator overhead (GlobalMemoryStatusEx is less precise)
        EXPECT_LT(growth, 10 * 1024 * 1024)
            << "Memory grew by " << FormatBytes(growth) << " after 10000 timer cycles";
    }
}

// Test: State machine transitions do not leak memory
TEST_F(MemoryTest, StateMachineMemoryStability) {
    size_t start_memory = GetCurrentMemoryUsage();

    // Perform many state machine cycles
    for (int i = 0; i < 10000; ++i) {
        StateMachine sm;
        sm.SetOnStateChange([](State, State, const Event&) {});  // Correct callback signature
        sm.ProcessEvent(StartEvent{});
        sm.ProcessEvent(TimerExpiredEvent{BreakType::kShort});
        sm.ProcessEvent(BreakCompletedEvent{});
        sm.ProcessEvent(blinkbreak::ResetEvent{});
    }

    size_t end_memory = GetCurrentMemoryUsage();

    if (start_memory > 0 && end_memory > 0) {
        size_t growth = (end_memory > start_memory) ? (end_memory - start_memory) : 0;
        EXPECT_LT(growth, 10 * 1024 * 1024)
            << "Memory grew by " << FormatBytes(growth) << " after 10000 state machine cycles";
    }
}

// Test: BreakScheduler does not leak memory during operation
TEST_F(MemoryTest, BreakSchedulerMemoryStability) {
    size_t start_memory = GetCurrentMemoryUsage();

    BreakConfig short_break;
    short_break.interval = Duration(1);  // 1 second
    short_break.duration = Duration(1);
    short_break.enabled = true;

    BreakConfig long_break;
    long_break.interval = Duration(10);
    long_break.duration = Duration(2);
    long_break.enabled = true;

    OverlayConfig overlay;  // Required third parameter

    BreakScheduler scheduler(short_break, long_break, overlay);
    scheduler.SetOnBreakStart([](const BreakInfo&) {});
    scheduler.SetOnBreakEnd([](const BreakInfo&) {});
    scheduler.SetOnWarning([](BreakType, Duration) {}, Duration(5));  // Requires warning_time

    scheduler.Start();

    // Simulate many timer ticks
    for (int i = 0; i < 10000; ++i) {
        scheduler.Update(DurationMs(100));
    }

    scheduler.Pause();  // BreakScheduler doesn't have Stop(), use Pause()

    size_t end_memory = GetCurrentMemoryUsage();

    if (start_memory > 0 && end_memory > 0) {
        size_t growth = (end_memory > start_memory) ? (end_memory - start_memory) : 0;
        EXPECT_LT(growth, 10 * 1024 * 1024)
            << "Memory grew by " << FormatBytes(growth) << " after 10000 scheduler updates";
    }
}

// Test: Configuration parsing does not leak memory
TEST_F(MemoryTest, ConfigParsingMemoryStability) {
    size_t start_memory = GetCurrentMemoryUsage();

    // Use correct Glaze JSON field names: interval_seconds, duration_seconds, etc.
    std::string json = R"({
        "short_break": {
            "interval_seconds": 1200,
            "duration_seconds": 20,
            "enabled": true,
            "messages": ["Take a break!", "Rest your eyes"]
        },
        "long_break": {
            "interval_seconds": 3600,
            "duration_seconds": 300,
            "enabled": true,
            "messages": ["Long break time"]
        },
        "auto_start": true,
        "idle": {
            "enabled": true,
            "threshold_seconds": 300,
            "pause_on_idle": true,
            "reset_on_idle": false
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
        }
    })";

    ConfigManager config_manager;  // Non-static methods require an instance

    // Parse many times
    for (int i = 0; i < 1000; ++i) {
        auto result = config_manager.ParseJson(json);
        ASSERT_TRUE(result.has_value()) << "Parse failed: " << result.error().message;
    }

    size_t end_memory = GetCurrentMemoryUsage();

    if (start_memory > 0 && end_memory > 0) {
        size_t growth = (end_memory > start_memory) ? (end_memory - start_memory) : 0;
        EXPECT_LT(growth, 10 * 1024 * 1024)
            << "Memory grew by " << FormatBytes(growth) << " after 1000 config parses";
    }
}

// Test: JSON serialization does not leak memory
TEST_F(MemoryTest, ConfigSerializationMemoryStability) {
    size_t start_memory = GetCurrentMemoryUsage();

    AppConfig config;
    config.short_break.interval = Duration(1200);
    config.short_break.duration = Duration(20);
    config.short_break.enabled = true;
    config.short_break.messages = {"Message 1", "Message 2", "Message 3"};

    ConfigManager config_manager;  // Non-static methods require an instance

    // Serialize many times
    for (int i = 0; i < 1000; ++i) {
        std::string json = config_manager.ToJson(config);
        ASSERT_FALSE(json.empty());
    }

    size_t end_memory = GetCurrentMemoryUsage();

    if (start_memory > 0 && end_memory > 0) {
        size_t growth = (end_memory > start_memory) ? (end_memory - start_memory) : 0;
        EXPECT_LT(growth, 10 * 1024 * 1024)
            << "Memory grew by " << FormatBytes(growth) << " after 1000 config serializations";
    }
}

// Test: Callback registration/unregistration does not leak
TEST_F(MemoryTest, CallbackMemoryStability) {
    size_t start_memory = GetCurrentMemoryUsage();

    for (int i = 0; i < 10000; ++i) {
        StateMachine sm;

        // Register various callbacks with correct signature
        sm.SetOnStateChange([i](State from, State to, const Event& event) {
            (void)i;
            (void)from;
            (void)to;
            (void)event;
        });

        // Trigger some transitions
        sm.ProcessEvent(StartEvent{});
        sm.ProcessEvent(PauseEvent{});
        sm.ProcessEvent(ResumeEvent{});

        // StateMachine destructor should clean up callbacks
    }

    size_t end_memory = GetCurrentMemoryUsage();

    if (start_memory > 0 && end_memory > 0) {
        size_t growth = (end_memory > start_memory) ? (end_memory - start_memory) : 0;
        EXPECT_LT(growth, 10 * 1024 * 1024)
            << "Memory grew by " << FormatBytes(growth) << " after 10000 callback cycles";
    }
}

// Test: MessageProvider cycling does not leak memory
TEST_F(MemoryTest, MessageProviderMemoryStability) {
    size_t start_memory = GetCurrentMemoryUsage();

    std::vector<std::string> messages = {
        "Take a break and look away from the screen.",
        "Rest your eyes by looking at something 20 feet away.",
        "Blink several times to refresh your eyes.",
        "Stand up and stretch your body.",
        "Take a few deep breaths and relax.",
    };

    for (int i = 0; i < 10000; ++i) {
        MessageProvider provider(messages);
        for (int j = 0; j < 10; ++j) {
            auto msg = provider.GetNext();  // Correct method name
            (void)msg;
        }
    }

    size_t end_memory = GetCurrentMemoryUsage();

    if (start_memory > 0 && end_memory > 0) {
        size_t growth = (end_memory > start_memory) ? (end_memory - start_memory) : 0;
        EXPECT_LT(growth, 10 * 1024 * 1024)
            << "Memory grew by " << FormatBytes(growth) << " after 10000 message provider cycles";
    }
}
