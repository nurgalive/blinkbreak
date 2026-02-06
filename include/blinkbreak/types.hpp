/// @file types.hpp
/// @brief Common type definitions for BlinkBreak application.

#ifndef BLINKBREAK_TYPES_HPP
#define BLINKBREAK_TYPES_HPP

#include <chrono>
#include <cstdint>
#include <string>

namespace blinkbreak {

/// @brief Duration type using seconds.
using Duration = std::chrono::seconds;

/// @brief Duration type using milliseconds.
using DurationMs = std::chrono::milliseconds;

/// @brief Time point using steady clock for timer operations.
using TimePoint = std::chrono::steady_clock::time_point;

/// @brief Break type enumeration.
enum class BreakType : std::uint8_t {
    kShort,  ///< Short break (e.g., 20 seconds every 10 minutes)
    kLong    ///< Long break (e.g., 5 minutes every hour)
};

/// @brief Converts BreakType to string representation.
/// @param type The break type to convert.
/// @return String representation of the break type.
[[nodiscard]] constexpr const char* BreakTypeToString(BreakType type) {
    switch (type) {
        case BreakType::kShort:
            return "Short";
        case BreakType::kLong:
            return "Long";
    }
    return "Unknown";
}

}  // namespace blinkbreak

#endif  // BLINKBREAK_TYPES_HPP