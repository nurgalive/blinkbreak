/// @file test_timer_display.cpp
/// @brief UI tests for TimerDisplay Slint component.

#include <gtest/gtest.h>

#include "timer_display.h"
#include "ui_test_utils.hpp"

namespace blinkbreak::ui_test {
namespace {

/// @test Verifies default TimerDisplay property values.
/// @details Confirms the component initializes with expected time strings,
/// progress, and break counts without relying on the event loop.
TEST(TimerDisplayUiTest, DefaultValuesAreAvailable) {
    auto display = TimerDisplayWindow::create();
    
    EXPECT_EQ(ToStdString(display->get_time()), "00:00");
    EXPECT_EQ(ToStdString(display->get_secondary_time()), "00:00");
    EXPECT_FLOAT_EQ(display->get_progress(), 0.0f);
    EXPECT_EQ(display->get_short_break_count(), 0);
    EXPECT_EQ(display->get_long_break_count(), 0);
}

/// @test Verifies property round-trips for TimerDisplay.
/// @details Sets time strings, progress, and break counts, then asserts the
/// getters return the same values.
TEST(TimerDisplayUiTest, PropertiesRoundTrip) {
    auto display = TimerDisplayWindow::create();
    
    display->set_time("05:30");
    display->set_secondary_time("25:00");
    display->set_progress(0.75f);
    display->set_short_break_count(3);
    display->set_long_break_count(1);

    EXPECT_EQ(ToStdString(display->get_time()), "05:30");
    EXPECT_EQ(ToStdString(display->get_secondary_time()), "25:00");
    EXPECT_FLOAT_EQ(display->get_progress(), 0.75f);
    EXPECT_EQ(display->get_short_break_count(), 3);
    EXPECT_EQ(display->get_long_break_count(), 1);
}

/// @test Verifies progress accepts boundary values.
/// @details Ensures progress can be set to 0.0 and 1.0 without clamping.
TEST(TimerDisplayUiTest, ProgressAcceptsBoundaryValues) {
    auto display = TimerDisplayWindow::create();
    
    display->set_progress(0.0f);
    EXPECT_FLOAT_EQ(display->get_progress(), 0.0f);

    display->set_progress(1.0f);
    EXPECT_FLOAT_EQ(display->get_progress(), 1.0f);
}

/// @test Verifies break counts accept edge values.
/// @details Confirms both zero and larger counts are stored correctly.
TEST(TimerDisplayUiTest, BreakCountsAcceptEdgeValues) {
    auto display = TimerDisplayWindow::create();
    
    display->set_short_break_count(0);
    display->set_long_break_count(0);
    EXPECT_EQ(display->get_short_break_count(), 0);
    EXPECT_EQ(display->get_long_break_count(), 0);

    display->set_short_break_count(42);
    display->set_long_break_count(7);
    EXPECT_EQ(display->get_short_break_count(), 42);
    EXPECT_EQ(display->get_long_break_count(), 7);
}

}  // namespace
}  // namespace blinkbreak::ui_test