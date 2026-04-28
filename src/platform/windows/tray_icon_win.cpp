#ifdef _WIN32

    #include "tray_icon_win.hpp"

    #include <CommCtrl.h>
    #include <spdlog/spdlog.h>

    #include "../resources/resource.h"

namespace blinkbreak {
namespace platform {

HICON TrayIconWin::LoadIconResource(int resource_id) {
    const int cx = GetSystemMetrics(SM_CXSMICON);
    const int cy = GetSystemMetrics(SM_CYSMICON);

    HICON icon =
        static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(resource_id),
                                      IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR));

    if (!icon) {
        spdlog::warn("LoadImageW failed for resource id {} ({}x{}), error={}", resource_id, cx, cy,
                     GetLastError());
    }
    return icon;
}

namespace {
constexpr GUID kTrayIconGuid = {
    0x9f6299e7, 0xe91a, 0x4aca, {0x81, 0xc6, 0xb6, 0xb8, 0x4d, 0x1f, 0x44, 0xaa}};
constexpr UINT_PTR kTraySubclassId = 0x42425452;  // 'BBTR'

NOTIFYICONDATAW BuildNotifyIconData(const NOTIFYICONDATAW& source, DWORD flags) {
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = source.hWnd;
    data.uID = source.uID;
    data.uFlags = flags;
    data.uCallbackMessage = source.uCallbackMessage;
    data.hIcon = source.hIcon;
    data.uVersion = source.uVersion;
    data.guidItem = source.guidItem;
    if ((flags & NIF_TIP) != 0) {
        wcsncpy_s(data.szTip, source.szTip, _TRUNCATE);
    }
    return data;
}
}  // namespace

TrayIconWin::TrayIconWin()
    : host_window_(nullptr),
      hmenu_(nullptr),
      is_visible_(false),
      subclass_installed_(false),
      icon_running_(nullptr),
      icon_paused_(nullptr) {
    icon_running_ = LoadIconResource(IDI_APP_BLUE);
    icon_paused_ = LoadIconResource(IDI_APP_YELLOW);

    ZeroMemory(&nid_, sizeof(nid_));
    nid_.cbSize = sizeof(nid_);
    nid_.uID = 1;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_GUID;
    nid_.uCallbackMessage = kWmTrayIcon;
    nid_.uVersion = NOTIFYICON_VERSION_4;
    nid_.hIcon = icon_paused_ ? icon_paused_ : icon_running_;
    nid_.guidItem = kTrayIconGuid;
    wcscpy_s(nid_.szTip, L"BlinkBreak");

    spdlog::debug("TrayIconWin created");
}

TrayIconWin::~TrayIconWin() {
    Hide();

    if (subclass_installed_ && host_window_) {
        RemoveWindowSubclass(host_window_, HostWindowSubclassProc, kTraySubclassId);
        subclass_installed_ = false;
    }

    if (hmenu_) {
        DestroyMenu(hmenu_);
    }
    if (icon_running_) {
        DestroyIcon(icon_running_);
        icon_running_ = nullptr;
    }
    if (icon_paused_) {
        DestroyIcon(icon_paused_);
        icon_paused_ = nullptr;
    }
    spdlog::debug("TrayIconWin destroyed");
}

bool TrayIconWin::Show() {
    if (is_visible_) {
        return true;
    }
    if (!nid_.hWnd) {
        spdlog::warn("Tray icon cannot be shown before host window is assigned");
        return false;
    }

    auto add_data = BuildNotifyIconData(nid_, NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_GUID);
    if (!Shell_NotifyIconW(NIM_ADD, &add_data)) {
        spdlog::error("Failed to show tray icon");
        return false;
    }

    auto version_data = BuildNotifyIconData(nid_, 0);
    Shell_NotifyIconW(NIM_SETVERSION, &version_data);
    is_visible_ = true;
    spdlog::info("Tray icon shown");
    return true;
}

void TrayIconWin::Hide() {
    if (!is_visible_) {
        return;
    }

    auto delete_data = BuildNotifyIconData(nid_, 0);
    Shell_NotifyIconW(NIM_DELETE, &delete_data);
    is_visible_ = false;
    spdlog::info("Tray icon hidden");
}

void TrayIconWin::SetTooltip(const std::string& tooltip) {
    std::wstring wide_tooltip(tooltip.begin(), tooltip.end());
    wcsncpy_s(nid_.szTip, wide_tooltip.c_str(), _TRUNCATE);

    if (is_visible_) {
        auto modify_data = BuildNotifyIconData(nid_, NIF_TIP | NIF_GUID);
        Shell_NotifyIconW(NIM_MODIFY, &modify_data);
    }
}

void TrayIconWin::SetMenu(const std::vector<MenuItem>& items) {
    menu_items_ = items;

    if (hmenu_) {
        DestroyMenu(hmenu_);
    }
    hmenu_ = CreatePopupMenu();

    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        if (item.is_separator) {
            AppendMenuW(hmenu_, MF_SEPARATOR, 0, nullptr);
            continue;
        }

        std::wstring wide_text(item.text.begin(), item.text.end());
        UINT flags = MF_STRING;
        if (!item.enabled) {
            flags |= MF_GRAYED;
        }
        if (item.checked) {
            flags |= MF_CHECKED;
        }
        AppendMenuW(hmenu_, flags, kFirstMenuItemId + static_cast<UINT>(i), wide_text.c_str());
    }
}

void TrayIconWin::SetOnClick(std::function<void()> callback) {
    on_click_ = std::move(callback);
}

void TrayIconWin::SetOnDoubleClick(std::function<void()> callback) {
    on_double_click_ = std::move(callback);
}

void TrayIconWin::SetHostWindow(std::uintptr_t native_window_handle) {
    HWND new_host = reinterpret_cast<HWND>(native_window_handle);
    if (host_window_ == new_host) {
        return;
    }

    if (subclass_installed_ && host_window_) {
        RemoveWindowSubclass(host_window_, HostWindowSubclassProc, kTraySubclassId);
        subclass_installed_ = false;
    }

    host_window_ = new_host;
    nid_.hWnd = host_window_;

    if (!host_window_) {
        return;
    }

    if (!SetWindowSubclass(host_window_, HostWindowSubclassProc, kTraySubclassId,
                           reinterpret_cast<DWORD_PTR>(this))) {
        spdlog::error("SetWindowSubclass failed, error={}", GetLastError());
        host_window_ = nullptr;
        nid_.hWnd = nullptr;
        return;
    }

    subclass_installed_ = true;
}

void TrayIconWin::SetIcon(int icon_id) {
    HICON selected_icon = nullptr;
    if (icon_id == kIconPaused) {
        selected_icon = icon_paused_;
    } else if (icon_id == kIconRunning) {
        selected_icon = icon_running_;
    }

    if (!selected_icon || nid_.hIcon == selected_icon) {
        return;
    }

    nid_.hIcon = selected_icon;
    if (is_visible_) {
        auto modify_data = BuildNotifyIconData(nid_, NIF_ICON | NIF_GUID);
        Shell_NotifyIconW(NIM_MODIFY, &modify_data);
    }
}

LRESULT CALLBACK TrayIconWin::HostWindowSubclassProc(HWND hwnd, UINT msg, WPARAM wparam,
                                                     LPARAM lparam, UINT_PTR subclass_id,
                                                     DWORD_PTR ref_data) {
    auto* self = reinterpret_cast<TrayIconWin*>(ref_data);
    if (!self) {
        return DefSubclassProc(hwnd, msg, wparam, lparam);
    }

    if (msg == kWmTrayIcon) {
        switch (LOWORD(lparam)) {
            case WM_LBUTTONUP:
                if (self->on_click_) {
                    self->on_click_();
                }
                return 0;
            case WM_LBUTTONDBLCLK:
                if (self->on_double_click_) {
                    self->on_double_click_();
                }
                return 0;
            case WM_RBUTTONUP:
                self->ShowContextMenu();
                return 0;
        }
    }

    if (msg == WM_COMMAND) {
        const UINT id = LOWORD(wparam);
        if (id >= kFirstMenuItemId) {
            const size_t index = static_cast<size_t>(id - kFirstMenuItemId);
            if (index < self->menu_items_.size()) {
                const auto& item = self->menu_items_[index];
                if (item.callback && item.enabled) {
                    item.callback();
                }
                return 0;
            }
        }
    }

    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(hwnd, HostWindowSubclassProc, subclass_id);
        self->subclass_installed_ = false;
        self->host_window_ = nullptr;
        self->nid_.hWnd = nullptr;
    }

    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

void TrayIconWin::ShowContextMenu() {
    if (!hmenu_ || !host_window_) {
        return;
    }

    POINT pt;
    GetCursorPos(&pt);

    SetForegroundWindow(host_window_);
    TrackPopupMenu(hmenu_, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, host_window_, nullptr);
    PostMessage(host_window_, WM_NULL, 0, 0);
}

std::unique_ptr<ITrayIcon> CreateTrayIcon() {
    return std::make_unique<TrayIconWin>();
}

}  // namespace platform
}  // namespace blinkbreak

#endif  // _WIN32
