#ifndef BLINKBREAK_TESTS_UI_UI_TEST_UTILS_HPP
#define BLINKBREAK_TESTS_UI_UI_TEST_UTILS_HPP

#include <slint.h>

#include <string>

namespace blinkbreak::ui_test {

/// @brief Converts a Slint SharedString into a standard string.
/// @param value Slint string to convert.
/// @return A std::string containing the same character data.
inline std::string ToStdString(const slint::SharedString& value) {
    return std::string(value.data(), value.size());
}

/// @brief Records how many times a zero-argument callback is invoked.
/// @details Use this spy in UI tests to verify that Slint callbacks fire.
/// Example:
/// @code
/// CallbackSpy spy;
/// component->on_clicked([&] { spy(); });
/// component->invoke_clicked();
/// EXPECT_EQ(spy.count, 1);
/// @endcode
struct CallbackSpy {
    /// @brief Number of times the callback has been invoked.
    int count = 0;

    /// @brief Increments the invocation count.
    void operator()() { ++count; }
};

/// @brief Records invocations and the last boolean value passed.
/// @details Useful for validating boolean-valued callbacks in Slint.
/// Example:
/// @code
/// BoolCallbackSpy spy;
/// component->on_toggle([&](bool v) { spy(v); });
/// component->invoke_toggle(true);
/// EXPECT_EQ(spy.count, 1);
/// EXPECT_TRUE(spy.last_value);
/// @endcode
struct BoolCallbackSpy {
    /// @brief Number of times the callback has been invoked.
    int count = 0;
    /// @brief Last boolean value observed from the callback.
    bool last_value = false;

    /// @brief Records the invocation and latest value.
    void operator()(bool value) {
        ++count;
        last_value = value;
    }
};

}  // namespace blinkbreak::ui_test

#endif  // BLINKBREAK_TESTS_UI_UI_TEST_UTILS_HPP