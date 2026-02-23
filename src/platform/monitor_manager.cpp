/// @file monitor_manager.cpp
/// @brief Monitor manager factory implementation (stub fallback).

#include "monitor_manager.hpp"

#ifdef _WIN32
#include "windows/monitor_manager_win.hpp"
#endif

namespace blinkbreak::platform {
namespace {

class StubMonitorManager final : public IMonitorManager {
public:
    std::vector<MonitorInfo> GetMonitors() override { return {}; }

    MonitorInfo GetPrimaryMonitor() override { return {}; }

    void SetOnMonitorChange(std::function<void()>) override {}
};

}  // namespace

std::unique_ptr<IMonitorManager> CreateMonitorManager() {
#ifdef _WIN32
    return std::make_unique<MonitorManagerWin>();
#else
    return std::make_unique<StubMonitorManager>();
#endif
}

}  // namespace blinkbreak::platform