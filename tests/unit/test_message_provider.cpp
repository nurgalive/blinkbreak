/// @file test_message_provider.cpp
/// @brief Unit tests for the MessageProvider class.

#include "core/message_provider.hpp"

#include <gtest/gtest.h>

#include <set>

namespace blinkbreak {
namespace {

/// @brief Test fixture for MessageProvider tests.
class MessageProviderTest : public ::testing::Test {
protected:
    std::vector<std::string> test_messages_ = {
        "Message 1",
        "Message 2",
        "Message 3"
    };
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

/// @test Constructor creates provider with messages.
TEST_F(MessageProviderTest, ConstructorCreatesWithMessages) {
    MessageProvider provider(test_messages_);
    EXPECT_EQ(provider.GetMessageCount(), 3);
}

/// @test Empty messages get default message.
TEST_F(MessageProviderTest, EmptyMessagesGetDefault) {
    MessageProvider provider({});
    EXPECT_EQ(provider.GetMessageCount(), 1);
    EXPECT_EQ(provider.GetNext(), "Take a break!");
}

/// @test GetCurrent returns first message initially.
TEST_F(MessageProviderTest, GetCurrentReturnsFirstInitially) {
    MessageProvider provider(test_messages_, false);
    EXPECT_EQ(provider.GetCurrent(), "Message 1");
}

/// @test GetNext returns messages in order when not rotating.
TEST_F(MessageProviderTest, GetNextReturnsFirstWhenNotRotating) {
    MessageProvider provider(test_messages_, false);
    EXPECT_EQ(provider.GetNext(), "Message 1");
    EXPECT_EQ(provider.GetNext(), "Message 1");
    EXPECT_EQ(provider.GetNext(), "Message 1");
}

// ============================================================================
// Rotation Tests
// ============================================================================

/// @test GetNext rotates through messages.
TEST_F(MessageProviderTest, GetNextRotatesThroughMessages) {
    MessageProvider provider(test_messages_, true, false);
    EXPECT_EQ(provider.GetNext(), "Message 1");
    EXPECT_EQ(provider.GetNext(), "Message 2");
    EXPECT_EQ(provider.GetNext(), "Message 3");
    EXPECT_EQ(provider.GetNext(), "Message 1");  // Wraps around
}

/// @test Reset returns to first message.
TEST_F(MessageProviderTest, ResetReturnsToFirst) {
    MessageProvider provider(test_messages_, true, false);
    (void)provider.GetNext();
    (void)provider.GetNext();
    provider.Reset();
    EXPECT_EQ(provider.GetNext(), "Message 1");
}

// ============================================================================
// Random Mode Tests
// ============================================================================

/// @test Random mode returns all messages.
TEST_F(MessageProviderTest, RandomModeReturnsAllMessages) {
    MessageProvider provider(test_messages_, true, true);
    std::set<std::string> seen;

    for (int i = 0; i < 3; ++i) {
        seen.insert(provider.GetNext());
    }

    EXPECT_EQ(seen.size(), 3);
}

// ============================================================================
// Configuration Tests
// ============================================================================

/// @test SetMessages updates messages.
TEST_F(MessageProviderTest, SetMessagesUpdatesMessages) {
    MessageProvider provider(test_messages_);
    provider.SetMessages({"New message"});

    EXPECT_EQ(provider.GetMessageCount(), 1);
    EXPECT_EQ(provider.GetNext(), "New message");
}

/// @test SetRotate changes rotation behavior.
TEST_F(MessageProviderTest, SetRotateChangesRotation) {
    MessageProvider provider(test_messages_, false);
    (void)provider.GetNext();
    provider.SetRotate(true);

    EXPECT_EQ(provider.GetNext(), "Message 1");
    EXPECT_EQ(provider.GetNext(), "Message 2");
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

/// @test Move constructor works correctly.
TEST_F(MessageProviderTest, MoveConstructorWorks) {
    MessageProvider provider(test_messages_, true, false);
    (void)provider.GetNext();  // Advance to Message 2

    MessageProvider moved(std::move(provider));
    EXPECT_EQ(moved.GetNext(), "Message 2");
}

}  // namespace
}  // namespace blinkbreak