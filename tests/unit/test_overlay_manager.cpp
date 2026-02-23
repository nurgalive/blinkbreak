/// @file test_overlay_manager.cpp
/// @brief Unit tests for OverlayManager multi-monitor behavior.

#include <gtest/gtest.h>

#include "platform/monitor_manager.hpp"
#include "ui/overlay_manager.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <vector>

namespace blinkbreak::test {
namespace {

class FakeMonitorManager final : public platform::IMonitorManager {
public:
    explicit FakeMonitorManager(std::vector<platform::MonitorInfo> monitors)
        : monitors_(std::move(monitors)) {}

    std::vector<platform::MonitorInfo> GetMonitors() override { return monitors_; }

    platform::MonitorInfo GetPrimaryMonitor() override {
        for (const auto& monitor : monitors_) {
            if (monitor.is_primary) {
                return monitor;
            }
        }
        if (!monitors_.empty()) {
            return monitors_.front();
        }
        return {};
    }

    void SetOnMonitorChange(std::function<void()> callback) override {
        on_change_ = std::move(callback);
    }

    void SetMonitors(std::vector<platform::MonitorInfo> monitors) {
        monitors_ = std::move(monitors);
    }

    void FireChange() {
        if (on_change_) {
            on_change_();
        }
    }

private:
    std::vector<platform::MonitorInfo> monitors_;
    std::function<void()> on_change_;
};

platform::MonitorInfo MakeMonitor(int id,
                                  int x,
                                  int y,
                                  int width,
                                  int height,
                                  bool is_primary) {
    platform::MonitorInfo info;
    info.id = id;
    info.name = "Monitor " + std::to_string(id);
    info.x = x;
    info.y = y;
    info.width = width;
    info.height = height;
    info.is_primary = is_primary;
    return info;
}

BreakInfo MakeBreakInfo() {
    BreakInfo info;
    info.type = BreakType::kShort;
    info.duration = Duration{20};
    info.message = "Hydrate";
    info.can_skip = true;
    info.can_snooze = true;
    info.snooze_duration = Duration{300};
    return info;
}

std::function<void(std::function<void()>)> ImmediateInvoker() {
    return [](std::function<void()> task) { task(); };
}

}  // namespace

TEST(OverlayManagerTest, CreatesOverlayPerMonitorWhenShowingOnAll) {
    std::vector<platform::MonitorInfo> monitors{
        MakeMonitor(1, 0, 0, 1920, 1080, true),
        MakeMonitor(2, 1920, 0, 1920, 1080, false),
        MakeMonitor(3, -1600, 0, 1600, 900, false),
    };

    auto monitor_manager = std::make_unique<FakeMonitorManager>(monitors);
    OverlayManager manager(std::move(monitor_manager), ImmediateInvoker());

    manager.SetShowOnAllMonitors(true);
    manager.Show(MakeBreakInfo());

    EXPECT_EQ(manager.GetOverlayCount(), monitors.size());
}

TEST(OverlayManagerTest, UsesPrimaryMonitorWhenShowOnAllDisabled) {
    std::vector<platform::MonitorInfo> monitors{
        MakeMonitor(1, 0, 0, 1920, 1080, true),
        MakeMonitor(2, 1920, 0, 1920, 1080, false),
    };

    auto monitor_manager = std::make_unique<FakeMonitorManager>(monitors);
    OverlayManager manager(std::move(monitor_manager), ImmediateInvoker());

    manager.SetShowOnAllMonitors(false);
    manager.Show(MakeBreakInfo());

    EXPECT_EQ(manager.GetOverlayCount(), 1u);
}

TEST(OverlayManagerTest, HandleMonitorChangeRebuildsOverlays) {
    std::vector<platform::MonitorInfo> initial_monitors{
        MakeMonitor(1, 0, 0, 1920, 1080, true),
        MakeMonitor(2, 1920, 0, 1920, 1080, false),
    };

    auto fake_manager = std::make_unique<FakeMonitorManager>(initial_monitors);
    auto* fake_ptr = fake_manager.get();

    OverlayManager manager(std::move(fake_manager), ImmediateInvoker());
    manager.SetShowOnAllMonitors(true);
    manager.Show(MakeBreakInfo());

    EXPECT_EQ(manager.GetOverlayCount(), initial_monitors.size());

    std::vector<platform::MonitorInfo> updated_monitors{
        MakeMonitor(1, 0, 0, 1920, 1080, true),
    };

    fake_ptr->SetMonitors(updated_monitors);
    manager.HandleMonitorChange();

    EXPECT_EQ(manager.GetOverlayCount(), updated_monitors.size());
}

}  // namespace blinkbreak::test