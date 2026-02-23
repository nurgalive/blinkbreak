/// @file overlay_manager.cpp
/// @brief Implementation of the OverlayManager class.

#include "overlay_manager.hpp"

#include "overlay.h"
#include "platform/monitor_manager.hpp"

#include <slint.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#include <algorithm>
#include <cstdint>
#include <format>
#include <string>
#include <utility>

namespace blinkbreak {
namespace {

std::string FormatDuration(Duration duration) {
    const auto total_seconds =
        std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    const auto minutes = total_seconds / 60;
    const auto seconds = total_seconds % 60;
    return std::format("{:02}:{:02}", minutes, seconds);
}

slint::SharedString ToSharedString(const std::string& str) {
    return slint::SharedString(str);
}

#ifdef _WIN32

/// @brief Apply Win32 window styles to hide overlay from taskbar/Alt+Tab.
/// Also ensures WS_EX_TOPMOST is set for reliable always-on-top behaviour.
/// Does NOT use SWP_FRAMECHANGED to avoid triggering WM_NCCALCSIZE which causes
/// Slint to re-process and mangle the window geometry on non-primary monitors.
void StyleOverlayWin32(HWND hwnd, std::size_t index) {
    if (!hwnd) {
        return;
    }

    LONG ex_style = GetWindowLong(hwnd, GWL_EXSTYLE);
    const LONG original_ex_style = ex_style;

    ex_style |= WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
    ex_style &= ~WS_EX_APPWINDOW;

    if (ex_style != original_ex_style) {
        SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        spdlog::debug("Overlay[{}] Win32 styles applied (WS_EX_TOOLWINDOW|WS_EX_TOPMOST)", index);
    }
}

/// @brief Force correct window geometry and Z-order via Win32 SetWindowPos.
/// Uses HWND_TOPMOST to ensure always-on-top, and physical pixel coordinates.
/// Follows up with InvalidateRect + UpdateWindow to force an immediate full
/// repaint of the client area.
void ForceWin32Geometry(HWND hwnd, std::size_t index, int x, int y, int w, int h) {
    if (!hwnd) {
        return;
    }
    SetWindowPos(hwnd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE);
    InvalidateRect(hwnd, nullptr, TRUE);
    UpdateWindow(hwnd);
    spdlog::info("Overlay[{}] Win32 forced geometry + repaint: ({},{} {}x{})", index, x, y, w, h);
}

/// @brief Force the GPU swapchain to be recreated at the correct dimensions
/// by toggling the window through a tiny intermediate size.
///
/// On portrait (rotated) monitors the Slint/skia DirectX backend may create the
/// swapchain at the native (un-rotated) panel dimensions (e.g. 2560x1440 instead
/// of 1440x2560).  Resizing through an intermediate 1x1 forces the backend to
/// call IDXGISwapChain::ResizeBuffers twice — the second call creates the
/// swapchain at the correct portrait dimensions.
void ForceSwapchainRecreation(HWND hwnd, std::size_t index, int x, int y, int w, int h) {
    if (!hwnd) {
        return;
    }

    spdlog::info("Overlay[{}] forcing swapchain recreation via size toggle ({},{})->({},{})",
                 index, 1, 1, w, h);

    // Step 1: resize to 1x1 — forces the swapchain to a clearly different size
    SetWindowPos(hwnd, HWND_TOPMOST, x, y, 1, 1, SWP_NOACTIVATE);

    // Step 2: resize to the correct portrait dimensions — the swapchain MUST be
    // recreated because the dimensions differ from step 1
    SetWindowPos(hwnd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE);

    InvalidateRect(hwnd, nullptr, TRUE);
    UpdateWindow(hwnd);

    spdlog::info("Overlay[{}] swapchain recreation complete", index);
}

/// @brief Log the actual Win32 window rect for diagnostic purposes.
void LogOverlayWin32Rect(HWND hwnd, std::size_t index) {
    if (!hwnd) {
        return;
    }

    RECT rect = {};
    if (GetWindowRect(hwnd, &rect)) {
        const int w = rect.right - rect.left;
        const int h = rect.bottom - rect.top;
        spdlog::info("Overlay[{}] Win32 actual rect: x={} y={} w={} h={}",
                     index, rect.left, rect.top, w, h);
    }
}

/// @brief Query the display orientation for a given device name.
/// Returns DMDO_DEFAULT (0), DMDO_90 (1), DMDO_180 (2), or DMDO_270 (3).
DWORD GetDisplayOrientation(const std::string& device_name) {
    // Convert UTF-8 device name to wide string
    std::wstring wide_name;
    if (!device_name.empty()) {
        int required = MultiByteToWideChar(CP_UTF8, 0, device_name.c_str(),
                                           static_cast<int>(device_name.size()),
                                           nullptr, 0);
        if (required > 0) {
            wide_name.resize(static_cast<size_t>(required));
            MultiByteToWideChar(CP_UTF8, 0, device_name.c_str(),
                                static_cast<int>(device_name.size()),
                                wide_name.data(), required);
        }
    }

    DEVMODEW dm = {};
    dm.dmSize = sizeof(dm);

    const wchar_t* dev = wide_name.empty() ? nullptr : wide_name.c_str();
    if (EnumDisplaySettingsW(dev, ENUM_CURRENT_SETTINGS, &dm)) {
        spdlog::info("Display '{}' orientation={} native={}x{}",
                     device_name, dm.dmDisplayOrientation,
                     dm.dmPelsWidth, dm.dmPelsHeight);
        return dm.dmDisplayOrientation;
    }
    return DMDO_DEFAULT;
}

#endif  // _WIN32

}  // namespace

OverlayManager::OverlayManager()
    : OverlayManager(platform::CreateMonitorManager()) {}

OverlayManager::OverlayManager(std::unique_ptr<platform::IMonitorManager> monitor_manager)
    : OverlayManager(std::move(monitor_manager), nullptr) {}

OverlayManager::OverlayManager(std::unique_ptr<platform::IMonitorManager> monitor_manager,
                               std::function<void(std::function<void()>)> event_loop_invoker)
    : monitor_manager_(std::move(monitor_manager)),
      visible_(false),
      pending_show_(false),
      pending_can_skip_(false),
      pending_can_snooze_(false),
      show_on_all_monitors_(true),
      event_loop_invoker_(std::move(event_loop_invoker)) {
    if (monitor_manager_) {
        monitor_manager_->SetOnMonitorChange([this]() { HandleMonitorChange(); });
    }
}

OverlayManager::~OverlayManager() = default;

void OverlayManager::Show(const BreakInfo& info) {
    {
        std::lock_guard lock(overlay_mutex_);
        pending_show_ = true;
        pending_message_ = info.message;
        pending_break_type_ = std::string(BreakTypeToString(info.type));
        pending_time_remaining_ = FormatDuration(info.duration);
        pending_can_skip_ = info.can_skip;
        pending_can_snooze_ = info.can_snooze;
        if (info.can_snooze && info.snooze_duration > Duration::zero()) {
            const auto minutes = std::max<std::int64_t>(
                1, (info.snooze_duration.count() + 59) / 60);
            pending_snooze_label_ = std::format("Snooze ({} min)", minutes);
        } else {
            pending_snooze_label_.clear();
        }
    }

    RunOnEventLoop([this]() {
        // 1. Create overlay windows and cache monitor info
        PrepareOverlaysForLayout();

        std::string message;
        std::string break_type;
        std::string time_remaining;
        std::string snooze_label;
        bool can_skip = false;
        bool can_snooze = false;
        std::optional<float> opacity_override;
        bool should_show = false;

        {
            std::lock_guard lock(overlay_mutex_);
            message = pending_message_;
            break_type = pending_break_type_;
            time_remaining = pending_time_remaining_;
            snooze_label = pending_snooze_label_;
            can_skip = pending_can_skip_;
            can_snooze = pending_can_snooze_;
            opacity_override = pending_opacity_;
            should_show = pending_show_;
        }

        // 2. PRE-SHOW: set position and size on each overlay BEFORE show().
        //    This ensures the rendering surface is created at the correct
        //    dimensions (critical for portrait monitors).
        PreSizeOverlaysForMonitors();

        // 3. Set overlay properties (message, buttons, opacity) and show
        ApplyStateToOverlays(message, break_type, time_remaining, snooze_label,
                             can_skip, can_snooze, opacity_override, should_show);

        {
            std::lock_guard lock(overlay_mutex_);
            visible_ = should_show;
        }

        // 4. Schedule deferred positioning after Slint creates native HWNDs.
        //    Re-applies position + size via Slint API, then Win32 styling.
        //    For portrait monitors, forces swapchain recreation via size toggle.
        if (should_show) {
            ScheduleDeferredPositioning();
        }
    });
}

void OverlayManager::Hide() {
    {
        std::lock_guard lock(overlay_mutex_);
        pending_show_ = false;
    }

    RunOnEventLoop([this]() {
        // Cancel any pending positioning timer
        {
            std::lock_guard lock(overlay_mutex_);
            position_timer_.reset();
        }

        {
            std::lock_guard lock(overlay_mutex_);
            for (auto& overlay_ptr : overlays_) {
                if (overlay_ptr) {
                    auto overlay_handle = *overlay_ptr;
                    overlay_handle->hide();
                }
            }
            visible_ = false;
        }
    });
}

void OverlayManager::UpdateTimeRemaining(const std::string& time_remaining) {
    bool has_overlay = false;
    {
        std::lock_guard lock(overlay_mutex_);
        has_overlay = !overlays_.empty() && visible_;
    }

    if (!has_overlay) {
        return;
    }

    RunOnEventLoop([this, value = time_remaining]() {
        std::lock_guard lock(overlay_mutex_);
        for (auto& overlay_ptr : overlays_) {
            if (!overlay_ptr) {
                continue;
            }
            auto overlay_handle = *overlay_ptr;
            overlay_handle->set_time_remaining(ToSharedString(value));
        }
    });
}

void OverlayManager::UpdateMessage(const std::string& message) {
    bool has_overlay = false;
    {
        std::lock_guard lock(overlay_mutex_);
        has_overlay = !overlays_.empty() && visible_;
    }

    if (!has_overlay) {
        return;
    }

    RunOnEventLoop([this, value = message]() {
        std::lock_guard lock(overlay_mutex_);
        for (auto& overlay_ptr : overlays_) {
            if (!overlay_ptr) {
                continue;
            }
            auto overlay_handle = *overlay_ptr;
            overlay_handle->set_message(ToSharedString(value));
        }
    });
}

void OverlayManager::UpdateActions(bool can_skip, bool can_snooze) {
    bool has_overlay = false;
    {
        std::lock_guard lock(overlay_mutex_);
        has_overlay = !overlays_.empty() && visible_;
    }

    if (!has_overlay) {
        return;
    }

    RunOnEventLoop([this, can_skip, can_snooze]() {
        std::lock_guard lock(overlay_mutex_);
        for (auto& overlay_ptr : overlays_) {
            if (!overlay_ptr) {
                continue;
            }
            auto overlay_handle = *overlay_ptr;
            overlay_handle->set_can_skip(can_skip);
            overlay_handle->set_can_snooze(can_snooze);
        }
    });
}

void OverlayManager::UpdateOpacity(float opacity) {
    bool has_overlay = false;
    {
        std::lock_guard lock(overlay_mutex_);
        has_overlay = !overlays_.empty() && visible_;
    }

    if (!has_overlay) {
        return;
    }

    RunOnEventLoop([this, opacity]() {
        std::optional<float> op = opacity;
        std::lock_guard lock(overlay_mutex_);
        for (auto& overlay_ptr : overlays_) {
            if (!overlay_ptr) {
                continue;
            }
            auto overlay_handle = *overlay_ptr;
            overlay_handle->set_overlay_opacity(std::clamp(opacity, 0.0f, 1.0f));
        }
    });
}

void OverlayManager::UpdateSnoozeLabel(const std::string& label) {
    bool has_overlay = false;
    {
        std::lock_guard lock(overlay_mutex_);
        has_overlay = !overlays_.empty() && visible_;
    }

    if (!has_overlay) {
        return;
    }

    RunOnEventLoop([this, label]() {
        std::lock_guard lock(overlay_mutex_);
        for (auto& overlay_ptr : overlays_) {
            if (!overlay_ptr) {
                continue;
            }
            auto overlay_handle = *overlay_ptr;
            overlay_handle->set_snooze_label(ToSharedString(label));
        }
    });
}

void OverlayManager::SetShowOnAllMonitors(bool show_on_all) {
    bool was_visible = false;
    std::string message;
    std::string break_type;
    std::string time_remaining;
    std::string snooze_label;
    bool can_skip = false;
    bool can_snooze = false;
    std::optional<float> opacity_override;
    bool should_show = false;

    {
        std::lock_guard lock(overlay_mutex_);
        was_visible = visible_;
        show_on_all_monitors_ = show_on_all;
        message = pending_message_;
        break_type = pending_break_type_;
        time_remaining = pending_time_remaining_;
        snooze_label = pending_snooze_label_;
        can_skip = pending_can_skip_;
        can_snooze = pending_can_snooze_;
        opacity_override = pending_opacity_;
        should_show = pending_show_;
    }

    if (was_visible) {
        RunOnEventLoop([this, message, break_type, time_remaining, snooze_label,
                        can_skip, can_snooze, opacity_override, should_show]() {
            PrepareOverlaysForLayout();
            PreSizeOverlaysForMonitors();
            ApplyStateToOverlays(message, break_type, time_remaining, snooze_label,
                                 can_skip, can_snooze, opacity_override, should_show);
            {
                std::lock_guard lock(overlay_mutex_);
                visible_ = should_show;
            }
            if (should_show) {
                ScheduleDeferredPositioning();
            }
        });
    }
}

void OverlayManager::HandleMonitorChange() {
    std::string message;
    std::string break_type;
    std::string time_remaining;
    std::string snooze_label;
    bool can_skip = false;
    bool can_snooze = false;
    std::optional<float> opacity_override;
    bool should_show = false;

    {
        std::lock_guard lock(overlay_mutex_);
        if (!visible_) {
            return;
        }
        message = pending_message_;
        break_type = pending_break_type_;
        time_remaining = pending_time_remaining_;
        snooze_label = pending_snooze_label_;
        can_skip = pending_can_skip_;
        can_snooze = pending_can_snooze_;
        opacity_override = pending_opacity_;
        should_show = pending_show_;
    }

    RunOnEventLoop([this, message, break_type, time_remaining, snooze_label,
                    can_skip, can_snooze, opacity_override, should_show]() {
        PrepareOverlaysForLayout();
        PreSizeOverlaysForMonitors();
        ApplyStateToOverlays(message, break_type, time_remaining, snooze_label,
                             can_skip, can_snooze, opacity_override, should_show);
        {
            std::lock_guard lock(overlay_mutex_);
            visible_ = should_show;
        }
        if (should_show) {
            ScheduleDeferredPositioning();
        }
    });
}

void OverlayManager::SetOnSkip(std::function<void()> callback) {
    bool has_overlay = false;
    {
        std::lock_guard lock(overlay_mutex_);
        on_skip_ = callback;
        has_overlay = !overlays_.empty();
    }

    if (!has_overlay) {
        return;
    }

    RunOnEventLoop([this]() {
        for (auto& overlay_ptr : overlays_) {
            if (!overlay_ptr) {
                continue;
            }
            auto overlay_handle = *overlay_ptr;
            overlay_handle->on_skip_clicked([this] {
                std::function<void()> callback;
                {
                    std::lock_guard lock(overlay_mutex_);
                    callback = on_skip_;
                }
                if (callback) {
                    callback();
                }
            });
        }
    });
}

void OverlayManager::SetOnSnooze(std::function<void()> callback) {
    bool has_overlay = false;
    {
        std::lock_guard lock(overlay_mutex_);
        on_snooze_ = callback;
        has_overlay = !overlays_.empty();
    }

    if (!has_overlay) {
        return;
    }

    RunOnEventLoop([this]() {
        for (auto& overlay_ptr : overlays_) {
            if (!overlay_ptr) {
                continue;
            }
            auto overlay_handle = *overlay_ptr;
            overlay_handle->on_snooze_clicked([this] {
                std::function<void()> callback;
                {
                    std::lock_guard lock(overlay_mutex_);
                    callback = on_snooze_;
                }
                if (callback) {
                    callback();
                }
            });
        }
    });
}

bool OverlayManager::IsVisible() const {
    std::lock_guard lock(overlay_mutex_);
    return visible_;
}

std::size_t OverlayManager::GetOverlayCount() const {
    std::lock_guard lock(overlay_mutex_);
    return overlays_.size();
}

void OverlayManager::RunOnEventLoop(std::function<void()> task) {
    if (event_loop_invoker_) {
        event_loop_invoker_(std::move(task));
    } else {
        slint::invoke_from_event_loop(std::move(task));
    }
}

void OverlayManager::PrepareOverlaysForLayout() {
    bool show_all = true;
    {
        std::lock_guard lock(overlay_mutex_);
        show_all = show_on_all_monitors_;
    }

    std::vector<platform::MonitorInfo> monitors;
    if (monitor_manager_ && show_all) {
        monitors = monitor_manager_->GetMonitors();
    } else if (monitor_manager_) {
        const auto primary = monitor_manager_->GetPrimaryMonitor();
        if (primary.width > 0 && primary.height > 0) {
            monitors.push_back(primary);
        }
    }

    spdlog::info(
        "Overlay layout: show_all={}, monitor_count={}, overlay_count={}",
        show_all, monitors.size(), overlays_.size());

    for (std::size_t i = 0; i < monitors.size(); ++i) {
        const auto& monitor = monitors[i];
        const bool is_portrait = monitor.height > monitor.width;
        spdlog::info(
            "Monitor[{}] id={} name='{}' primary={} x={} y={} w={} h={} scale={} portrait={}",
            i, monitor.id, monitor.name, monitor.is_primary, monitor.x, monitor.y,
            monitor.width, monitor.height, monitor.scale_factor, is_portrait);

#ifdef _WIN32
        // Query native orientation for diagnostics
        if (is_portrait) {
            GetDisplayOrientation(monitor.name);
        }
#endif
    }

    // Cache monitors for positioning phase
    {
        std::lock_guard lock(overlay_mutex_);
        cached_monitors_ = std::move(monitors);
    }

    const auto monitor_count = std::max<std::size_t>(1, cached_monitors_.size());
    EnsureOverlaysForMonitors(monitor_count);
}

void OverlayManager::PreSizeOverlaysForMonitors() {
    std::vector<platform::MonitorInfo> monitors;
    {
        std::lock_guard lock(overlay_mutex_);
        monitors = cached_monitors_;
    }

    const auto count = (std::min)(monitors.size(), overlays_.size());

    spdlog::info("Pre-show sizing {} overlays for {} monitors", count, monitors.size());

    for (std::size_t i = 0; i < count; ++i) {
        const auto& monitor = monitors[i];
        auto overlay_handle = *overlays_[i];

        const bool is_portrait = monitor.height > monitor.width;

        spdlog::info(
            "PreSize[{}] monitor: x={} y={} w={} h={} scale={} portrait={}",
            i, monitor.x, monitor.y, monitor.width, monitor.height,
            monitor.scale_factor, is_portrait);

        // Set size and position BEFORE show() so Slint creates the
        // rendering surface at the correct dimensions from the start.
        overlay_handle->window().set_size(
            slint::PhysicalSize({static_cast<uint32_t>(monitor.width),
                                 static_cast<uint32_t>(monitor.height)}));
        overlay_handle->window().set_position(
            slint::PhysicalPosition({monitor.x, monitor.y}));
    }
}

void OverlayManager::ScheduleDeferredPositioning() {
    // Defer positioning to give Slint time to create native HWNDs after show().
    std::lock_guard lock(overlay_mutex_);

    position_timer_ = std::make_unique<slint::Timer>();
    position_timer_->start(
        slint::TimerMode::SingleShot,
        std::chrono::milliseconds(80),
        [this]() {
            PositionOverlaysOnMonitors();
        });
}

void OverlayManager::PositionOverlaysOnMonitors() {
    std::vector<platform::MonitorInfo> monitors;
    bool show_all = true;
    {
        std::lock_guard lock(overlay_mutex_);
        monitors = cached_monitors_;
        show_all = show_on_all_monitors_;
    }

    if (monitors.empty()) {
        spdlog::warn("No monitors available for positioning");
        return;
    }

    spdlog::info("Positioning {} overlays on {} monitors",
                 overlays_.size(), monitors.size());

    const auto count = (std::min)(monitors.size(), overlays_.size());

    for (std::size_t i = 0; i < count; ++i) {
        const auto& monitor = monitors[i];
        auto overlay_handle = *overlays_[i];

        const bool is_portrait = monitor.height > monitor.width;

        spdlog::info(
            "Overlay[{}] target monitor: x={} y={} w={} h={} (scale={}, portrait={})",
            i, monitor.x, monitor.y, monitor.width, monitor.height,
            monitor.scale_factor, is_portrait);

        // Apply Slint sizing
        overlay_handle->window().set_position(
            slint::PhysicalPosition({monitor.x, monitor.y}));
        overlay_handle->window().set_size(
            slint::PhysicalSize({static_cast<uint32_t>(monitor.width),
                                 static_cast<uint32_t>(monitor.height)}));

        // Log what Slint reports back
        const auto reported_pos = overlay_handle->window().position();
        const auto reported_size = overlay_handle->window().size();
        const auto reported_scale = overlay_handle->window().scale_factor();
        spdlog::info(
            "Overlay[{}] Slint reports: pos=({}, {}) size=({}, {}) scale={} (monitor scale={})",
            i, reported_pos.x, reported_pos.y,
            reported_size.width, reported_size.height,
            reported_scale, monitor.scale_factor);

#ifdef _WIN32
        HWND hwnd = static_cast<HWND>(overlay_handle->window().win32_hwnd());
        if (hwnd) {
            StyleOverlayWin32(hwnd, i);

            if (is_portrait) {
                // Portrait monitors: the GPU swapchain is often created at
                // the native (un-rotated) panel dimensions by Slint's skia
                // DirectX backend.  Force a swapchain recreation by toggling
                // the window through an intermediate 1x1 size and then to
                // the correct portrait dimensions.
                ForceSwapchainRecreation(hwnd, i, monitor.x, monitor.y,
                                        monitor.width, monitor.height);
            } else {
                // Landscape: just force correct geometry + repaint.
                ForceWin32Geometry(hwnd, i, monitor.x, monitor.y,
                                   monitor.width, monitor.height);
            }

            LogOverlayWin32Rect(hwnd, i);
        } else {
            spdlog::debug("Overlay[{}] HWND not yet available for styling", i);
        }
#endif
    }

#ifdef _WIN32
    // If any HWND is still missing, schedule a retry.
    bool any_missing_hwnd = false;
    for (std::size_t i = 0; i < count && !any_missing_hwnd; ++i) {
        auto overlay_handle = *overlays_[i];
        if (!overlay_handle->window().win32_hwnd()) {
            any_missing_hwnd = true;
        }
    }

    if (any_missing_hwnd) {
        std::lock_guard lock(overlay_mutex_);
        if (positioning_retry_count_ < 5) {
            positioning_retry_count_++;
            spdlog::info("Scheduling styling retry {} in 150ms (waiting for HWNDs)",
                         positioning_retry_count_);

            position_timer_ = std::make_unique<slint::Timer>();
            position_timer_->start(
                slint::TimerMode::SingleShot,
                std::chrono::milliseconds(150),
                [this]() {
                    PositionOverlaysOnMonitors();
                });
        } else {
            spdlog::warn("Gave up waiting for HWNDs after {} retries", positioning_retry_count_);
            positioning_retry_count_ = 0;
        }
    } else {
        std::lock_guard lock(overlay_mutex_);
        if (positioning_retry_count_ > 0) {
            spdlog::info("All overlay HWNDs obtained after {} retries", positioning_retry_count_);
        }
        positioning_retry_count_ = 0;

        // Schedule a verification pass.
        // For portrait monitors we also do a second swapchain recreation
        // attempt if the first one didn't stick, plus a Slint-level
        // hide → resize → show cycle as a nuclear option.
        position_timer_ = std::make_unique<slint::Timer>();
        position_timer_->start(
            slint::TimerMode::SingleShot,
            std::chrono::milliseconds(250),
            [this, monitors_copy = monitors, overlay_count = count]() {
                VerifyAndFixOverlays(monitors_copy, overlay_count);
            });
    }
#endif
}

#ifdef _WIN32
void OverlayManager::VerifyAndFixOverlays(
    const std::vector<platform::MonitorInfo>& monitors,
    std::size_t overlay_count) {

    spdlog::info("Overlay verification pass ({} overlays)", overlay_count);

    bool any_portrait_needs_fix = false;

    for (std::size_t i = 0; i < overlay_count && i < overlays_.size(); ++i) {
        const auto& mon = monitors[i];
        auto oh = *overlays_[i];
        const auto rp = oh->window().position();
        const auto rs = oh->window().size();
        const auto rscale = oh->window().scale_factor();
        const bool is_portrait = mon.height > mon.width;

        const bool pos_ok = (rp.x == mon.x && rp.y == mon.y);
        const bool size_ok =
            (static_cast<int>(rs.width) == mon.width &&
             static_cast<int>(rs.height) == mon.height);

        if (!pos_ok || !size_ok) {
            spdlog::warn(
                "Overlay[{}] verify MISMATCH: expected pos=({},{}) size=({},{}), "
                "got pos=({},{}) size=({},{}) scale={} portrait={}. Re-applying.",
                i, mon.x, mon.y, mon.width, mon.height,
                rp.x, rp.y, rs.width, rs.height, rscale, is_portrait);
        } else {
            spdlog::info(
                "Overlay[{}] verify OK: pos=({},{}) size=({},{}) scale={} portrait={}",
                i, rp.x, rp.y, rs.width, rs.height, rscale, is_portrait);
        }

        // Check Win32 rect to detect swapchain mismatch
        HWND hwnd = static_cast<HWND>(oh->window().win32_hwnd());
        if (!hwnd) continue;

        RECT rect = {};
        GetWindowRect(hwnd, &rect);
        const int actual_w = rect.right - rect.left;
        const int actual_h = rect.bottom - rect.top;

        spdlog::info("Overlay[{}] Win32 rect: ({},{} {}x{})",
                     i, rect.left, rect.top, actual_w, actual_h);

        if (is_portrait) {
            // For portrait monitors, the swapchain issue may persist after
            // the first toggle.  Try a more aggressive approach:
            //
            // 1. Hide the Slint window (destroys the rendering surface)
            // 2. Force the HWND to the correct dimensions via Win32
            // 3. Re-show the Slint window (creates a new rendering surface)
            //
            // By setting the HWND dimensions WHILE hidden, when Slint
            // re-creates the rendering surface on show(), it should query
            // the HWND's current client rect and create the surface at the
            // correct portrait dimensions.

            spdlog::info(
                "Overlay[{}] portrait: hide → Win32 force → show (surface recreation)",
                i);

            // Hide the Slint window
            oh->hide();

            // Force HWND to correct portrait dimensions while hidden
            SetWindowPos(hwnd, HWND_TOPMOST,
                         mon.x, mon.y, mon.width, mon.height,
                         SWP_NOACTIVATE);

            // Also tell Slint the correct dimensions
            oh->window().set_size(
                slint::PhysicalSize({static_cast<uint32_t>(mon.width),
                                     static_cast<uint32_t>(mon.height)}));
            oh->window().set_position(
                slint::PhysicalPosition({mon.x, mon.y}));

            // Re-show — Slint should create a fresh rendering surface
            oh->show();

            // Re-apply styles (lost during hide)
            StyleOverlayWin32(hwnd, i);

            // Force the HWND geometry again after show
            ForceSwapchainRecreation(hwnd, i, mon.x, mon.y,
                                     mon.width, mon.height);

            LogOverlayWin32Rect(hwnd, i);

            any_portrait_needs_fix = true;
        } else {
            // Landscape: re-apply via Slint and Win32
            oh->window().set_size(
                slint::PhysicalSize({static_cast<uint32_t>(mon.width),
                                     static_cast<uint32_t>(mon.height)}));
            oh->window().set_position(
                slint::PhysicalPosition({mon.x, mon.y}));
            ForceWin32Geometry(hwnd, i, mon.x, mon.y,
                               mon.width, mon.height);
        }
    }

    // For portrait monitors, schedule one more pass to verify the fix stuck
    // and apply a final WM_SIZE if needed
    if (any_portrait_needs_fix) {
        std::lock_guard lock(overlay_mutex_);
        position_timer_ = std::make_unique<slint::Timer>();
        position_timer_->start(
            slint::TimerMode::SingleShot,
            std::chrono::milliseconds(200),
            [this, monitors_copy = monitors, overlay_count]() {
                FinalPortraitPass(monitors_copy, overlay_count);
            });
    }
}

void OverlayManager::FinalPortraitPass(
    const std::vector<platform::MonitorInfo>& monitors,
    std::size_t overlay_count) {

    spdlog::info("Portrait final verification pass");

    for (std::size_t i = 0; i < overlay_count && i < overlays_.size(); ++i) {
        const auto& mon = monitors[i];
        if (mon.height <= mon.width) continue;

        auto oh = *overlays_[i];
        HWND hwnd = static_cast<HWND>(oh->window().win32_hwnd());
        if (!hwnd) continue;

        const auto rp = oh->window().position();
        const auto rs = oh->window().size();
        const auto rscale = oh->window().scale_factor();

        spdlog::info(
            "Overlay[{}] portrait final check: pos=({},{}) size=({},{}) scale={}",
            i, rp.x, rp.y, rs.width, rs.height, rscale);

        RECT rect = {};
        GetWindowRect(hwnd, &rect);
        const int actual_w = rect.right - rect.left;
        const int actual_h = rect.bottom - rect.top;

        spdlog::info("Overlay[{}] portrait Win32 rect: ({},{} {}x{})",
                     i, rect.left, rect.top, actual_w, actual_h);

        // Re-apply Slint sizing
        oh->window().set_size(
            slint::PhysicalSize({static_cast<uint32_t>(mon.width),
                                 static_cast<uint32_t>(mon.height)}));
        oh->window().set_position(
            slint::PhysicalPosition({mon.x, mon.y}));

        // Force Win32 geometry one last time
        ForceWin32Geometry(hwnd, i, mon.x, mon.y, mon.width, mon.height);

        // Send explicit WM_SIZE with the correct portrait dimensions.
        // This is a last-resort attempt to get the rendering backend to
        // resize its internal surface/swapchain.
        // MAKELPARAM packs width in low word, height in high word.
        LPARAM size_param = MAKELPARAM(
            static_cast<WORD>(mon.width),
            static_cast<WORD>(mon.height));
        SendMessage(hwnd, WM_SIZE, SIZE_RESTORED, size_param);
        spdlog::info(
            "Overlay[{}] sent WM_SIZE({}, {})",
            i, mon.width, mon.height);

        InvalidateRect(hwnd, nullptr, TRUE);
        UpdateWindow(hwnd);

        LogOverlayWin32Rect(hwnd, i);
    }
}
#endif  // _WIN32

void OverlayManager::EnsureOverlaysForMonitors(std::size_t monitor_count) {
    const auto required = std::max<std::size_t>(1, monitor_count);

    // Always destroy and recreate overlay windows rather than reusing hidden
    // ones.  The Slint software renderer (winit-software) does not properly
    // reinitialise the render buffer when a previously-hidden window is
    // re-shown, which causes the semi-transparent background Rectangle to
    // not repaint — leaving only the centred text visible.  Recreating is
    // cheap (breaks happen minutes apart) and guarantees a fresh surface.
    if (!overlays_.empty()) {
        spdlog::debug("Destroying {} existing overlay(s) to guarantee fresh render surfaces",
                      overlays_.size());
    }
    overlays_.clear();
    overlays_.reserve(required);

    spdlog::debug("Creating {} fresh overlay window(s)", required);

    for (std::size_t i = 0; i < required; ++i) {
        overlays_.push_back(
            std::make_unique<slint::ComponentHandle<BreakOverlay>>(BreakOverlay::create()));
    }

    for (auto& overlay_ptr : overlays_) {
        if (!overlay_ptr) {
            continue;
        }
        auto overlay_handle = *overlay_ptr;

        overlay_handle->on_skip_clicked([this] {
            std::function<void()> callback;
            {
                std::lock_guard lock(overlay_mutex_);
                callback = on_skip_;
            }
            if (callback) {
                callback();
            }
        });

        overlay_handle->on_snooze_clicked([this] {
            std::function<void()> callback;
            {
                std::lock_guard lock(overlay_mutex_);
                callback = on_snooze_;
            }
            if (callback) {
                callback();
            }
        });
    }

    // Reset retry counter when creating new overlays
    {
        std::lock_guard lock(overlay_mutex_);
        positioning_retry_count_ = 0;
    }
}

void OverlayManager::ApplyStateToOverlays(const std::string& message,
                                          const std::string& break_type,
                                          const std::string& time_remaining,
                                          const std::string& snooze_label,
                                          bool can_skip,
                                          bool can_snooze,
                                          const std::optional<float>& opacity_override,
                                          bool should_show) {
    for (auto& overlay_ptr : overlays_) {
        if (!overlay_ptr) {
            continue;
        }
        auto overlay_handle = *overlay_ptr;
        overlay_handle->set_message(ToSharedString(message));
        overlay_handle->set_break_type(ToSharedString(break_type));
        overlay_handle->set_time_remaining(ToSharedString(time_remaining));
        overlay_handle->set_can_skip(can_skip);
        overlay_handle->set_can_snooze(can_snooze);
        if (!snooze_label.empty()) {
            overlay_handle->set_snooze_label(ToSharedString(snooze_label));
        }

        if (opacity_override) {
            overlay_handle->set_overlay_opacity(
                std::clamp(*opacity_override, 0.0f, 1.0f));
        }

        if (should_show) {
            overlay_handle->show();
        }
    }
}

}  // namespace blinkbreak