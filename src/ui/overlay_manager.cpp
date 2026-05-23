/// @file overlay_manager.cpp
/// @brief Implementation of the OverlayManager class with multi-monitor support.

#include "overlay_manager.hpp"

#include <slint.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <format>
#include <string>
#include <utility>

#include <spdlog/spdlog.h>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#endif

#include "overlay.h"

namespace blinkbreak
{
namespace
{

std::string FormatDuration(Duration duration)
{
  const auto total_seconds = std::max<std::int64_t>(0, static_cast<std::int64_t>(duration.count()));
  const auto minutes = total_seconds / 60;
  const auto seconds = total_seconds % 60;
  return std::format("{:02}:{:02}", minutes, seconds);
}

slint::SharedString ToSharedString(const std::string& value)
{
  return slint::SharedString(value);
}

void RunOnEventLoop(std::function<void()> task)
{
  slint::invoke_from_event_loop(std::move(task));
}

}  // namespace

OverlayManager::OverlayManager() = default;

OverlayManager::~OverlayManager()
{
  Hide();
}

void OverlayManager::SetMonitorManager(std::shared_ptr<platform::IMonitorManager> monitor_manager)
{
  std::lock_guard lock(overlay_mutex_);
  monitor_manager_ = std::move(monitor_manager);
}

void OverlayManager::SetShowOnAllMonitors(bool all_monitors)
{
  std::lock_guard lock(overlay_mutex_);
  show_on_all_monitors_ = all_monitors;
  spdlog::info("OverlayManager: show_on_all_monitors = {}", all_monitors);
}

bool OverlayManager::GetShowOnAllMonitors() const
{
  std::lock_guard lock(overlay_mutex_);
  return show_on_all_monitors_;
}

void OverlayManager::Show(const BreakInfo& info)
{
  {
    std::lock_guard lock(overlay_mutex_);
    pending_show_ = true;
    pending_message_ = info.message;
    pending_break_type_ = std::string(BreakTypeToString(info.type));
    pending_time_remaining_ = FormatDuration(info.duration);
    pending_can_skip_ = info.can_skip;
    pending_can_snooze_ = info.can_snooze;
    if (info.can_snooze && info.snooze_duration > Duration::zero())
    {
      const auto minutes = std::max<std::int64_t>(1, (info.snooze_duration.count() + 59) / 60);
      pending_snooze_label_ = std::format("Snooze ({} min)", minutes);
    }
    else
    {
      pending_snooze_label_.clear();
    }
  }

  RunOnEventLoop(
      [this]()
      {
        CreateOverlays();

        std::lock_guard lock(overlay_mutex_);
        for (auto& instance : overlays_)
        {
          ApplyProperties(instance);
          PositionOverlay(instance);
          (*instance.handle)->show();
      // Force a small resize nudge to ensure the Slint software renderer
      // updates its backing buffer on some GPU/DPI configurations where a
      // newly created fullscreen window can appear blank.
#ifdef _WIN32
          HWND hwnd_nudge = (*instance.handle)->window().win32_hwnd();
          if (hwnd_nudge)
          {
            RECT r2{};
            if (GetWindowRect(hwnd_nudge, &r2))
            {
              const int w = r2.right - r2.left;
              const int h = r2.bottom - r2.top;
              // tweak size by +1 then restore after a short delay
              SetWindowPos(hwnd_nudge, nullptr, 0, 0, w + 1, h, SWP_NOMOVE | SWP_NOZORDER);
              slint::Timer::single_shot(
                  std::chrono::milliseconds(50), [hwnd_nudge, w, h]()
                  { SetWindowPos(hwnd_nudge, nullptr, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER); });
            }
          }
#endif
      // Diagnostic: log native HWND and window rect so we can debug per-monitor
      // placement issues (helps when high-DPI or multi-GPU setups misplace windows).
#ifdef _WIN32
          HWND hwnd = (*instance.handle)->window().win32_hwnd();
          if (hwnd)
          {
            RECT r{};
            if (GetWindowRect(hwnd, &r))
            {
              spdlog::debug("Overlay created for '{}' hwnd={} rect=({},{} {}x{}) dpi={}",
                            instance.monitor.name, reinterpret_cast<std::uintptr_t>(hwnd), r.left,
                            r.top, r.right - r.left, r.bottom - r.top, instance.monitor.dpi);
            }
            else
            {
              spdlog::warn("Overlay created for '{}' hwnd={} but GetWindowRect failed, err={}",
                           instance.monitor.name, reinterpret_cast<std::uintptr_t>(hwnd),
                           GetLastError());
            }
          }
          else
          {
            spdlog::warn("Overlay created for '{}' but native hwnd is null", instance.monitor.name);
          }
#endif
        }
        visible_ = !overlays_.empty();
        pending_show_ = false;
      });
}

void OverlayManager::Hide()
{
  {
    std::lock_guard lock(overlay_mutex_);
    pending_show_ = false;
  }

  RunOnEventLoop(
      [this]()
      {
        {
          std::lock_guard lock(overlay_mutex_);
          for (auto& instance : overlays_)
          {
            (*instance.handle)->hide();
          }
          overlays_.clear();
          visible_ = false;
        }
      });
}

void OverlayManager::UpdateTimeRemaining(const std::string& time_remaining)
{
  {
    std::lock_guard lock(overlay_mutex_);
    pending_time_remaining_ = time_remaining;
    if (overlays_.empty())
    {
      return;
    }
  }

  RunOnEventLoop(
      [this]()
      {
        std::lock_guard lock(overlay_mutex_);
        for (auto& instance : overlays_)
        {
          auto handle = *instance.handle;
          handle->set_time_remaining(ToSharedString(pending_time_remaining_));
        }
      });
}

void OverlayManager::UpdateMessage(const std::string& message)
{
  {
    std::lock_guard lock(overlay_mutex_);
    pending_message_ = message;
    if (overlays_.empty())
    {
      return;
    }
  }

  RunOnEventLoop(
      [this]()
      {
        std::lock_guard lock(overlay_mutex_);
        for (auto& instance : overlays_)
        {
          auto handle = *instance.handle;
          handle->set_message(ToSharedString(pending_message_));
        }
      });
}

void OverlayManager::UpdateActions(bool can_skip, bool can_snooze)
{
  {
    std::lock_guard lock(overlay_mutex_);
    pending_can_skip_ = can_skip;
    pending_can_snooze_ = can_snooze;
    if (overlays_.empty())
    {
      return;
    }
  }

  RunOnEventLoop(
      [this]()
      {
        std::lock_guard lock(overlay_mutex_);
        for (auto& instance : overlays_)
        {
          auto handle = *instance.handle;
          handle->set_can_skip(pending_can_skip_);
          handle->set_can_snooze(pending_can_snooze_);
        }
      });
}

void OverlayManager::UpdateOpacity(float opacity)
{
  {
    std::lock_guard lock(overlay_mutex_);
    pending_opacity_ = opacity;
    if (overlays_.empty())
    {
      return;
    }
  }

  RunOnEventLoop(
      [this]()
      {
        std::lock_guard lock(overlay_mutex_);
        if (!pending_opacity_)
        {
          return;
        }
        const float opa = std::clamp(*pending_opacity_, 0.0f, 1.0f);
        for (auto& instance : overlays_)
        {
          auto handle = *instance.handle;
          handle->set_overlay_opacity(opa);
        }
        pending_opacity_.reset();
      });
}

void OverlayManager::UpdateSnoozeLabel(const std::string& label)
{
  {
    std::lock_guard lock(overlay_mutex_);
    pending_snooze_label_ = label;
    if (overlays_.empty())
    {
      return;
    }
  }

  RunOnEventLoop(
      [this]()
      {
        std::lock_guard lock(overlay_mutex_);
        if (pending_snooze_label_.empty())
        {
          return;
        }
        for (auto& instance : overlays_)
        {
          auto handle = *instance.handle;
          handle->set_snooze_label(ToSharedString(pending_snooze_label_));
        }
      });
}

void OverlayManager::SetOnSkip(std::function<void()> callback)
{
  std::lock_guard lock(overlay_mutex_);
  on_skip_ = std::move(callback);
}

void OverlayManager::SetOnSnooze(std::function<void()> callback)
{
  std::lock_guard lock(overlay_mutex_);
  on_snooze_ = std::move(callback);
}

bool OverlayManager::IsVisible() const
{
  std::lock_guard lock(overlay_mutex_);
  return visible_;
}

void OverlayManager::CreateOverlays()
{
  // Clear existing overlays.
  for (auto& instance : overlays_)
  {
    (*instance.handle)->hide();
  }
  overlays_.clear();

  // Determine target monitors.
  std::vector<platform::MonitorInfo> targets;

  {
    std::lock_guard lock(overlay_mutex_);
    if (monitor_manager_)
    {
      monitor_manager_->RefreshMonitors();
      if (show_on_all_monitors_)
      {
        targets = monitor_manager_->GetMonitors();
      }
      else
      {
        targets.push_back(monitor_manager_->GetPrimaryMonitor());
      }
    }
  }

  // Fallback: if no monitors, create a single fullscreen overlay.
  if (targets.empty())
  {
    spdlog::warn("OverlayManager: no monitors detected, using single fullscreen overlay");
    platform::MonitorInfo fallback{};
    fallback.id = 0;
    fallback.name = "fallback";
    fallback.is_primary = true;
    targets.push_back(fallback);
  }

  spdlog::info("OverlayManager: creating {} overlay window(s)", targets.size());

  for (auto& monitor : targets)
  {
    OverlayInstance instance{};
    instance.handle =
        std::make_unique<slint::ComponentHandle<BreakOverlay>>(BreakOverlay::create());
    instance.monitor = std::move(monitor);

    WireCallbacks(*instance.handle);
    overlays_.push_back(std::move(instance));
  }
}

void OverlayManager::ApplyProperties(OverlayInstance& instance)
{
  auto handle = *instance.handle;
  handle->set_message(ToSharedString(pending_message_));
  handle->set_break_type(ToSharedString(pending_break_type_));
  handle->set_time_remaining(ToSharedString(pending_time_remaining_));
  handle->set_can_skip(pending_can_skip_);
  handle->set_can_snooze(pending_can_snooze_);
  if (!pending_snooze_label_.empty())
  {
    handle->set_snooze_label(ToSharedString(pending_snooze_label_));
  }
  if (pending_opacity_)
  {
    handle->set_overlay_opacity(std::clamp(*pending_opacity_, 0.0f, 1.0f));
  }
}

void OverlayManager::WireCallbacks(const slint::ComponentHandle<BreakOverlay>& handle)
{
  handle->on_skip_clicked(
      [this]
      {
        std::function<void()> callback;
        {
          std::lock_guard lock(overlay_mutex_);
          callback = on_skip_;
        }
        if (callback)
        {
          callback();
        }
      });

  handle->on_snooze_clicked(
      [this]
      {
        std::function<void()> callback;
        {
          std::lock_guard lock(overlay_mutex_);
          callback = on_snooze_;
        }
        if (callback)
        {
          callback();
        }
      });
}

void OverlayManager::PositionOverlay(OverlayInstance& instance)
{
  const auto& mon = instance.monitor;
  auto handle = *instance.handle;

  if (mon.width <= 0 || mon.height <= 0)
  {
    // Fallback: use fullscreen mode for unknown geometry.
    spdlog::debug("OverlayManager: monitor '{}' has no geometry, using fullscreen", mon.name);
    handle->window().set_fullscreen(true);
    return;
  }

  // Position the overlay window to exactly cover the monitor.
  spdlog::debug("OverlayManager: positioning overlay on '{}' at ({},{}) {}x{} orientation={}",
                mon.name, mon.x, mon.y, mon.width, mon.height,
                platform::OrientationToString(mon.orientation));

  // Slint's set_position/set_size use physical pixels, which is what Win32 gives us.
  handle->window().set_position(slint::PhysicalPosition({mon.x, mon.y}));
  handle->window().set_size(
      slint::PhysicalSize({static_cast<uint32_t>(mon.width), static_cast<uint32_t>(mon.height)}));

  // After setting position and size, set fullscreen to ensure window flags
  // (always-on-top, no frame) work correctly with the WM.
  handle->window().set_fullscreen(true);
}

}  // namespace blinkbreak
