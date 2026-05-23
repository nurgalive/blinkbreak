/// @file monitor_manager_win.cpp
/// @brief Windows implementation of IMonitorManager using Win32 APIs.

#include "monitor_manager_win.hpp"

#include <spdlog/spdlog.h>

// clang-format off
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellscalingapi.h>
// clang-format on

#include <algorithm>
#include <string>

#pragma comment(lib, "Shcore.lib")

namespace blinkbreak
{
namespace platform
{

namespace
{

/// @brief Converts a wide string to a UTF-8 std::string.
std::string WideToUtf8(const wchar_t* wide)
{
  if (!wide || wide[0] == L'\0')
  {
    return {};
  }
  const int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
  if (len <= 0)
  {
    return {};
  }
  std::string result(static_cast<std::size_t>(len - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), len, nullptr, nullptr);
  return result;
}

/// @brief Queries the display orientation for a device using EnumDisplaySettingsW.
MonitorOrientation QueryOrientation(const wchar_t* device_name)
{
  DEVMODEW dm{};
  dm.dmSize = sizeof(dm);
  if (!EnumDisplaySettingsW(device_name, ENUM_CURRENT_SETTINGS, &dm))
  {
    return MonitorOrientation::kLandscape;
  }
  switch (dm.dmDisplayOrientation)
  {
    case DMDO_DEFAULT:
      return MonitorOrientation::kLandscape;
    case DMDO_90:
      return MonitorOrientation::kPortrait;
    case DMDO_180:
      return MonitorOrientation::kLandscapeFlipped;
    case DMDO_270:
      return MonitorOrientation::kPortraitFlipped;
    default:
      return MonitorOrientation::kLandscape;
  }
}

/// @brief Queries the effective DPI for a monitor handle.
unsigned int QueryDpi(HMONITOR hmon)
{
  UINT dpi_x = 96;
  UINT dpi_y = 96;
  if (SUCCEEDED(GetDpiForMonitor(hmon, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y)))
  {
    return static_cast<unsigned int>(dpi_x);
  }
  return 96;
}

}  // namespace

MonitorManagerWin::MonitorManagerWin()
{
  RefreshMonitors();
}

void MonitorManagerWin::RefreshMonitors()
{
  EnumContext ctx{};

  EnumDisplayMonitors(
      nullptr, nullptr,
      [](HMONITOR hMonitor, HDC /*hdc*/, LPRECT /*lprcMonitor*/, LPARAM dwData) -> BOOL
      {
        auto* context = reinterpret_cast<EnumContext*>(dwData);

        MONITORINFOEXW mi{};
        mi.cbSize = sizeof(mi);
        if (!GetMonitorInfoW(hMonitor, &mi))
        {
          return TRUE;  // Continue enumeration.
        }

        MonitorInfo info{};
        info.id = context->next_id++;
        info.name = WideToUtf8(mi.szDevice);
        info.x = mi.rcMonitor.left;
        info.y = mi.rcMonitor.top;
        info.width = mi.rcMonitor.right - mi.rcMonitor.left;
        info.height = mi.rcMonitor.bottom - mi.rcMonitor.top;
        info.is_primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
        info.orientation = QueryOrientation(mi.szDevice);
        info.dpi = QueryDpi(hMonitor);

        context->monitors.push_back(std::move(info));
        return TRUE;
      },
      reinterpret_cast<LPARAM>(&ctx));

  // Sort so that the primary is always first.
  std::ranges::sort(ctx.monitors,
                    [](const MonitorInfo& a, const MonitorInfo& b)
                    {
                      if (a.is_primary != b.is_primary)
                      {
                        return a.is_primary;
                      }
                      return a.id < b.id;
                    });

  // Re-assign sequential IDs after sorting.
  for (int i = 0; i < static_cast<int>(ctx.monitors.size()); ++i)
  {
    ctx.monitors[static_cast<std::size_t>(i)].id = i;
  }

  {
    std::lock_guard lock(mutex_);
    monitors_ = std::move(ctx.monitors);
  }

  spdlog::info("MonitorManagerWin: detected {} monitor(s)", monitors_.size());
  for (const auto& m : monitors_)
  {
    spdlog::info("  [{}] {} {}x{} @ ({},{}) primary={} orientation={} dpi={}", m.id, m.name,
                 m.width, m.height, m.x, m.y, m.is_primary, OrientationToString(m.orientation),
                 m.dpi);
  }
}

std::vector<MonitorInfo> MonitorManagerWin::GetMonitors() const
{
  std::lock_guard lock(mutex_);
  return monitors_;
}

MonitorInfo MonitorManagerWin::GetPrimaryMonitor() const
{
  std::lock_guard lock(mutex_);
  for (const auto& m : monitors_)
  {
    if (m.is_primary)
    {
      return m;
    }
  }
  // Fallback: return the first monitor if none is marked primary.
  if (!monitors_.empty())
  {
    return monitors_.front();
  }
  return {};
}

int MonitorManagerWin::GetMonitorCount() const
{
  std::lock_guard lock(mutex_);
  return static_cast<int>(monitors_.size());
}

void MonitorManagerWin::SetOnMonitorChange(std::function<void()> callback)
{
  std::lock_guard lock(mutex_);
  on_change_ = std::move(callback);
}

// Factory function
std::unique_ptr<IMonitorManager> CreateMonitorManager()
{
  return std::make_unique<MonitorManagerWin>();
}

}  // namespace platform
}  // namespace blinkbreak
