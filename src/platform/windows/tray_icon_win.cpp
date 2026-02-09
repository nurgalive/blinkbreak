/// @file tray_icon_win.cpp
/// @brief Windows-specific system tray icon implementation.

#ifdef _WIN32

#include "tray_icon_win.hpp"

#include <spdlog/spdlog.h>

namespace {

constexpr DWORD kGreenInner = 0xFF4CAF50;
constexpr DWORD kGreenOuter = 0xFF2E7D32;
constexpr DWORD kYellowInner = 0xFFFBC02D;
constexpr DWORD kYellowOuter = 0xFFF9A825;

HICON CreateSimpleIconWithColors(DWORD inner_color, DWORD outer_color) {
    const int size = 16;
    HDC hdc = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(hdc);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = size;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hbm = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HBITMAP hbmMask = CreateBitmap(size, size, 1, 1, nullptr);

    if (hbm && bits) {
        auto* pixels = static_cast<DWORD*>(bits);
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                int dx = x - size / 2;
                int dy = y - size / 2;
                int dist = dx * dx + dy * dy;

                if (dist < 36) {
                    pixels[y * size + x] = inner_color;
                } else if (dist < 64) {
                    pixels[y * size + x] = outer_color;
                } else {
                    pixels[y * size + x] = 0x00000000;
                }
            }
        }
    }

    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmMask = hbmMask;
    ii.hbmColor = hbm;

    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hbm);
    DeleteObject(hbmMask);
    DeleteDC(memDC);
    ReleaseDC(nullptr, hdc);

    return hIcon;
}

}  // namespace

namespace blinkbreak {
namespace platform {

TrayIconWin::TrayIconWin()
    : hwnd_(nullptr),
      hmenu_(nullptr),
      is_visible_(false),
      icon_running_(nullptr),
      icon_paused_(nullptr) {
    instance_ = this;

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"BlinkBreakTrayClass";
    RegisterClassExW(&wc);

    // Create hidden window
    hwnd_ = CreateWindowExW(
        0, L"BlinkBreakTrayClass", L"BlinkBreak",
        0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
        GetModuleHandle(nullptr), nullptr
    );

    // Initialize tray icon data
    ZeroMemory(&nid_, sizeof(nid_));
    nid_.cbSize = sizeof(nid_);
    nid_.hWnd = hwnd_;
    nid_.uID = 1;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = kWmTrayIcon;
    icon_running_ = CreateSimpleIconWithColors(kGreenInner, kGreenOuter);
    icon_paused_ = CreateSimpleIconWithColors(kYellowInner, kYellowOuter);
    nid_.hIcon = icon_paused_ ? icon_paused_ : icon_running_;
    wcscpy_s(nid_.szTip, L"BlinkBreak");

    spdlog::debug("TrayIconWin created");
}

TrayIconWin::~TrayIconWin() {
    Hide();
    if (icon_running_) {
        DestroyIcon(icon_running_);
        icon_running_ = nullptr;
    }
    if (icon_paused_) {
        DestroyIcon(icon_paused_);
        icon_paused_ = nullptr;
    }
    nid_.hIcon = nullptr;
    if (hmenu_) {
        DestroyMenu(hmenu_);
    }
    if (hwnd_) {
        DestroyWindow(hwnd_);
    }
    instance_ = nullptr;
    spdlog::debug("TrayIconWin destroyed");
}

bool TrayIconWin::Show() {
    if (is_visible_) {
        return true;
    }

    if (Shell_NotifyIconW(NIM_ADD, &nid_)) {
        is_visible_ = true;
        spdlog::info("Tray icon shown");
        return true;
    }

    spdlog::error("Failed to show tray icon");
    return false;
}

void TrayIconWin::Hide() {
    if (!is_visible_) {
        return;
    }

    Shell_NotifyIconW(NIM_DELETE, &nid_);
    is_visible_ = false;
    spdlog::info("Tray icon hidden");
}

void TrayIconWin::SetTooltip(const std::string& tooltip) {
    std::wstring wide_tooltip(tooltip.begin(), tooltip.end());
    wcsncpy_s(nid_.szTip, wide_tooltip.c_str(), _TRUNCATE);

    if (is_visible_) {
        Shell_NotifyIconW(NIM_MODIFY, &nid_);
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
        } else {
            std::wstring wide_text(item.text.begin(), item.text.end());
            UINT flags = MF_STRING;
            if (!item.enabled) flags |= MF_GRAYED;
            if (item.checked) flags |= MF_CHECKED;
            AppendMenuW(hmenu_, flags, i + 1, wide_text.c_str());
        }
    }
}

void TrayIconWin::SetOnClick(std::function<void()> callback) {
    on_click_ = std::move(callback);
}

void TrayIconWin::SetOnDoubleClick(std::function<void()> callback) {
    on_double_click_ = std::move(callback);
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
        Shell_NotifyIconW(NIM_MODIFY, &nid_);
    }
}

LRESULT CALLBACK TrayIconWin::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == kWmTrayIcon && instance_) {
        switch (LOWORD(lparam)) {
            case WM_LBUTTONUP:
                if (instance_->on_click_) {
                    instance_->on_click_();
                }
                break;
            case WM_LBUTTONDBLCLK:
                if (instance_->on_double_click_) {
                    instance_->on_double_click_();
                }
                break;
            case WM_RBUTTONUP:
                instance_->ShowContextMenu();
                break;
        }
        return 0;
    }

    if (msg == WM_COMMAND && instance_) {
        UINT id = LOWORD(wparam);
        if (id > 0 && id <= instance_->menu_items_.size()) {
            const auto& item = instance_->menu_items_[id - 1];
            if (item.callback && item.enabled) {
                item.callback();
            }
        }
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void TrayIconWin::ShowContextMenu() {
    if (!hmenu_) return;

    POINT pt;
    GetCursorPos(&pt);

    SetForegroundWindow(hwnd_);
    TrackPopupMenu(hmenu_, TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                   pt.x, pt.y, 0, hwnd_, nullptr);
    PostMessage(hwnd_, WM_NULL, 0, 0);
}

HICON TrayIconWin::CreateSimpleIcon(COLORREF inner_color, COLORREF outer_color) {
    return CreateSimpleIconWithColors(static_cast<DWORD>(inner_color),
                                      static_cast<DWORD>(outer_color));
}

// Factory function
std::unique_ptr<ITrayIcon> CreateTrayIcon() {
    return std::make_unique<TrayIconWin>();
}

}  // namespace platform
}  // namespace blinkbreak

#endif  // _WIN32