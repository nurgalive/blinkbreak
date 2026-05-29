/// @file test_settings_dialog.cpp
/// @brief UI tests for embedded settings properties on MainWindow.

#include <gtest/gtest.h>

#include "main_window.h"
#include "ui_test_utils.hpp"

namespace blinkbreak::ui_test
{
namespace
{

TEST(SettingsViewUiTest, DefaultValuesAreAvailable)
{
  auto window = MainWindow::create();

  EXPECT_EQ(ToStdString(window->get_short_interval_minutes()), "10");
  EXPECT_EQ(ToStdString(window->get_short_duration_seconds()), "20");
  EXPECT_EQ(ToStdString(window->get_long_interval_minutes()), "60");
  EXPECT_EQ(ToStdString(window->get_long_duration_seconds()), "300");
  EXPECT_EQ(ToStdString(window->get_snooze_duration_minutes()), "5");
  EXPECT_EQ(ToStdString(window->get_validation_error()), "");
  EXPECT_FALSE(window->get_has_validation_error());
}

TEST(SettingsViewUiTest, ValuesRoundTrip)
{
  auto window = MainWindow::create();

  window->set_short_interval_minutes("15");
  window->set_short_duration_seconds("30");
  window->set_long_interval_minutes("90");
  window->set_long_duration_seconds("600");
  window->set_snooze_duration_minutes("7");

  EXPECT_EQ(ToStdString(window->get_short_interval_minutes()), "15");
  EXPECT_EQ(ToStdString(window->get_short_duration_seconds()), "30");
  EXPECT_EQ(ToStdString(window->get_long_interval_minutes()), "90");
  EXPECT_EQ(ToStdString(window->get_long_duration_seconds()), "600");
  EXPECT_EQ(ToStdString(window->get_snooze_duration_minutes()), "7");
}

TEST(SettingsViewUiTest, ValidationErrorUpdatesFlag)
{
  auto window = MainWindow::create();

  window->set_validation_error("");
  EXPECT_FALSE(window->get_has_validation_error());

  window->set_validation_error("Invalid input");
  EXPECT_TRUE(window->get_has_validation_error());
  EXPECT_EQ(ToStdString(window->get_validation_error()), "Invalid input");
}

TEST(SettingsViewUiTest, OverlayAllMonitorsRoundTrip)
{
  auto window = MainWindow::create();

  EXPECT_TRUE(window->get_overlay_all_monitors());
  window->set_overlay_all_monitors(false);
  EXPECT_FALSE(window->get_overlay_all_monitors());
  window->set_overlay_all_monitors(true);
  EXPECT_TRUE(window->get_overlay_all_monitors());
}

TEST(SettingsViewUiTest, IdleDetectionRoundTrip)
{
  auto window = MainWindow::create();

  EXPECT_TRUE(window->get_idle_enabled());
  EXPECT_EQ(ToStdString(window->get_idle_threshold_minutes()), "3");
  EXPECT_TRUE(window->get_idle_pause_on_idle());

  window->set_idle_enabled(false);
  window->set_idle_threshold_minutes("5");
  window->set_idle_pause_on_idle(false);
  window->set_idle_reset_short_on_idle(false);
  window->set_idle_reset_short_threshold_minutes("30");
  window->set_idle_reset_long_on_idle(false);
  window->set_idle_reset_long_threshold_minutes("45");

  EXPECT_FALSE(window->get_idle_enabled());
  EXPECT_EQ(ToStdString(window->get_idle_threshold_minutes()), "5");
  EXPECT_FALSE(window->get_idle_pause_on_idle());
  EXPECT_FALSE(window->get_idle_reset_short_on_idle());
  EXPECT_EQ(ToStdString(window->get_idle_reset_short_threshold_minutes()), "30");
  EXPECT_FALSE(window->get_idle_reset_long_on_idle());
  EXPECT_EQ(ToStdString(window->get_idle_reset_long_threshold_minutes()), "45");
}

TEST(SettingsViewUiTest, ThemeSettingsRoundTrip)
{
  auto window = MainWindow::create();

  EXPECT_TRUE(window->get_settings_theme_follow_system());
  EXPECT_FALSE(window->get_settings_theme_dark_mode());

  window->set_settings_theme_follow_system(false);
  window->set_settings_theme_dark_mode(true);

  EXPECT_FALSE(window->get_settings_theme_follow_system());
  EXPECT_TRUE(window->get_settings_theme_dark_mode());
}

TEST(SettingsViewUiTest, NotificationSettingsRoundTrip)
{
  auto window = MainWindow::create();

  EXPECT_TRUE(window->get_notification_enabled());
  EXPECT_EQ(ToStdString(window->get_notification_warning_seconds()), "30");
  EXPECT_TRUE(window->get_notification_respect_dnd());
  EXPECT_FALSE(window->get_notification_respect_fullscreen());

  window->set_notification_enabled(false);
  window->set_notification_warning_seconds("60");
  window->set_notification_respect_dnd(false);
  window->set_notification_respect_fullscreen(true);

  EXPECT_FALSE(window->get_notification_enabled());
  EXPECT_EQ(ToStdString(window->get_notification_warning_seconds()), "60");
  EXPECT_FALSE(window->get_notification_respect_dnd());
  EXPECT_TRUE(window->get_notification_respect_fullscreen());
}

TEST(SettingsViewUiTest, SettingsChangedCallbackInvokes)
{
  auto window = MainWindow::create();

  CallbackSpy settings_spy;
  window->on_settings_changed([&settings_spy]() { settings_spy(); });

  window->invoke_settings_changed();

  EXPECT_EQ(settings_spy.count, 1);
}

}  // namespace
}  // namespace blinkbreak::ui_test
