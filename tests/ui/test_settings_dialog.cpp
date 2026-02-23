/// @file test_settings_dialog.cpp
/// @brief UI tests for SettingsDialog Slint component.

#include <gtest/gtest.h>

#include "main_window.h"
#include "ui_test_utils.hpp"

namespace blinkbreak::ui_test {
namespace {

/// @test Verifies default values are present on construction.
/// @details Confirms the dialog exposes expected default intervals and durations,
/// and that the validation error state starts cleared.
TEST(SettingsDialogUiTest, DefaultValuesAreAvailable) {
    auto dialog = SettingsDialog::create();
    
    EXPECT_EQ(ToStdString(dialog->get_short_interval_minutes()), "10");
    EXPECT_EQ(ToStdString(dialog->get_short_duration_seconds()), "20");
    EXPECT_EQ(ToStdString(dialog->get_long_interval_minutes()), "60");
    EXPECT_EQ(ToStdString(dialog->get_long_duration_seconds()), "300");
    EXPECT_EQ(ToStdString(dialog->get_snooze_duration_minutes()), "5");
    EXPECT_TRUE(dialog->get_show_on_all_monitors());
    EXPECT_EQ(ToStdString(dialog->get_validation_error()), "");
    EXPECT_FALSE(dialog->get_has_validation_error());
}

/// @test Verifies settings properties round-trip correctly.
/// @details Sets all interval/duration properties and ensures getters return
/// the same values, validating the bindings are functional.
TEST(SettingsDialogUiTest, ValuesRoundTrip) {
    auto dialog = SettingsDialog::create();
    
    dialog->set_short_interval_minutes("15");
    dialog->set_short_duration_seconds("30");
    dialog->set_long_interval_minutes("90");
    dialog->set_long_duration_seconds("600");
    dialog->set_snooze_duration_minutes("7");
    dialog->set_show_on_all_monitors(false);

    EXPECT_EQ(ToStdString(dialog->get_short_interval_minutes()), "15");
    EXPECT_EQ(ToStdString(dialog->get_short_duration_seconds()), "30");
    EXPECT_EQ(ToStdString(dialog->get_long_interval_minutes()), "90");
    EXPECT_EQ(ToStdString(dialog->get_long_duration_seconds()), "600");
    EXPECT_EQ(ToStdString(dialog->get_snooze_duration_minutes()), "7");
    EXPECT_FALSE(dialog->get_show_on_all_monitors());
}

/// @test Verifies validation error toggles the error flag.
/// @details Ensures an empty error clears the flag and a non-empty message
/// sets both the string and the `has-validation-error` property.
TEST(SettingsDialogUiTest, ValidationErrorUpdatesFlag) {
    auto dialog = SettingsDialog::create();
    
    dialog->set_validation_error("");
    EXPECT_FALSE(dialog->get_has_validation_error());

    dialog->set_validation_error("Invalid input");
    EXPECT_TRUE(dialog->get_has_validation_error());
    EXPECT_EQ(ToStdString(dialog->get_validation_error()), "Invalid input");
}

/// @test Verifies save and cancel callbacks are invoked.
/// @details Confirms both callback hooks are wired and fire exactly once each
/// when invoked from the test harness.
TEST(SettingsDialogUiTest, SaveAndCancelCallbacksInvoke) {
    auto dialog = SettingsDialog::create();
    
    CallbackSpy save_spy;
    CallbackSpy cancel_spy;

    dialog->on_save_clicked([&save_spy]() { save_spy(); });
    dialog->on_cancel_clicked([&cancel_spy]() { cancel_spy(); });

    dialog->invoke_save_clicked();
    dialog->invoke_cancel_clicked();

    EXPECT_EQ(save_spy.count, 1);
    EXPECT_EQ(cancel_spy.count, 1);
}

}  // namespace
}  // namespace blinkbreak::ui_test