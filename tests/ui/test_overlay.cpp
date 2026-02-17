/// @file test_overlay.cpp
/// @brief UI tests for BreakOverlay Slint component.

#include <gtest/gtest.h>

#include "overlay.h"
#include "ui_test_utils.hpp"

namespace blinkbreak::ui_test {
namespace {

/// @test Verifies overlay properties round-trip.
/// @details Sets message, time, break type, and action toggles; expects getters to match.
TEST(OverlayUiTest, PropertiesRoundTrip) {
    auto overlay = BreakOverlay::create();

    overlay->set_message("Hydrate");
    overlay->set_time_remaining("00:45");
    overlay->set_break_type("Long");
    overlay->set_can_skip(false);
    overlay->set_can_snooze(true);
    overlay->set_snooze_label("Snooze (7 min)");
    overlay->set_overlay_opacity(0.55f);

    EXPECT_EQ(ToStdString(overlay->get_message()), "Hydrate");
    EXPECT_EQ(ToStdString(overlay->get_time_remaining()), "00:45");
    EXPECT_EQ(ToStdString(overlay->get_break_type()), "Long");
    EXPECT_FALSE(overlay->get_can_skip());
    EXPECT_TRUE(overlay->get_can_snooze());
    EXPECT_EQ(ToStdString(overlay->get_snooze_label()), "Snooze (7 min)");
    EXPECT_FLOAT_EQ(overlay->get_overlay_opacity(), 0.55f);
}

/// @test Verifies skip and snooze callbacks invoke.
/// @details Ensures both callbacks fire once when invoked.
TEST(OverlayUiTest, SkipAndSnoozeCallbacksInvoke) {
    auto overlay = BreakOverlay::create();

    CallbackSpy skip_spy;
    CallbackSpy snooze_spy;

    overlay->on_skip_clicked([&skip_spy]() { skip_spy(); });
    overlay->on_snooze_clicked([&snooze_spy]() { snooze_spy(); });

    overlay->invoke_skip_clicked();
    overlay->invoke_snooze_clicked();

    EXPECT_EQ(skip_spy.count, 1);
    EXPECT_EQ(snooze_spy.count, 1);
}

}  // namespace
}  // namespace blinkbreak::ui_test