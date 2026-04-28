/// @file mock_platform.hpp
/// @brief Mock platform components for integration testing.

#ifndef BLINKBREAK_TESTS_MOCK_PLATFORM_HPP
#define BLINKBREAK_TESTS_MOCK_PLATFORM_HPP

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "platform/platform_interface.hpp"

namespace blinkbreak::testing {

using namespace platform;

/// @brief Mock idle detector for integration testing.
///
/// Provides controllable idle simulation without real system polling.
class MockIdleDetector : public IIdleDetector {
public:
    void Start() override {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = true;
    }

    void Stop() override {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }

    [[nodiscard]] bool IsRunning() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return running_;
    }

    [[nodiscard]] std::chrono::milliseconds GetIdleTime() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return simulated_idle_time_;
    }

    [[nodiscard]] bool IsIdle() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return simulated_idle_time_ >= threshold_;
    }

    void SetIdleThreshold(std::chrono::seconds threshold) override {
        std::lock_guard<std::mutex> lock(mutex_);
        threshold_ = std::chrono::duration_cast<std::chrono::milliseconds>(threshold);
    }

    [[nodiscard]] std::chrono::seconds GetIdleThreshold() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::chrono::duration_cast<std::chrono::seconds>(threshold_);
    }

    void SetOnIdle(std::function<void()> callback) override {
        std::lock_guard<std::mutex> lock(mutex_);
        on_idle_ = std::move(callback);
    }

    void SetOnActive(std::function<void()> callback) override {
        std::lock_guard<std::mutex> lock(mutex_);
        on_active_ = std::move(callback);
    }

    // --- Test control methods ---

    /// @brief Simulates user becoming idle for a duration.
    void SimulateIdle(std::chrono::milliseconds duration) {
        std::function<void()> callback;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            bool was_idle = simulated_idle_time_ >= threshold_;
            simulated_idle_time_ = duration;
            bool is_now_idle = simulated_idle_time_ >= threshold_;
            if (!was_idle && is_now_idle && on_idle_) {
                callback = on_idle_;
            }
        }
        if (callback) {
            callback();
        }
    }

    /// @brief Simulates user activity (ends idle state).
    void SimulateActivity() {
        std::function<void()> callback;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            bool was_idle = simulated_idle_time_ >= threshold_;
            simulated_idle_time_ = std::chrono::milliseconds(0);
            if (was_idle && on_active_) {
                callback = on_active_;
            }
        }
        if (callback) {
            callback();
        }
    }

    /// @brief Manually triggers the idle callback.
    void TriggerIdleCallback() {
        std::function<void()> callback;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callback = on_idle_;
        }
        if (callback) {
            callback();
        }
    }

    /// @brief Manually triggers the active callback.
    void TriggerActiveCallback() {
        std::function<void()> callback;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callback = on_active_;
        }
        if (callback) {
            callback();
        }
    }

    /// @brief Advances idle time without triggering callbacks.
    void AdvanceIdleTime(std::chrono::milliseconds delta) {
        std::lock_guard<std::mutex> lock(mutex_);
        simulated_idle_time_ += delta;
    }

private:
    mutable std::mutex mutex_;
    bool running_ = false;
    std::chrono::milliseconds simulated_idle_time_{0};
    std::chrono::milliseconds threshold_{std::chrono::minutes(5)};
    std::function<void()> on_idle_;
    std::function<void()> on_active_;
};

/// @brief Mock DND detector for integration testing.
///
/// Provides controllable DND state simulation.
class MockDndDetector : public IDndDetector {
public:
    void Start() override {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = true;
    }

    void Stop() override {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }

    [[nodiscard]] bool IsRunning() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return running_;
    }

    [[nodiscard]] DndState GetState() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    [[nodiscard]] DndState RefreshState() override {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    [[nodiscard]] bool IsDndActive() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_ != DndState::AcceptsNotifications;
    }

    [[nodiscard]] bool IsFullScreenDetected() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return fullscreen_;
    }

    void SetOnDndChange(std::function<void(bool)> callback) override {
        std::lock_guard<std::mutex> lock(mutex_);
        on_change_ = std::move(callback);
    }

    void SetPollingInterval(std::chrono::milliseconds interval) override {
        std::lock_guard<std::mutex> lock(mutex_);
        interval_ = interval;
    }

    [[nodiscard]] std::chrono::milliseconds GetPollingInterval() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return interval_;
    }

    // --- Test control methods ---

    /// @brief Sets the DND state.
    void SetState(DndState state) {
        std::function<void(bool)> callback;
        bool is_dnd_active = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            bool was_active = state_ != DndState::AcceptsNotifications;
            state_ = state;
            is_dnd_active = state_ != DndState::AcceptsNotifications;
            if (was_active != is_dnd_active && on_change_) {
                callback = on_change_;
            }
        }
        if (callback) {
            callback(is_dnd_active);
        }
    }

    /// @brief Sets the fullscreen detection state.
    void SetFullScreen(bool fullscreen) {
        std::lock_guard<std::mutex> lock(mutex_);
        fullscreen_ = fullscreen;
    }

    /// @brief Convenience method to enable DND.
    void EnableDnd() { SetState(DndState::PresentationMode); }

    /// @brief Convenience method to disable DND.
    void DisableDnd() { SetState(DndState::AcceptsNotifications); }

private:
    mutable std::mutex mutex_;
    bool running_ = false;
    DndState state_ = DndState::AcceptsNotifications;
    bool fullscreen_ = false;
    std::chrono::milliseconds interval_{std::chrono::seconds(1)};
    std::function<void(bool)> on_change_;
};

/// @brief Mock notification manager for integration testing.
///
/// Records notification calls for verification.
class MockNotificationManager : public INotificationManager {
public:
    [[nodiscard]] bool IsSupported() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return supported_;
    }

    bool Initialize() override {
        std::lock_guard<std::mutex> lock(mutex_);
        initialized_ = true;
        return true;
    }

    int64_t Show(const std::string& title, const std::string& body) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_) {
            return -1;
        }
        ++show_count_;
        last_title_ = title;
        last_body_ = body;
        notifications_.push_back({next_id_, title, body, true});
        return next_id_++;
    }

    void Hide(int64_t id) override {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& notif : notifications_) {
            if (notif.id == id) {
                notif.visible = false;
                break;
            }
        }
    }

    void SetOnAction(std::function<void(NotificationAction)> callback) override {
        std::lock_guard<std::mutex> lock(mutex_);
        on_action_ = std::move(callback);
    }

    // --- Test control methods ---

    /// @brief Sets whether notifications are supported.
    void SetSupported(bool supported) {
        std::lock_guard<std::mutex> lock(mutex_);
        supported_ = supported;
    }

    /// @brief Triggers an action callback.
    void TriggerAction(NotificationAction action) {
        std::function<void(NotificationAction)> callback;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callback = on_action_;
        }
        if (callback) {
            callback(action);
        }
    }

    /// @brief Gets the total number of Show() calls.
    [[nodiscard]] int GetShowCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return show_count_;
    }

    /// @brief Gets the last notification title.
    [[nodiscard]] std::string GetLastTitle() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_title_;
    }

    /// @brief Gets the last notification body.
    [[nodiscard]] std::string GetLastBody() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_body_;
    }

    /// @brief Checks if a notification is currently visible.
    [[nodiscard]] bool IsVisible(int64_t id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& notif : notifications_) {
            if (notif.id == id) {
                return notif.visible;
            }
        }
        return false;
    }

    /// @brief Resets all state.
    void Reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        show_count_ = 0;
        last_title_.clear();
        last_body_.clear();
        notifications_.clear();
        next_id_ = 1;
    }

private:
    struct NotificationRecord {
        int64_t id;
        std::string title;
        std::string body;
        bool visible;
    };

    mutable std::mutex mutex_;
    bool supported_ = true;
    bool initialized_ = false;
    int64_t next_id_ = 1;
    int show_count_ = 0;
    std::string last_title_;
    std::string last_body_;
    std::vector<NotificationRecord> notifications_;
    std::function<void(NotificationAction)> on_action_;
};

/// @brief Mock monitor manager for integration testing.
///
/// Provides configurable monitor layouts.
class MockMonitorManager : public IMonitorManager {
public:
    MockMonitorManager() {
        // Default single primary monitor
        MonitorInfo m;
        m.id = 0;
        m.name = "Mock Monitor 1";
        m.x = 0;
        m.y = 0;
        m.width = 1920;
        m.height = 1080;
        m.is_primary = true;
        m.orientation = MonitorOrientation::kLandscape;
        m.dpi = 96;
        monitors_.push_back(m);
    }

    void RefreshMonitors() override {
        // No-op for mock
    }

    [[nodiscard]] std::vector<MonitorInfo> GetMonitors() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return monitors_;
    }

    [[nodiscard]] MonitorInfo GetPrimaryMonitor() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& m : monitors_) {
            if (m.is_primary) {
                return m;
            }
        }
        return monitors_.empty() ? MonitorInfo{} : monitors_[0];
    }

    [[nodiscard]] int GetMonitorCount() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return static_cast<int>(monitors_.size());
    }

    void SetOnMonitorChange(std::function<void()> callback) override {
        std::lock_guard<std::mutex> lock(mutex_);
        on_change_ = std::move(callback);
    }

    // --- Test control methods ---

    /// @brief Sets the monitor configuration.
    void SetMonitors(std::vector<MonitorInfo> monitors) {
        std::function<void()> callback;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            monitors_ = std::move(monitors);
            callback = on_change_;
        }
        if (callback) {
            callback();
        }
    }

    /// @brief Triggers the monitor change callback.
    void TriggerMonitorChange() {
        std::function<void()> callback;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callback = on_change_;
        }
        if (callback) {
            callback();
        }
    }

    /// @brief Configures a dual-monitor setup.
    void SetDualMonitors() {
        std::vector<MonitorInfo> monitors;

        MonitorInfo m1;
        m1.id = 0;
        m1.name = "Monitor 1";
        m1.x = 0;
        m1.y = 0;
        m1.width = 1920;
        m1.height = 1080;
        m1.is_primary = true;
        m1.orientation = MonitorOrientation::kLandscape;
        m1.dpi = 96;
        monitors.push_back(m1);

        MonitorInfo m2;
        m2.id = 1;
        m2.name = "Monitor 2";
        m2.x = 1920;
        m2.y = 0;
        m2.width = 1920;
        m2.height = 1080;
        m2.is_primary = false;
        m2.orientation = MonitorOrientation::kLandscape;
        m2.dpi = 96;
        monitors.push_back(m2);

        SetMonitors(monitors);
    }

private:
    mutable std::mutex mutex_;
    std::vector<MonitorInfo> monitors_;
    std::function<void()> on_change_;
};

/// @brief Mock tray icon for integration testing.
class MockTrayIcon : public ITrayIcon {
public:
    bool Show() override {
        visible_ = true;
        return true;
    }

    void Hide() override { visible_ = false; }

    void SetTooltip(const std::string& tooltip) override { tooltip_ = tooltip; }

    void SetMenu(const std::vector<MenuItem>& items) override { menu_items_ = items; }

    void SetOnClick(std::function<void()> callback) override { on_click_ = std::move(callback); }

    void SetOnDoubleClick(std::function<void()> callback) override {
        on_double_click_ = std::move(callback);
    }

    void SetIcon(int icon_id) override { icon_id_ = icon_id; }

    // --- Test control methods ---

    [[nodiscard]] bool IsVisible() const { return visible_; }
    [[nodiscard]] std::string GetTooltip() const { return tooltip_; }
    [[nodiscard]] const std::vector<MenuItem>& GetMenuItems() const { return menu_items_; }
    [[nodiscard]] int GetIconId() const { return icon_id_; }

    void SimulateClick() {
        if (on_click_) {
            on_click_();
        }
    }

    void SimulateDoubleClick() {
        if (on_double_click_) {
            on_double_click_();
        }
    }

    void SimulateMenuItemClick(size_t index) {
        if (index < menu_items_.size() && menu_items_[index].callback) {
            menu_items_[index].callback();
        }
    }

private:
    bool visible_ = false;
    std::string tooltip_;
    std::vector<MenuItem> menu_items_;
    int icon_id_ = 0;
    std::function<void()> on_click_;
    std::function<void()> on_double_click_;
};

}  // namespace blinkbreak::testing

#endif  // BLINKBREAK_TESTS_MOCK_PLATFORM_HPP
