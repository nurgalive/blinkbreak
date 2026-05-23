/// @file test_monitor_manager.cpp
/// @brief Unit tests for IMonitorManager and MonitorInfo.

#include <algorithm>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "platform/platform_interface.hpp"

namespace blinkbreak::platform
{
namespace
{

// ─────────────────────────────────────────────────────────────────────────────
// MonitorInfo tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(MonitorInfoTest, DefaultConstructionHasReasonableDefaults)
{
  MonitorInfo info{};
  EXPECT_EQ(info.id, 0);
  EXPECT_TRUE(info.name.empty());
  EXPECT_EQ(info.x, 0);
  EXPECT_EQ(info.y, 0);
  EXPECT_EQ(info.width, 0);
  EXPECT_EQ(info.height, 0);
  EXPECT_FALSE(info.is_primary);
  EXPECT_EQ(info.orientation, MonitorOrientation::kLandscape);
  EXPECT_EQ(info.dpi, 96u);
}

TEST(MonitorInfoTest, FieldsCanBeAssigned)
{
  MonitorInfo info{};
  info.id = 2;
  info.name = "Test Monitor";
  info.x = -1920;
  info.y = 0;
  info.width = 1920;
  info.height = 1080;
  info.is_primary = false;
  info.orientation = MonitorOrientation::kPortrait;
  info.dpi = 144;

  EXPECT_EQ(info.id, 2);
  EXPECT_EQ(info.name, "Test Monitor");
  EXPECT_EQ(info.x, -1920);
  EXPECT_EQ(info.y, 0);
  EXPECT_EQ(info.width, 1920);
  EXPECT_EQ(info.height, 1080);
  EXPECT_FALSE(info.is_primary);
  EXPECT_EQ(info.orientation, MonitorOrientation::kPortrait);
  EXPECT_EQ(info.dpi, 144u);
}

TEST(MonitorInfoTest, NegativeCoordinatesSupported)
{
  MonitorInfo info{};
  info.x = -3840;
  info.y = -1080;
  EXPECT_EQ(info.x, -3840);
  EXPECT_EQ(info.y, -1080);
}

// ─────────────────────────────────────────────────────────────────────────────
// MonitorOrientation tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(MonitorOrientationTest, OrientationToStringReturnsExpected)
{
  EXPECT_STREQ(OrientationToString(MonitorOrientation::kLandscape), "Landscape");
  EXPECT_STREQ(OrientationToString(MonitorOrientation::kPortrait), "Portrait");
  EXPECT_STREQ(OrientationToString(MonitorOrientation::kLandscapeFlipped), "Landscape (flipped)");
  EXPECT_STREQ(OrientationToString(MonitorOrientation::kPortraitFlipped), "Portrait (flipped)");
}

// ─────────────────────────────────────────────────────────────────────────────
// Mock MonitorManager for testing overlay manager behavior
// ─────────────────────────────────────────────────────────────────────────────

class MockMonitorManager : public IMonitorManager
{
public:
  explicit MockMonitorManager(std::vector<MonitorInfo> monitors) : monitors_(std::move(monitors)) {}

  void RefreshMonitors() override { ++refresh_count_; }

  std::vector<MonitorInfo> GetMonitors() const override { return monitors_; }

  MonitorInfo GetPrimaryMonitor() const override
  {
    for (const auto& m : monitors_)
    {
      if (m.is_primary)
        return m;
    }
    return monitors_.empty() ? MonitorInfo{} : monitors_.front();
  }

  int GetMonitorCount() const override { return static_cast<int>(monitors_.size()); }

  void SetOnMonitorChange(std::function<void()> callback) override
  {
    on_change_ = std::move(callback);
  }

  void SetMonitors(std::vector<MonitorInfo> monitors) { monitors_ = std::move(monitors); }

  int refresh_count_ = 0;

private:
  std::vector<MonitorInfo> monitors_;
  std::function<void()> on_change_;
};

// ─────────────────────────────────────────────────────────────────────────────
// MockMonitorManager tests (validate mock itself)
// ─────────────────────────────────────────────────────────────────────────────

MonitorInfo MakePrimary(int w, int h)
{
  MonitorInfo info{};
  info.id = 0;
  info.name = "Primary";
  info.x = 0;
  info.y = 0;
  info.width = w;
  info.height = h;
  info.is_primary = true;
  info.orientation = MonitorOrientation::kLandscape;
  return info;
}

MonitorInfo MakeSecondary(int id, int x, int y, int w, int h,
                          MonitorOrientation orient = MonitorOrientation::kLandscape)
{
  MonitorInfo info{};
  info.id = id;
  info.name = "Monitor " + std::to_string(id);
  info.x = x;
  info.y = y;
  info.width = w;
  info.height = h;
  info.is_primary = false;
  info.orientation = orient;
  return info;
}

TEST(MockMonitorManagerTest, SingleMonitor)
{
  MockMonitorManager mgr({MakePrimary(1920, 1080)});
  EXPECT_EQ(mgr.GetMonitorCount(), 1);
  EXPECT_TRUE(mgr.GetPrimaryMonitor().is_primary);
  EXPECT_EQ(mgr.GetPrimaryMonitor().width, 1920);
}

TEST(MockMonitorManagerTest, DualMonitorLandscape)
{
  auto primary = MakePrimary(1920, 1080);
  auto secondary = MakeSecondary(1, 1920, 0, 2560, 1440);

  MockMonitorManager mgr({primary, secondary});
  EXPECT_EQ(mgr.GetMonitorCount(), 2);

  auto monitors = mgr.GetMonitors();
  EXPECT_EQ(monitors.size(), 2u);
  EXPECT_TRUE(monitors[0].is_primary);
  EXPECT_EQ(monitors[1].x, 1920);
  EXPECT_EQ(monitors[1].width, 2560);
}

TEST(MockMonitorManagerTest, PortraitMonitor)
{
  auto portrait = MakeSecondary(1, 1920, 0, 1080, 1920, MonitorOrientation::kPortrait);
  MockMonitorManager mgr({MakePrimary(1920, 1080), portrait});

  auto monitors = mgr.GetMonitors();
  EXPECT_EQ(monitors[1].orientation, MonitorOrientation::kPortrait);
  EXPECT_EQ(monitors[1].width, 1080);
  EXPECT_EQ(monitors[1].height, 1920);
}

TEST(MockMonitorManagerTest, RefreshIncrementsCounter)
{
  MockMonitorManager mgr({MakePrimary(1920, 1080)});
  EXPECT_EQ(mgr.refresh_count_, 0);
  mgr.RefreshMonitors();
  mgr.RefreshMonitors();
  EXPECT_EQ(mgr.refresh_count_, 2);
}

TEST(MockMonitorManagerTest, GetPrimaryMonitorFallsBackToFirst)
{
  auto secondary1 = MakeSecondary(0, 0, 0, 1920, 1080);
  auto secondary2 = MakeSecondary(1, 1920, 0, 2560, 1440);

  MockMonitorManager mgr({secondary1, secondary2});
  // No monitor marked as primary: should fall back to first.
  EXPECT_EQ(mgr.GetPrimaryMonitor().id, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Win32 MonitorManager (live test — only verifies that it runs on Windows)
// ─────────────────────────────────────────────────────────────────────────────

#ifdef _WIN32

TEST(MonitorManagerWinTest, CreateMonitorManagerReturnsNonNull)
{
  auto mgr = CreateMonitorManager();
  EXPECT_NE(mgr, nullptr);
}

TEST(MonitorManagerWinTest, GetMonitorsReturnsAtLeastOne)
{
  auto mgr = CreateMonitorManager();
  auto monitors = mgr->GetMonitors();
  EXPECT_GE(monitors.size(), 1u);
}

TEST(MonitorManagerWinTest, PrimaryMonitorExists)
{
  auto mgr = CreateMonitorManager();
  auto primary = mgr->GetPrimaryMonitor();
  EXPECT_TRUE(primary.is_primary);
  EXPECT_GT(primary.width, 0);
  EXPECT_GT(primary.height, 0);
}

TEST(MonitorManagerWinTest, MonitorCountMatchesVector)
{
  auto mgr = CreateMonitorManager();
  auto monitors = mgr->GetMonitors();
  EXPECT_EQ(mgr->GetMonitorCount(), static_cast<int>(monitors.size()));
}

TEST(MonitorManagerWinTest, AllMonitorsHavePositiveDimensions)
{
  auto mgr = CreateMonitorManager();
  for (const auto& m : mgr->GetMonitors())
  {
    EXPECT_GT(m.width, 0) << "Monitor " << m.id << " has zero width";
    EXPECT_GT(m.height, 0) << "Monitor " << m.id << " has zero height";
  }
}

TEST(MonitorManagerWinTest, AllMonitorsHaveNonEmptyName)
{
  auto mgr = CreateMonitorManager();
  for (const auto& m : mgr->GetMonitors())
  {
    EXPECT_FALSE(m.name.empty()) << "Monitor " << m.id << " has empty name";
  }
}

TEST(MonitorManagerWinTest, AllMonitorsHaveValidDpi)
{
  auto mgr = CreateMonitorManager();
  for (const auto& m : mgr->GetMonitors())
  {
    EXPECT_GE(m.dpi, 72u) << "Monitor " << m.id << " has unreasonably low DPI";
    EXPECT_LE(m.dpi, 600u) << "Monitor " << m.id << " has unreasonably high DPI";
  }
}

TEST(MonitorManagerWinTest, RefreshMonitorsDoesNotCrash)
{
  auto mgr = CreateMonitorManager();
  mgr->RefreshMonitors();
  EXPECT_GE(mgr->GetMonitorCount(), 1);
}

TEST(MonitorManagerWinTest, SetOnMonitorChangeDoesNotCrash)
{
  auto mgr = CreateMonitorManager();
  int change_count = 0;
  mgr->SetOnMonitorChange([&change_count] { ++change_count; });
  // Just verify it doesn't crash; the callback fires on display changes.
}

#endif  // _WIN32

}  // namespace
}  // namespace blinkbreak::platform
