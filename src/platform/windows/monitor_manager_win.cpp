/// @file monitor_manager_win.cpp
/// @brief Windows monitor manager implementation.

#ifdef _WIN32

#include "monitor_manager_win.hpp"

#include <spdlog/spdlog.h>

namespace blinkbreak::platform {
namespace {

constexpr wchar_t kMonitorWindowClass[] = L"BlinkBreakMonitorListener";

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int required_size =
        WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()),
                            nullptr, 0, nullptr, nullptr);
    if (required_size <= 0) {
        return {};
    }
    std::string output(static_cast<size_t>(required_size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()),
                        output.data(), required_size, nullptr, nullptr);
    return output;
}

enum class MonitorDpiType : int {
    kEffectiveDpi = 0,
};

using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, MonitorDpiType, UINT*, UINT*);

float GetMonitorScaleFactor(HMONITOR monitor) {
    auto* shcore = LoadLibraryW(L"Shcore.dll");
    if (!shcore) {
        return 1.0f;
    }

    auto* fn = reinterpret_cast<GetDpiForMonitorFn>(
        GetProcAddress(shcore, "GetDpiForMonitor"));
    if (!fn) {
        FreeLibrary(shcore);
        return 1.0f;
    }

    UINT dpi_x = 96;
    UINT dpi_y = 96;
    if (SUCCEEDED(fn(monitor, MonitorDpiType::kEffectiveDpi, &dpi_x, &dpi_y))) {
        FreeLibrary(shcore);
        return static_cast<float>(dpi_x) / 96.0f;
    }

    FreeLibrary(shcore);
    return 1.0f;
}

}  // namespace

MonitorManagerWin::MonitorManagerWin() : message_window_(nullptr) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MonitorManagerWin::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kMonitorWindowClass;
    RegisterClassExW(&wc);

    message_window_ =
        CreateWindowExW(0, kMonitorWindowClass, L"BlinkBreakMonitorListener", 0, 0, 0, 0, 0,
                        HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), this);

    RefreshMonitors();
    spdlog::debug("MonitorManagerWin initialized");
}

MonitorManagerWin::~MonitorManagerWin() {
    StopChangeListener();
    spdlog::debug("MonitorManagerWin destroyed");
}

std::vector<MonitorInfo> MonitorManagerWin::GetMonitors() {
    bool needs_refresh = false;
    {
        std::lock_guard lock(mutex_);
        needs_refresh = cached_monitors_.empty();
        if (!needs_refresh) {
            return cached_monitors_;
        }
    }

    RefreshMonitors();
    std::lock_guard lock(mutex_);
    return cached_monitors_;
}

MonitorInfo MonitorManagerWin::GetPrimaryMonitor() {
    RefreshMonitors();

    std::lock_guard lock(mutex_);
    for (const auto& monitor : cached_monitors_) {
        if (monitor.is_primary) {
            return monitor;
        }
    }
    if (!cached_monitors_.empty()) {
        return cached_monitors_.front();
    }
    return {};
}

void MonitorManagerWin::SetOnMonitorChange(std::function<void()> callback) {
    std::lock_guard lock(mutex_);
    on_change_ = std::move(callback);
}

BOOL CALLBACK MonitorManagerWin::EnumMonitorsProc(HMONITOR monitor, HDC, LPRECT, LPARAM data) {
    auto* context = reinterpret_cast<EnumContext*>(data);
    if (!context) {
        return FALSE;
    }

    MONITORINFOEXW info = {};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info)) {
        return TRUE;
    }

    MonitorInfo entry;
    entry.id = context->next_id++;
    entry.x = info.rcMonitor.left;
    entry.y = info.rcMonitor.top;
    entry.width = info.rcMonitor.right - info.rcMonitor.left;
    entry.height = info.rcMonitor.bottom - info.rcMonitor.top;
    entry.scale_factor = GetMonitorScaleFactor(monitor);
    entry.is_primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;

    std::wstring device_name(info.szDevice);
    entry.name = WideToUtf8(device_name);

    context->monitors.push_back(std::move(entry));
    return TRUE;
}

LRESULT CALLBACK MonitorManagerWin::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        auto* create_struct = reinterpret_cast<CREATESTRUCTW*>(lparam);
        auto* instance = reinterpret_cast<MonitorManagerWin*>(create_struct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(instance));
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    auto* instance =
        reinterpret_cast<MonitorManagerWin*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!instance) {
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    if (msg == WM_DISPLAYCHANGE || msg == WM_DEVICECHANGE) {
        instance->RefreshMonitors();
        std::function<void()> callback;
        {
            std::lock_guard lock(instance->mutex_);
            callback = instance->on_change_;
        }
        if (callback) {
            callback();
        }
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void MonitorManagerWin::RefreshMonitors() {
    EnumContext context;
    EnumDisplayMonitors(nullptr, nullptr, &MonitorManagerWin::EnumMonitorsProc,
                        reinterpret_cast<LPARAM>(&context));

    {
        std::lock_guard lock(mutex_);
        cached_monitors_ = std::move(context.monitors);
    }
}

void MonitorManagerWin::StopChangeListener() {
    if (message_window_) {
        DestroyWindow(message_window_);
        message_window_ = nullptr;
    }
}

}  // namespace blinkbreak::platform

#endif  // _WIN32