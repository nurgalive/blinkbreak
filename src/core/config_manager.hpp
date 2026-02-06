/// @file config_manager.hpp
/// @brief Configuration management system.

#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "config_types.hpp"

namespace blinkbreak {

/// @brief Error type for configuration operations.
struct ConfigError {
    std::string message;  ///< Error message.
    std::string field;    ///< Field that caused the error (if applicable).
};

/// @brief Result type for configuration operations.
template <typename T>
using ConfigResult = std::expected<T, ConfigError>;

/// @brief Manages application configuration loading, saving, and validation.
///
/// The ConfigManager handles all configuration file operations including:
/// - Loading configuration from JSON files
/// - Saving configuration to JSON files
/// - Validating configuration values
/// - Providing default configuration
///
/// @code
/// ConfigManager config_manager;
/// auto config = config_manager.Load("config.json");
/// if (config) {
///     // Use config.value()
/// }
/// @endcode
class ConfigManager {
public:
    /// @brief Constructs a ConfigManager.
    ConfigManager();

    /// @brief Destructor.
    ~ConfigManager();

    // Non-copyable, non-movable
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;

    /// @brief Loads configuration from a file.
    /// @param path Path to the configuration file.
    /// @return The loaded configuration or an error.
    [[nodiscard]] ConfigResult<AppConfig> Load(const std::filesystem::path& path);

    /// @brief Saves configuration to a file.
    /// @param config The configuration to save.
    /// @param path Path to save the configuration to.
    /// @return Success or an error.
    [[nodiscard]] ConfigResult<void> Save(const AppConfig& config,
                                          const std::filesystem::path& path);

    /// @brief Validates a configuration.
    /// @param config The configuration to validate.
    /// @return List of validation errors (empty if valid).
    [[nodiscard]] std::vector<ConfigError> Validate(const AppConfig& config) const;

    /// @brief Gets the default configuration.
    /// @return The default configuration with sensible values.
    [[nodiscard]] static AppConfig GetDefault();

    /// @brief Gets the default configuration file path.
    /// @return Platform-specific default config path.
    [[nodiscard]] static std::filesystem::path GetDefaultPath();

    /// @brief Parses configuration from a JSON string.
    /// @param json_string The JSON string to parse.
    /// @return The parsed configuration or an error.
    [[nodiscard]] ConfigResult<AppConfig> ParseJson(const std::string& json_string);

    /// @brief Converts configuration to a JSON string.
    /// @param config The configuration to convert.
    /// @return The JSON string representation.
    [[nodiscard]] std::string ToJson(const AppConfig& config) const;

private:
    /// @brief Validates a break configuration.
    /// @param config The break config to validate.
    /// @param name Name of the break type for error messages.
    /// @return List of validation errors.
    [[nodiscard]] std::vector<ConfigError> ValidateBreakConfig(const BreakConfig& config,
                                                               const std::string& name) const;
};

}  // namespace blinkbreak
