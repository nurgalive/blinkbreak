/// @file dnd_detector_win.hpp
/// @brief Windows Do Not Disturb detection using SHQueryUserNotificationState.

#ifndef BLINKBREAK_PLATFORM_DND_DETECTOR_WIN_HPP
#define BLINKBREAK_PLATFORM_DND_DETECTOR_WIN_HPP

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>

#include "platform/platform_interface.hpp"

namespace blinkbreak::platform {

/// @brief Windows implementation of IDndDetector using FocusSessionManager
/// and SHQueryUserNotificationState.
///
/// This class monitors the Windows DND/Focus Assist state by polling the
/// Windows 11 FocusSessionManager API when available and falling back to
/// SHQueryUserNotificationState for presentation/full-screen legacy states.
class DndDetectorWin : public IDndDetector {
public:
    DndDetectorWin();
    ~DndDetectorWin() override;

    // Non-copyable, non-movable
    DndDetectorWin(const DndDetectorWin&) = delete;
    DndDetectorWin& operator=(const DndDetectorWin&) = delete;
    DndDetectorWin(DndDetectorWin&&) = delete;
    DndDetectorWin& operator=(DndDetectorWin&&) = delete;

    void Start() override;
    void Stop() override;
    [[nodiscard]] bool IsRunning() const override;
    [[nodiscard]] DndState GetState() const override;
    [[nodiscard]] DndState RefreshState() override;
    [[nodiscard]] bool IsDndActive() const override;
    [[nodiscard]] bool IsFullScreenDetected() const override;
    void SetOnDndChange(std::function<void(bool)> callback) override;
    void SetPollingInterval(std::chrono::milliseconds interval) override;
    [[nodiscard]] std::chrono::milliseconds GetPollingInterval() const override;

private:
    /// @brief Background thread function for polling DND state.
    void PollThread();

    /// @brief Queries the current DND state from Windows API.
    /// @return The current DndState.
    [[nodiscard]] DndState QueryState() const;

    /// @brief Checks if Windows 11 Focus mode is active.
    ///
    /// This prefers the documented FocusSessionManager API and only falls back
    /// to the registry heuristic when the runtime API is unavailable.
    /// @return True if Focus mode is active.
    [[nodiscard]] bool IsWindows11FocusActive() const;

    /// @brief Queries the Windows 11 FocusSessionManager API.
    /// @param is_active Receives the current Focus active flag.
    /// @return True if the API was available and queried successfully.
    [[nodiscard]] bool TryGetWindows11FocusActive(bool& is_active) const;

    /// @brief Fallback registry heuristic for Windows 11 Focus mode.
    /// @return True if the legacy registry heuristic indicates Focus activity.
    [[nodiscard]] bool IsWindows11FocusActiveViaRegistry() const;

    /// @brief Checks if Windows 11 Do Not Disturb is active via Cloud Data Store.
    /// @return True if a non-Unrestricted quiet hours profile is currently selected.
    [[nodiscard]] bool IsWindows11DoNotDisturbActive() const;

    /// @brief Reads the current quiet hours profile from Cloud Data Store.
    /// @param selected_profile Receives the selected profile string.
    /// @return True if the query succeeded.
    [[nodiscard]] bool TryGetQuietHoursSelectedProfile(std::string& selected_profile) const;

    /// @brief Converts a DndState into the corresponding shell API reason string.
    /// @param state The state to describe.
    /// @return Human-readable reason string.
    [[nodiscard]] static const char* QueryReasonToString(DndState state);

    std::atomic<bool> running_{false};
    std::atomic<DndState> current_state_{DndState::AcceptsNotifications};
    std::atomic<bool> last_dnd_active_{false};
    std::atomic<std::chrono::milliseconds> polling_interval_{std::chrono::milliseconds{1000}};
    std::function<void(bool)> on_dnd_change_;
    std::thread poll_thread_;
    mutable std::mutex mutex_;
};

}  // namespace blinkbreak::platform

#endif  // BLINKBREAK_PLATFORM_DND_DETECTOR_WIN_HPP
