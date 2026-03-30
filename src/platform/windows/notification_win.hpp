/// @file notification_win.hpp
/// @brief Windows toast notification implementation using WinToast.

#ifndef BLINKBREAK_PLATFORM_NOTIFICATION_WIN_HPP
#define BLINKBREAK_PLATFORM_NOTIFICATION_WIN_HPP

#include <cstdint>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "platform/platform_interface.hpp"

namespace blinkbreak::platform {

/// @brief Windows implementation of INotificationManager using WinToast.
class NotificationManagerWin : public INotificationManager {
public:
    NotificationManagerWin();
    ~NotificationManagerWin() override;

    // Non-copyable, non-movable
    NotificationManagerWin(const NotificationManagerWin&) = delete;
    NotificationManagerWin& operator=(const NotificationManagerWin&) = delete;
    NotificationManagerWin(NotificationManagerWin&&) = delete;
    NotificationManagerWin& operator=(NotificationManagerWin&&) = delete;

    bool Initialize() override;
    int64_t Show(const std::string& title, const std::string& message) override;
    void Hide(int64_t toast_id) override;
    void SetOnAction(std::function<void(NotificationAction)> callback) override;
    [[nodiscard]] bool IsSupported() const override;

private:
    void EnsureWorker();
    void StopWorker();
    void EnqueueTask(std::function<void()> task);

    bool initialized_ = false;
    std::function<void(NotificationAction)> on_action_;
    mutable std::mutex mutex_;

    std::thread worker_thread_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::queue<std::function<void()>> tasks_;
    std::atomic<bool> worker_running_{false};
    bool worker_started_ = false;
};

}  // namespace blinkbreak::platform

#endif  // BLINKBREAK_PLATFORM_NOTIFICATION_WIN_HPP
