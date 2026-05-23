/// @file dnd_detector_win.cpp
/// @brief Windows Do Not Disturb detection implementation.

#include "platform/windows/dnd_detector_win.hpp"

#include <array>
#include <cstdio>
#include <string>

#include <spdlog/spdlog.h>

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <shellapi.h>
#include <winrt/Windows.UI.Shell.h>

namespace blinkbreak::platform
{

// Windows 11 Focus Active Sessions registry path
// This key contains data when a Focus session is active
static constexpr const wchar_t* kWin11FocusRegPath =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\CloudStore\\Store\\"
    L"DefaultAccount\\Current\\default$windows.data.shell.focusactivesessions\\"
    L"windows.data.shell.focusactivesessions";

DndDetectorWin::DndDetectorWin()
{
  // Delay the first Focus/COM-backed probe until the polling thread starts.
  // This avoids touching COM on the main UI thread before Slint/winit finishes
  // its own apartment initialization.
  current_state_.store(DndState::AcceptsNotifications);
  last_dnd_active_.store(false);
  spdlog::debug("DndDetectorWin: initialized with deferred state probe");
}

DndDetectorWin::~DndDetectorWin()
{
  Stop();
}

void DndDetectorWin::Start()
{
  if (running_.load())
  {
    spdlog::debug("DndDetectorWin: already running");
    return;
  }

  running_.store(true);
  last_dnd_active_.store(current_state_.load() != DndState::AcceptsNotifications);
  poll_thread_ = std::thread(&DndDetectorWin::PollThread, this);
  spdlog::info("DndDetectorWin: started monitoring");
}

void DndDetectorWin::Stop()
{
  if (!running_.load())
  {
    return;
  }

  running_.store(false);
  if (poll_thread_.joinable())
  {
    poll_thread_.join();
  }
  spdlog::info("DndDetectorWin: stopped monitoring");
}

bool DndDetectorWin::IsRunning() const
{
  return running_.load();
}

DndState DndDetectorWin::GetState() const
{
  return current_state_.load();
}

DndState DndDetectorWin::RefreshState()
{
  const DndState new_state = QueryState();
  const DndState old_state = current_state_.exchange(new_state);

  const bool new_dnd_active = (new_state != DndState::AcceptsNotifications);
  const bool old_dnd_active = last_dnd_active_.exchange(new_dnd_active);

  if (new_dnd_active != old_dnd_active || new_state != old_state)
  {
    spdlog::info("DndDetectorWin: DND refresh observed state {} -> {} (active: {})",
                 DndStateToString(old_state), DndStateToString(new_state), new_dnd_active);
  }

  return new_state;
}

bool DndDetectorWin::IsDndActive() const
{
  return current_state_.load() != DndState::AcceptsNotifications;
}

bool DndDetectorWin::IsFullScreenDetected() const
{
  const auto state = current_state_.load();
  return state == DndState::Busy || state == DndState::FullScreenD3D ||
         state == DndState::WindowsStoreApp;
}

void DndDetectorWin::SetOnDndChange(std::function<void(bool)> callback)
{
  std::lock_guard<std::mutex> lock(mutex_);
  on_dnd_change_ = std::move(callback);
}

void DndDetectorWin::SetPollingInterval(std::chrono::milliseconds interval)
{
  polling_interval_.store(interval);
}

std::chrono::milliseconds DndDetectorWin::GetPollingInterval() const
{
  return polling_interval_.load();
}

void DndDetectorWin::PollThread()
{
  spdlog::debug("DndDetectorWin: poll thread started");

  while (running_.load())
  {
    // Query current state
    const DndState new_state = QueryState();
    const DndState old_state = current_state_.exchange(new_state);

    // Check if DND active state changed
    const bool new_dnd_active = (new_state != DndState::AcceptsNotifications);
    const bool old_dnd_active = last_dnd_active_.exchange(new_dnd_active);

    if (new_dnd_active != old_dnd_active)
    {
      spdlog::info("DndDetectorWin: DND state changed from {} to {} (active: {})",
                   DndStateToString(old_state), DndStateToString(new_state), new_dnd_active);

      // Invoke callback
      std::function<void(bool)> callback;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        callback = on_dnd_change_;
      }
      if (callback)
      {
        try
        {
          callback(new_dnd_active);
        }
        catch (const std::exception& e)
        {
          spdlog::error("DndDetectorWin: callback threw exception: {}", e.what());
        }
      }
    }

    // Sleep for polling interval
    const auto interval = polling_interval_.load();
    std::this_thread::sleep_for(interval);
  }

  spdlog::debug("DndDetectorWin: poll thread exiting");
}

DndState DndDetectorWin::QueryState() const
{
  // Check the documented Windows 11 Focus API first.
  // SHQueryUserNotificationState does not reliably report Win11 Focus state.
  if (IsWindows11FocusActive())
  {
    return DndState::FocusActive;
  }

  // Check the Windows 11 Do Not Disturb profile next.
  if (IsWindows11DoNotDisturbActive())
  {
    return DndState::DoNotDisturbActive;
  }

  // Fall back to legacy SHQueryUserNotificationState for other DND modes
  QUERY_USER_NOTIFICATION_STATE state = QUNS_ACCEPTS_NOTIFICATIONS;
  const HRESULT result = SHQueryUserNotificationState(&state);

  if (FAILED(result))
  {
    spdlog::warn("DndDetectorWin: SHQueryUserNotificationState failed: 0x{:08x}",
                 static_cast<unsigned int>(result));
    return DndState::AcceptsNotifications;  // Fail-open
  }

  switch (state)
  {
    case QUNS_NOT_PRESENT:
      return DndState::NotPresent;
    case QUNS_BUSY:
      return DndState::Busy;
    case QUNS_RUNNING_D3D_FULL_SCREEN:
      return DndState::FullScreenD3D;
    case QUNS_PRESENTATION_MODE:
      return DndState::PresentationMode;
    case QUNS_ACCEPTS_NOTIFICATIONS:
      return DndState::AcceptsNotifications;
    case QUNS_QUIET_TIME:
      return DndState::QuietTime;
    case QUNS_APP:
      return DndState::WindowsStoreApp;
    default:
      spdlog::warn("DndDetectorWin: unknown notification state: {}", static_cast<int>(state));
      return DndState::AcceptsNotifications;
  }
}

bool DndDetectorWin::IsWindows11FocusActive() const
{
  bool is_active = false;
  if (TryGetWindows11FocusActive(is_active))
  {
    return is_active;
  }

  return IsWindows11FocusActiveViaRegistry();
}

bool DndDetectorWin::TryGetWindows11FocusActive(bool& is_active) const
{
  is_active = false;

  try
  {
    winrt::init_apartment(winrt::apartment_type::single_threaded);
  }
  catch (const winrt::hresult_error& e)
  {
    if (e.code() != RPC_E_CHANGED_MODE)
    {
      spdlog::trace("DndDetectorWin: winrt::init_apartment failed: 0x{:08x}",
                    static_cast<unsigned int>(e.code().value));
      return false;
    }
  }

  try
  {
    if (!winrt::Windows::UI::Shell::FocusSessionManager::IsSupported())
    {
      return false;
    }

    const auto manager = winrt::Windows::UI::Shell::FocusSessionManager::GetDefault();
    is_active = manager.IsFocusActive();
    spdlog::trace("DndDetectorWin: FocusSessionManager reported IsFocusActive={}", is_active);
    return true;
  }
  catch (const winrt::hresult_error& e)
  {
    spdlog::trace("DndDetectorWin: FocusSessionManager query failed: 0x{:08x}",
                  static_cast<unsigned int>(e.code().value));
    return false;
  }
}

bool DndDetectorWin::IsWindows11FocusActiveViaRegistry() const
{
  HKEY hKey = nullptr;
  LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, kWin11FocusRegPath, 0, KEY_READ, &hKey);

  if (result != ERROR_SUCCESS)
  {
    // Key doesn't exist or can't be opened - Focus is not active
    return false;
  }

  // Check the "Data" value
  // When Focus is INACTIVE: Data exists but is small (header only)
  // When Focus is ACTIVE: Data is larger (header + session data)
  // After testing, the inactive state can have up to ~50 bytes
  // Active sessions typically have 100+ bytes
  DWORD dataSize = 0;
  result = RegQueryValueExW(hKey, L"Data", nullptr, nullptr, nullptr, &dataSize);

  RegCloseKey(hKey);

  // Use a high threshold to avoid false positives
  // Testing shows inactive state can have ~50 bytes, active has 100+
  const DWORD kMinActiveDataSize = 100;
  const bool isActive = (result == ERROR_SUCCESS && dataSize >= kMinActiveDataSize);

  // Log at trace level to avoid spam during polling
  spdlog::trace(
      "DndDetectorWin: Win11 Focus registry data size: {} bytes (threshold: {}, detected: {})",
      dataSize, kMinActiveDataSize, isActive);

  return isActive;
}

bool DndDetectorWin::IsWindows11DoNotDisturbActive() const
{
  std::string selected_profile;
  if (!TryGetQuietHoursSelectedProfile(selected_profile))
  {
    return false;
  }

  const bool is_active =
      !selected_profile.empty() && selected_profile.find("Unrestricted") == std::string::npos;
  spdlog::trace("DndDetectorWin: QuietHours selectedProfile='{}' active={}", selected_profile,
                is_active);
  return is_active;
}

bool DndDetectorWin::TryGetQuietHoursSelectedProfile(std::string& selected_profile) const
{
  selected_profile.clear();

  FILE* pipe = _popen(
      "\"C:\\Windows\\System32\\readCloudDataSettings.exe\" enum "
      "-type:Windows.Data.DoNotDisturb.QuietHoursSettings",
      "r");
  if (!pipe)
  {
    spdlog::warn("DndDetectorWin: failed to launch readCloudDataSettings.exe");
    return false;
  }

  std::array<char, 512> buffer{};
  std::string output;
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
  {
    output += buffer.data();
  }

  const int exit_code = _pclose(pipe);
  if (exit_code != 0)
  {
    spdlog::trace("DndDetectorWin: readCloudDataSettings.exe exited with {}", exit_code);
    return false;
  }

  const std::string key = R"("selectedProfile":")";
  const std::size_t start = output.find(key);
  if (start == std::string::npos)
  {
    spdlog::trace("DndDetectorWin: selectedProfile not found in quiet hours output");
    return false;
  }

  const std::size_t value_start = start + key.size();
  const std::size_t value_end = output.find('"', value_start);
  if (value_end == std::string::npos)
  {
    spdlog::trace("DndDetectorWin: selectedProfile parse failed");
    return false;
  }

  selected_profile = output.substr(value_start, value_end - value_start);
  return true;
}

const char* DndDetectorWin::QueryReasonToString(DndState state)
{
  switch (state)
  {
    case DndState::FocusActive:
      return "FocusSessionManager";
    case DndState::DoNotDisturbActive:
      return "QuietHoursSettings.selectedProfile";
    case DndState::Busy:
    case DndState::FullScreenD3D:
    case DndState::PresentationMode:
    case DndState::NotPresent:
    case DndState::QuietTime:
    case DndState::WindowsStoreApp:
      return "SHQueryUserNotificationState";
    case DndState::AcceptsNotifications:
      return "None";
  }

  return "Unknown";
}

// Factory function
std::unique_ptr<IDndDetector> CreateDndDetector()
{
  return std::make_unique<DndDetectorWin>();
}

}  // namespace blinkbreak::platform
