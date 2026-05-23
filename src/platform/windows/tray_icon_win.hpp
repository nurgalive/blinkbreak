/// @file tray_icon_win.hpp
/// @brief Windows-specific system tray icon implementation.

#ifndef BLINKBREAK_PLATFORM_WINDOWS_TRAY_ICON_WIN_HPP
#define BLINKBREAK_PLATFORM_WINDOWS_TRAY_ICON_WIN_HPP

#include "../platform_interface.hpp"

#ifdef _WIN32

  #include <Windows.h>
  #include <commctrl.h>
  #include <shellapi.h>

namespace blinkbreak
{
namespace platform
{

/// @brief Windows implementation of ITrayIcon.
///
/// Icons are loaded from Win32 resources embedded via the .rc file and the
/// tray attaches to BlinkBreak's existing main window instead of creating a
/// separate hidden host window. The tray uses NOTIFYICON_VERSION_4 for Windows
/// 7+ compatibility and sizes icons with GetSystemMetrics(SM_CXSMICON).
class TrayIconWin : public ITrayIcon
{
public:
  /// @brief Constructs a Windows tray icon.
  TrayIconWin();

  /// @brief Destructor - removes icon from tray.
  ~TrayIconWin() override;

  bool Show() override;
  void Hide() override;
  void SetTooltip(const std::string& tooltip) override;
  void SetMenu(const std::vector<MenuItem>& items) override;
  void SetOnClick(std::function<void()> callback) override;
  void SetOnDoubleClick(std::function<void()> callback) override;
  void SetHostWindow(std::uintptr_t native_window_handle) override;
  void SetIcon(int icon_id) override;

private:
  /// @brief Subclass procedure installed on the main window for tray events.
  static LRESULT CALLBACK HostWindowSubclassProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                                 UINT_PTR subclass_id, DWORD_PTR ref_data);

  /// @brief Shows the context menu at cursor position.
  void ShowContextMenu();

  /// @brief Loads an icon resource at the system-preferred small-icon size.
  /// @param resource_id  Resource identifier (e.g. IDI_APP_BLUE).
  /// @return Handle to the loaded icon, or nullptr on failure.
  static HICON LoadIconResource(int resource_id);

  HWND host_window_;                       ///< Native application window used for tray callbacks.
  NOTIFYICONDATAW nid_;                    ///< Tray icon data.
  HMENU hmenu_;                            ///< Context menu handle.
  std::vector<MenuItem> menu_items_;       ///< Current menu items.
  std::function<void()> on_click_;         ///< Left-click callback.
  std::function<void()> on_double_click_;  ///< Double-click callback.
  bool is_visible_;                        ///< Whether icon is visible.
  bool subclass_installed_;                ///< Whether the host window subclass is active.

  HICON icon_running_;  ///< Cached icon for running state (blue).
  HICON icon_paused_;   ///< Cached icon for paused state (yellow).

  static constexpr UINT kWmTrayIcon = WM_USER + 1;
  static constexpr UINT kFirstMenuItemId = 40000;
  static constexpr int kIconPaused = 0;
  static constexpr int kIconRunning = 1;
};

}  // namespace platform
}  // namespace blinkbreak

#endif  // _WIN32

#endif  // BLINKBREAK_PLATFORM_WINDOWS_TRAY_ICON_WIN_HPP
