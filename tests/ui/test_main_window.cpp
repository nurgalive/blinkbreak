/// @file test_main_window.cpp
/// @brief UI tests for MainWindow Slint component.

#include <gtest/gtest.h>

#include "main_window.h"
#include "ui_test_utils.hpp"

namespace blinkbreak::ui_test {
namespace {

/// @test Verifies the computed Start/Pause label reflects the running state.
/// @details Ensures the `start-button-label` binding flips between "Start" and
/// "Pause" when `is-running` is toggled, without requiring the event loop.
TEST(MainWindowUiTest, StartButtonLabelReflectsRunningState) {
    auto window = MainWindow::create();
    
    window->set_is_running(false);
    EXPECT_EQ(ToStdString(window->get_start_button_label()), "Start");

    window->set_is_running(true);
    EXPECT_EQ(ToStdString(window->get_start_button_label()), "Pause");
}

/// @test Verifies the start callback is invoked.
/// @details Confirms that `start-clicked` triggers the bound C++ handler and
/// increments the callback spy exactly once.
TEST(MainWindowUiTest, StartClickedInvokesCallback) {
    auto window = MainWindow::create();
    
    CallbackSpy start_spy;
    window->on_start_clicked([&start_spy]() { start_spy(); });

    window->set_is_running(false);
    window->invoke_start_clicked();

    EXPECT_EQ(start_spy.count, 1);
}

/// @test Verifies the pause callback is invoked.
/// @details Confirms that `pause-clicked` triggers the bound C++ handler and
/// increments the callback spy exactly once.
TEST(MainWindowUiTest, PauseClickedInvokesCallback) {
    auto window = MainWindow::create();
    
    CallbackSpy pause_spy;
    window->on_pause_clicked([&pause_spy]() { pause_spy(); });

    window->set_is_running(true);
    window->invoke_pause_clicked();

    EXPECT_EQ(pause_spy.count, 1);
}

/// @test Verifies skip and settings callbacks are invoked.
/// @details Ensures that both `skip-clicked` and `settings-clicked` callbacks
/// fire and each callback spy records a single invocation.
TEST(MainWindowUiTest, SkipAndSettingsCallbacksInvoke) {
    auto window = MainWindow::create();
    
    CallbackSpy skip_spy;
    CallbackSpy settings_spy;

    window->on_skip_clicked([&skip_spy]() { skip_spy(); });
    window->on_settings_clicked([&settings_spy]() { settings_spy(); });

    window->invoke_skip_clicked();
    window->invoke_settings_clicked();

    EXPECT_EQ(skip_spy.count, 1);
    EXPECT_EQ(settings_spy.count, 1);
}

/// @test Verifies property round-trips for main window state.
/// @details Sets multiple properties (time strings, status, progress, counts,
/// and skip enablement) and verifies getters return identical values.
TEST(MainWindowUiTest, PropertiesRoundTrip) {
    auto window = MainWindow::create();
    
    window->set_time_until_short("05:00");
    window->set_time_until_long("25:00");
    window->set_status_text("Running");
    window->set_short_progress(0.25f);
    window->set_long_progress(0.5f);
    window->set_short_break_count(2);
    window->set_long_break_count(1);
    window->set_can_skip(true);

    EXPECT_EQ(ToStdString(window->get_time_until_short()), "05:00");
    EXPECT_EQ(ToStdString(window->get_time_until_long()), "25:00");
    EXPECT_EQ(ToStdString(window->get_status_text()), "Running");
    EXPECT_FLOAT_EQ(window->get_short_progress(), 0.25f);
    EXPECT_FLOAT_EQ(window->get_long_progress(), 0.5f);
    EXPECT_EQ(window->get_short_break_count(), 2);
    EXPECT_EQ(window->get_long_break_count(), 1);
    EXPECT_TRUE(window->get_can_skip());
}

}  // namespace
}  // namespace blinkbreak::ui_test