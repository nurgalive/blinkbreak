/// @file test_main_window.cpp
/// @brief UI tests for MainWindow Slint component.

#include <gtest/gtest.h>

#include "main_window.h"
#include "ui_test_utils.hpp"

namespace blinkbreak::ui_test
{
namespace
{

/// @test Verifies the computed Start/Pause label reflects the running state.
/// @details Ensures the `start-button-label` binding flips between "Start" and
/// "Pause" when `is-running` is toggled, without requiring the event loop.
TEST(MainWindowUiTest, StartButtonLabelReflectsRunningState)
{
  auto window = MainWindow::create();

  window->set_is_running(false);
  EXPECT_EQ(ToStdString(window->get_start_button_label()), "Start");

  window->set_is_running(true);
  EXPECT_EQ(ToStdString(window->get_start_button_label()), "Pause");
}

/// @test Verifies the start callback is invoked.
/// @details Confirms that `start-clicked` triggers the bound C++ handler and
/// increments the callback spy exactly once.
TEST(MainWindowUiTest, StartClickedInvokesCallback)
{
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
TEST(MainWindowUiTest, PauseClickedInvokesCallback)
{
  auto window = MainWindow::create();

  CallbackSpy pause_spy;
  window->on_pause_clicked([&pause_spy]() { pause_spy(); });

  window->set_is_running(true);
  window->invoke_pause_clicked();

  EXPECT_EQ(pause_spy.count, 1);
}

/// @test Verifies settings callback is invoked.
/// @details Ensures that `settings-clicked` fires and the callback spy records
/// a single invocation.
TEST(MainWindowUiTest, SettingsCallbackInvokes)
{
  auto window = MainWindow::create();

  CallbackSpy settings_spy;

  window->on_settings_clicked([&settings_spy]() { settings_spy(); });

  window->invoke_settings_clicked();

  EXPECT_EQ(settings_spy.count, 1);
}

/// @test Verifies property round-trips for main window state.
/// @details Sets multiple properties (time strings, status, progress, counts,
/// tracked time, and skipped counts) and verifies getters return identical values.
TEST(MainWindowUiTest, PropertiesRoundTrip)
{
  auto window = MainWindow::create();

  window->set_time_until_short("05:00");
  window->set_time_until_long("25:00");
  window->set_tracked_time("00:45");
  window->set_status_text("Running");
  window->set_short_progress(0.25f);
  window->set_long_progress(0.5f);
  window->set_short_break_count(2);
  window->set_long_break_count(1);
  window->set_short_skipped_count(1);
  window->set_long_skipped_count(3);
  window->set_short_idle_skipped_count(2);
  window->set_long_idle_skipped_count(4);

  EXPECT_EQ(ToStdString(window->get_time_until_short()), "05:00");
  EXPECT_EQ(ToStdString(window->get_time_until_long()), "25:00");
  EXPECT_EQ(ToStdString(window->get_tracked_time()), "00:45");
  EXPECT_EQ(ToStdString(window->get_status_text()), "Running");
  EXPECT_FLOAT_EQ(window->get_short_progress(), 0.25f);
  EXPECT_FLOAT_EQ(window->get_long_progress(), 0.5f);
  EXPECT_EQ(window->get_short_break_count(), 2);
  EXPECT_EQ(window->get_long_break_count(), 1);
  EXPECT_EQ(window->get_short_skipped_count(), 1);
  EXPECT_EQ(window->get_long_skipped_count(), 3);
  EXPECT_EQ(window->get_short_idle_skipped_count(), 2);
  EXPECT_EQ(window->get_long_idle_skipped_count(), 4);
}

/// @test Verifies theme properties round-trip on the main window.
TEST(MainWindowUiTest, ThemePropertiesRoundTrip)
{
  auto window = MainWindow::create();

  EXPECT_TRUE(window->get_theme_follow_system());
  EXPECT_FALSE(window->get_theme_dark_mode());
  EXPECT_FALSE(window->get_dark_theme_enabled());

  window->set_theme_follow_system(false);
  window->set_theme_dark_mode(true);

  EXPECT_FALSE(window->get_theme_follow_system());
  EXPECT_TRUE(window->get_theme_dark_mode());
  EXPECT_TRUE(window->get_dark_theme_enabled());

  window->set_theme_follow_system(true);
  EXPECT_FALSE(window->get_dark_theme_enabled());
}

}  // namespace
}  // namespace blinkbreak::ui_test
