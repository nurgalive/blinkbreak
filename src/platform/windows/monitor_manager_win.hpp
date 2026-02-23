/// @file monitor_manager_win.hpp
/// @brief Windows monitor manager implementation.

#ifndef BLINKBREAK_PLATFORM_WINDOWS_MONITOR_MANAGER_WIN_HPP
#define BLINKBREAK_PLATFORM_WINDOWS_MONITOR_MANAGER_WIN_HPP

#ifdef _WIN32

#include "../monitor_manager.hpp"

#include <Windows.h>

#include <mutex>
#include <vector>

namespace blinkbreak::platform {

/// @brief Windows implementation of the monitor manager.
class MonitorManagerWin final : public IMonitorManager {
public:
    /// @brief Constructs the monitor manager.
    MonitorManagerWin();

    /// @brief Destructor.
    ~MonitorManagerWin() override;

    MonitorManagerWin(const MonitorManagerWin&) = delete;
    MonitorManagerWin& operator=(const MonitorManagerWin&) = delete;
    MonitorManagerWin(MonitorManagerWin&&) = delete;
    MonitorManagerWin& operator=(MonitorManagerWin&&) = delete;

    /// @brief Gets all available monitors.
    std::vector<MonitorInfo> GetMonitors() override;

    /// @brief Gets the primary monitor.
    MonitorInfo GetPrimaryMonitor() override;

    /// @brief Sets a callback for monitor configuration changes.
    void SetOnMonitorChange(std::function<void()> callback) override;

private:
    struct EnumContext {
        std::vector<MonitorInfo> monitors;
        int next_id = 0;
    };

    static BOOL CALLBACK EnumMonitorsProc(HMONITOR monitor, HDC, LPRECT, LPARAM data);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    void RefreshMonitors();
    void StopChangeListener();

    std::mutex mutex_;
    std::vector<MonitorInfo> cached_monitors_;
    std::function<void()> on_change_;
    HWND message_window_;
};

}  // namespace blinkbreak::platform

#endif  // _WIN32

#endif  // BLINKBREAK_PLATFORM_WINDOWS_MONITOR_MANAGER_WIN_HPP