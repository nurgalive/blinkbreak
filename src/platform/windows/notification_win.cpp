/// @file notification_win.cpp
/// @brief Windows toast notification implementation using WinToast.

#include "platform/windows/notification_win.hpp"

#include <spdlog/spdlog.h>
#include <wintoastlib.h>

#include <future>
#include <memory>
#include <string>
#include <utility>

#ifdef _WIN32
    #include <windows.h>
#endif

namespace blinkbreak::platform {
namespace {

std::wstring Utf8ToWide(const std::string& input) {
#ifdef _WIN32
    if (input.empty()) {
        return std::wstring();
    }
    const int size_needed =
        MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0);
    if (size_needed <= 0) {
        return std::wstring(input.begin(), input.end());
    }
    std::wstring output(static_cast<size_t>(size_needed), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), output.data(),
                        size_needed);
    return output;
#else
    return std::wstring(input.begin(), input.end());
#endif
}

class BlinkBreakToastHandler : public WinToastLib::IWinToastHandler {
public:
    explicit BlinkBreakToastHandler(
        std::shared_ptr<std::function<void(NotificationAction)>> callback)
        : callback_(std::move(callback)) {}

    void toastActivated() const override {
        Dispatch(NotificationAction::Clicked);
    }

    void toastActivated(int action_index) const override {
        if (action_index == 0) {
            Dispatch(NotificationAction::SkipBreak);
        } else if (action_index == 1) {
            Dispatch(NotificationAction::SnoozeBreak);
        } else {
            Dispatch(NotificationAction::Clicked);
        }
    }

    void toastActivated(std::wstring) const override {
        Dispatch(NotificationAction::Clicked);
    }

    void toastDismissed(WinToastDismissalReason) const override {
        Dispatch(NotificationAction::Dismissed);
    }

    void toastFailed() const override {
        spdlog::error("WinToast: toast failed");
    }

private:
    void Dispatch(NotificationAction action) const {
        if (!callback_) {
            return;
        }
        auto callback_copy = callback_;
        if (callback_copy && *callback_copy) {
            (*callback_copy)(action);
        }
    }

    std::shared_ptr<std::function<void(NotificationAction)>> callback_;
};

}  // namespace

NotificationManagerWin::NotificationManagerWin() = default;

NotificationManagerWin::~NotificationManagerWin() {
    StopWorker();
}

void NotificationManagerWin::EnsureWorker() {
    std::lock_guard lock(queue_mutex_);
    if (worker_started_) {
        return;
    }

    worker_running_.store(true);
    worker_thread_ = std::thread([this]() {
#ifdef _WIN32
        const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock queue_lock(queue_mutex_);
                queue_cv_.wait(queue_lock, [this]() {
                    return !tasks_.empty() || !worker_running_.load();
                });
                if (!worker_running_.load() && tasks_.empty()) {
                    break;
                }
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            if (task) {
                task();
            }
        }
#ifdef _WIN32
        if (SUCCEEDED(hr)) {
            CoUninitialize();
        }
#endif
    });
    worker_started_ = true;
}

void NotificationManagerWin::StopWorker() {
    {
        std::lock_guard lock(queue_mutex_);
        if (!worker_started_) {
            return;
        }
        worker_running_.store(false);
    }
    queue_cv_.notify_all();
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    worker_started_ = false;
}

void NotificationManagerWin::EnqueueTask(std::function<void()> task) {
    EnsureWorker();
    {
        std::lock_guard lock(queue_mutex_);
        tasks_.push(std::move(task));
    }
    queue_cv_.notify_one();
}

bool NotificationManagerWin::Initialize() {
    EnsureWorker();

    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();

    EnqueueTask([this, promise]() {
        std::lock_guard lock(mutex_);
        if (initialized_) {
            promise->set_value(true);
            return;
        }

        if (!WinToastLib::WinToast::isCompatible()) {
            spdlog::warn("WinToast: system not compatible");
            promise->set_value(false);
            return;
        }

        auto* toast = WinToastLib::WinToast::instance();
        toast->setAppName(L"BlinkBreak");
        const auto aumi = WinToastLib::WinToast::configureAUMI(
            L"BlinkBreak", L"BlinkBreak", L"BreakReminder", L"1.0");
        toast->setAppUserModelId(aumi);

        initialized_ = toast->initialize();
        if (!initialized_) {
            spdlog::error("WinToast: initialize failed");
        }
        promise->set_value(initialized_);
    });

    return future.get();
}

int64_t NotificationManagerWin::Show(const std::string& title, const std::string& message) {
    EnsureWorker();

    auto promise = std::make_shared<std::promise<int64_t>>();
    auto future = promise->get_future();

    EnqueueTask([this, title, message, promise]() {
        std::shared_ptr<std::function<void(NotificationAction)>> callback;
        {
            std::lock_guard lock(mutex_);
            if (!initialized_) {
                spdlog::warn("WinToast: Show called before Initialize");
                promise->set_value(-1);
                return;
            }
            callback = std::make_shared<std::function<void(NotificationAction)>>(on_action_);
        }

        WinToastLib::WinToastTemplate templ(WinToastLib::WinToastTemplate::Text02);
        templ.setTextField(Utf8ToWide(title), WinToastLib::WinToastTemplate::FirstLine);
        templ.setTextField(Utf8ToWide(message), WinToastLib::WinToastTemplate::SecondLine);
        templ.setDuration(WinToastLib::WinToastTemplate::Duration::Short);
        templ.addAction(L"Skip break");
        templ.addAction(L"Snooze break");

        auto* handler = new BlinkBreakToastHandler(callback);
        WinToastLib::WinToast::WinToastError error;

        const auto id = WinToastLib::WinToast::instance()->showToast(templ, handler, &error);
        if (id < 0) {
            spdlog::error("WinToast: showToast failed (error={})", static_cast<int>(error));
            promise->set_value(-1);
            return;
        }

        promise->set_value(static_cast<int64_t>(id));
    });

    return future.get();
}

void NotificationManagerWin::Hide(int64_t toast_id) {
    if (toast_id < 0) {
        return;
    }

    EnqueueTask([toast_id]() {
        WinToastLib::WinToast::instance()->hideToast(static_cast<int64_t>(toast_id));
    });
}

void NotificationManagerWin::SetOnAction(std::function<void(NotificationAction)> callback) {
    std::lock_guard lock(mutex_);
    on_action_ = std::move(callback);
}

bool NotificationManagerWin::IsSupported() const {
    return WinToastLib::WinToast::isCompatible();
}

std::unique_ptr<INotificationManager> CreateNotificationManager() {
    return std::make_unique<NotificationManagerWin>();
}

}  // namespace blinkbreak::platform
