/// @file idle_detector_win.cpp
/// @brief Windows-specific idle detection implementation.

#include "idle_detector_win.hpp"

#include <spdlog/spdlog.h>

// Windows headers
#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace blinkbreak {
namespace platform {

using namespace std::chrono_literals;

IdleDetectorWin::IdleDetectorWin()
    : running_(false), is_idle_(false), threshold_(300s) {  // Default 5 minutes
    spdlog::debug("IdleDetectorWin created with threshold={}s", threshold_.count());
}

IdleDetectorWin::~IdleDetectorWin() {
    Stop();
    spdlog::debug("IdleDetectorWin destroyed");
}

void IdleDetectorWin::Start() {
    if (running_.load()) {
        spdlog::warn("IdleDetectorWin::Start called but already running");
        return;
    }

    spdlog::info("IdleDetectorWin starting monitoring");
    running_.store(true);
    is_idle_.store(false);
    monitor_thread_ = std::thread(&IdleDetectorWin::MonitorThreadFunc, this);
}

void IdleDetectorWin::Stop() {
    if (!running_.load()) {
        return;
    }

    spdlog::info("IdleDetectorWin stopping monitoring");
    running_.store(false);

    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

bool IdleDetectorWin::IsRunning() const {
    return running_.load();
}

std::chrono::milliseconds IdleDetectorWin::GetIdleTime() const {
    LASTINPUTINFO lii{};
    lii.cbSize = sizeof(LASTINPUTINFO);

    if (!GetLastInputInfo(&lii)) {
        spdlog::warn("GetLastInputInfo failed, error={}", GetLastError());
        return std::chrono::milliseconds(0);
    }

    // GetTickCount returns milliseconds since system start
    // LASTINPUTINFO.dwTime is the tick count at last input
    DWORD current_tick = GetTickCount();
    DWORD idle_tick = current_tick - lii.dwTime;

    return std::chrono::milliseconds(idle_tick);
}

bool IdleDetectorWin::IsIdle() const {
    return is_idle_.load();
}

void IdleDetectorWin::SetIdleThreshold(std::chrono::seconds threshold) {
    std::lock_guard lock(mutex_);
    threshold_ = threshold;
    spdlog::debug("IdleDetectorWin threshold set to {}s", threshold.count());
}

std::chrono::seconds IdleDetectorWin::GetIdleThreshold() const {
    std::lock_guard lock(mutex_);
    return threshold_;
}

void IdleDetectorWin::SetOnIdle(std::function<void()> callback) {
    std::lock_guard lock(mutex_);
    on_idle_ = std::move(callback);
}

void IdleDetectorWin::SetOnActive(std::function<void()> callback) {
    std::lock_guard lock(mutex_);
    on_active_ = std::move(callback);
}

void IdleDetectorWin::MonitorThreadFunc() {
    spdlog::debug("Idle monitor thread started");

    constexpr auto kPollInterval = 500ms;

    while (running_.load()) {
        const auto idle_time = GetIdleTime();
        std::chrono::seconds threshold;
        std::function<void()> on_idle;
        std::function<void()> on_active;

        {
            std::lock_guard lock(mutex_);
            threshold = threshold_;
            on_idle = on_idle_;
            on_active = on_active_;
        }

        const bool was_idle = is_idle_.load();
        const bool now_idle = idle_time >= threshold;

        if (now_idle && !was_idle) {
            // Transition to idle
            spdlog::info("User became idle (idle for {}s, threshold={}s)",
                         std::chrono::duration_cast<std::chrono::seconds>(idle_time).count(),
                         threshold.count());
            is_idle_.store(true);
            if (on_idle) {
                on_idle();
            }
        } else if (!now_idle && was_idle) {
            // Transition to active
            spdlog::info("User became active");
            is_idle_.store(false);
            if (on_active) {
                on_active();
            }
        }

        std::this_thread::sleep_for(kPollInterval);
    }

    spdlog::debug("Idle monitor thread stopped");
}

// Factory function implementation
std::unique_ptr<IIdleDetector> CreateIdleDetector() {
    return std::make_unique<IdleDetectorWin>();
}

}  // namespace platform
}  // namespace blinkbreak
