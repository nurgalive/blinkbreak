/// @file test_notification.cpp
/// @brief Unit tests for notification manager interfaces.

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "platform/platform_interface.hpp"

namespace blinkbreak {
namespace {

class MockNotificationManager : public platform::INotificationManager {
public:
    bool Initialize() override {
        initialized_ = true;
        return true;
    }

    int64_t Show(const std::string& title, const std::string& message) override {
        (void)title;
        (void)message;
        if (!initialized_) {
            return -1;
        }
        return next_id_++;
    }

    void Hide(int64_t toast_id) override {
        (void)toast_id;
    }

    void SetOnAction(std::function<void(platform::NotificationAction)> callback) override {
        on_action_ = std::move(callback);
    }

    [[nodiscard]] bool IsSupported() const override {
        return supported_;
    }

    void TriggerAction(platform::NotificationAction action) {
        if (on_action_) {
            on_action_(action);
        }
    }

    void SetSupported(bool value) { supported_ = value; }
    [[nodiscard]] bool IsInitialized() const { return initialized_; }

private:
    bool supported_ = true;
    bool initialized_ = false;
    int64_t next_id_ = 0;
    std::function<void(platform::NotificationAction)> on_action_;
};

// ============================================================================
// Mock Notification Manager Tests
// ============================================================================

TEST(NotificationManagerTest, MockNotificationManagerInitialState) {
    MockNotificationManager manager;
    EXPECT_TRUE(manager.IsSupported());
    EXPECT_FALSE(manager.IsInitialized());
    EXPECT_TRUE(manager.Initialize());
    EXPECT_TRUE(manager.IsInitialized());
}

TEST(NotificationManagerTest, MockNotificationManagerShowReturnsId) {
    MockNotificationManager manager;
    manager.Initialize();

    const int64_t id = manager.Show("Title", "Body");
    EXPECT_GE(id, 0);
}

TEST(NotificationManagerTest, MockNotificationManagerHideDoesNotThrow) {
    MockNotificationManager manager;
    manager.Initialize();

    EXPECT_NO_THROW(manager.Hide(1));
    EXPECT_NO_THROW(manager.Hide(-1));
}

TEST(NotificationManagerTest, MockNotificationManagerActionCallback) {
    MockNotificationManager manager;
    std::vector<platform::NotificationAction> actions;

    manager.SetOnAction([&actions](platform::NotificationAction action) {
        actions.push_back(action);
    });

    manager.TriggerAction(platform::NotificationAction::Clicked);
    manager.TriggerAction(platform::NotificationAction::Dismissed);
    manager.TriggerAction(platform::NotificationAction::SkipBreak);
    manager.TriggerAction(platform::NotificationAction::SnoozeBreak);

    ASSERT_EQ(actions.size(), 4u);
    EXPECT_EQ(actions[0], platform::NotificationAction::Clicked);
    EXPECT_EQ(actions[1], platform::NotificationAction::Dismissed);
    EXPECT_EQ(actions[2], platform::NotificationAction::SkipBreak);
    EXPECT_EQ(actions[3], platform::NotificationAction::SnoozeBreak);
}

TEST(NotificationManagerTest, MockNotificationManagerShowWithEmptyStrings) {
    MockNotificationManager manager;
    manager.Initialize();

    const int64_t id = manager.Show("", "");
    EXPECT_GE(id, 0);
}

TEST(NotificationManagerTest, MockNotificationManagerShowAfterInitialize) {
    MockNotificationManager manager;
    EXPECT_EQ(manager.Show("Title", "Body"), -1);

    manager.Initialize();
    EXPECT_GE(manager.Show("Title", "Body"), 0);
}

TEST(NotificationManagerTest, MockNotificationManagerMultipleShowCalls) {
    MockNotificationManager manager;
    manager.Initialize();

    const int64_t id1 = manager.Show("Title1", "Body1");
    const int64_t id2 = manager.Show("Title2", "Body2");
    EXPECT_NE(id1, id2);
}

TEST(NotificationManagerTest, MockNotificationManagerSetActionCallbackOverwrite) {
    MockNotificationManager manager;
    int first_count = 0;
    int second_count = 0;

    manager.SetOnAction([&first_count](platform::NotificationAction) { ++first_count; });
    manager.SetOnAction([&second_count](platform::NotificationAction) { ++second_count; });

    manager.TriggerAction(platform::NotificationAction::Clicked);
    EXPECT_EQ(first_count, 0);
    EXPECT_EQ(second_count, 1);
}

// ============================================================================
// Platform Notification Manager Tests
// ============================================================================

TEST(NotificationManagerTest, NotificationManagerWinCreate) {
    auto manager = platform::CreateNotificationManager();
    EXPECT_NE(manager, nullptr);
}

TEST(NotificationManagerTest, NotificationManagerWinIsCompatible) {
#ifndef _WIN32
    GTEST_SKIP() << "WinToast only supported on Windows.";
#else
    auto manager = platform::CreateNotificationManager();
    ASSERT_NE(manager, nullptr);
    EXPECT_TRUE(manager->IsSupported());
#endif
}

TEST(NotificationManagerTest, NotificationManagerWinInitializeSucceeds) {
#ifndef _WIN32
    GTEST_SKIP() << "WinToast only supported on Windows.";
#else
    auto manager = platform::CreateNotificationManager();
    ASSERT_NE(manager, nullptr);
    if (!manager->IsSupported()) {
        GTEST_SKIP() << "System does not support toast notifications.";
    }
    EXPECT_TRUE(manager->Initialize());
#endif
}

TEST(NotificationManagerTest, NotificationManagerWinShowAndHide) {
#ifndef _WIN32
    GTEST_SKIP() << "WinToast only supported on Windows.";
#else
    auto manager = platform::CreateNotificationManager();
    ASSERT_NE(manager, nullptr);
    if (!manager->IsSupported()) {
        GTEST_SKIP() << "System does not support toast notifications.";
    }
    ASSERT_TRUE(manager->Initialize());

    const int64_t id = manager->Show("BlinkBreak", "Test notification");
    EXPECT_GE(id, 0);
    manager->Hide(id);
#endif
}

}  // namespace
}  // namespace blinkbreak
