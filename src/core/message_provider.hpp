/// @file message_provider.hpp
/// @brief Message provider for break notifications.

#ifndef BLINKBREAK_CORE_MESSAGE_PROVIDER_HPP
#define BLINKBREAK_CORE_MESSAGE_PROVIDER_HPP

#include <random>
#include <string>
#include <vector>

#include <blinkbreak/types.hpp>

namespace blinkbreak
{

/// @brief Provides messages for break notifications.
///
/// The MessageProvider manages a list of messages and provides them
/// either sequentially or randomly based on configuration.
///
/// @code
/// MessageProvider provider({"Message 1", "Message 2"}, true);
/// std::string msg = provider.GetNext();
/// @endcode
class MessageProvider
{
public:
  /// @brief Constructs a message provider with the given messages.
  /// @param messages List of messages to provide.
  /// @param rotate Whether to rotate through messages.
  /// @param random Whether to randomize message order (if rotating).
  explicit MessageProvider(std::vector<std::string> messages, bool rotate = true,
                           bool random = false);

  /// @brief Destructor.
  ~MessageProvider();

  // Non-copyable
  MessageProvider(const MessageProvider&) = delete;
  MessageProvider& operator=(const MessageProvider&) = delete;

  // Movable
  MessageProvider(MessageProvider&&) noexcept;
  MessageProvider& operator=(MessageProvider&&) noexcept;

  /// @brief Gets the next message.
  /// @return The next message in the rotation, or the single message if not rotating.
  [[nodiscard]] std::string GetNext();

  /// @brief Gets the current message without advancing.
  /// @return The current message.
  [[nodiscard]] std::string_view GetCurrent() const;

  /// @brief Gets the number of messages.
  /// @return The message count.
  [[nodiscard]] std::size_t GetMessageCount() const;

  /// @brief Resets the rotation to the beginning.
  void Reset();

  /// @brief Sets new messages.
  /// @param messages New list of messages.
  void SetMessages(std::vector<std::string> messages);

  /// @brief Enables or disables rotation.
  /// @param rotate Whether to rotate messages.
  void SetRotate(bool rotate);

  /// @brief Enables or disables random order.
  /// @param random Whether to randomize order.
  void SetRandom(bool random);

private:
  /// @brief Shuffles the messages if random mode is enabled.
  void ShuffleIfNeeded();

  std::vector<std::string> messages_;  ///< The message list.
  std::vector<std::size_t> indices_;   ///< Shuffled indices for random mode.
  std::size_t current_index_;          ///< Current position in rotation.
  bool rotate_;                        ///< Whether rotation is enabled.
  bool random_;                        ///< Whether random order is enabled.
  std::mt19937 rng_;                   ///< Random number generator.
};

}  // namespace blinkbreak

#endif  // BLINKBREAK_CORE_MESSAGE_PROVIDER_HPP
