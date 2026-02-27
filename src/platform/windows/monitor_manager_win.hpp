/// @file monitor_manager_win.hpp
/// @brief Windows implementation of IMonitorManager using Win32 APIs.

#ifndef BLINKBREAK_PLATFORM_WINDOWS_MONITOR_MANAGER_WIN_HPP
#define BLINKBREAK_PLATFORM_WINDOWS_MONITOR_MANAGER_WIN_HPP

#include <functional>
#include <mutex>
#include <vector>

#include "platform/platform_interface.hpp"


namespace blinkbreak {
namespace platform {

/// @brief Win32 implementation of IMonitorManager.
///
/// Uses EnumDisplayMonitors, GetMonitorInfoW, and EnumDisplaySettingsW
/// to enumerate monitors and query their geometry, DPI, and orientation.
class MonitorManagerWin : public IMonitorManager {
public:
    MonitorManagerWin();
    ~MonitorManagerWin() override = default;

    // Non-copyable, non-movable
    MonitorManagerWin(const MonitorManagerWin&) = delete;
    MonitorManagerWin& operator=(const MonitorManagerWin&) = delete;
    MonitorManagerWin(MonitorManagerWin&&) = delete;
    MonitorManagerWin& operator=(MonitorManagerWin&&) = delete;

    void RefreshMonitors() override;
    [[nodiscard]] std::vector<MonitorInfo> GetMonitors() const override;
    [[nodiscard]] MonitorInfo GetPrimaryMonitor() const override;
    [[nodiscard]] int GetMonitorCount() const override;
    void SetOnMonitorChange(std::function<void()> callback) override;

private:
    /// @brief Data collected during EnumDisplayMonitors callback.
    struct EnumContext {
        std::vector<MonitorInfo> monitors;
        int next_id = 0;
    };

    /// @brief Win32 callback for EnumDisplayMonitors.
    static int __stdcall EnumMonitorCallback(void* hMonitor, void* hdc, void* lprcMonitor,
                                             long long dwData);

    mutable std::mutex mutex_;
    std::vector<MonitorInfo> monitors_;
    std::function<void()> on_change_;
};

}  // namespace platform
}  // namespace blinkbreak

#endif  // BLINKBREAK_PLATFORM_WINDOWS_MONITOR_MANAGER_WIN_HPP
