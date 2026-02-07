/// @file message_provider.cpp
/// @brief Implementation of the MessageProvider class.

#include "message_provider.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <numeric>

namespace blinkbreak {

MessageProvider::MessageProvider(std::vector<std::string> messages,
                                 bool rotate,
                                 bool random)
    : messages_(std::move(messages)),
      current_index_(0),
      rotate_(rotate),
      random_(random),
      rng_(std::random_device{}()) {
    if (messages_.empty()) {
        messages_.push_back("Take a break!");
        spdlog::warn("MessageProvider created with empty messages, using default");
    }

    indices_.resize(messages_.size());
    std::iota(indices_.begin(), indices_.end(), 0);
    ShuffleIfNeeded();

    spdlog::debug("MessageProvider created with {} messages, rotate={}, random={}",
                  messages_.size(), rotate_, random_);
}

MessageProvider::~MessageProvider() = default;

MessageProvider::MessageProvider(MessageProvider&& other) noexcept
    : messages_(std::move(other.messages_)),
      indices_(std::move(other.indices_)),
      current_index_(other.current_index_),
      rotate_(other.rotate_),
      random_(other.random_),
      rng_(std::move(other.rng_)) {}

MessageProvider& MessageProvider::operator=(MessageProvider&& other) noexcept {
    if (this != &other) {
        messages_ = std::move(other.messages_);
        indices_ = std::move(other.indices_);
        current_index_ = other.current_index_;
        rotate_ = other.rotate_;
        random_ = other.random_;
        rng_ = std::move(other.rng_);
    }
    return *this;
}

std::string MessageProvider::GetNext() {
    if (messages_.empty()) {
        return "Take a break!";
    }

    std::size_t actual_index = random_ ? indices_[current_index_] : current_index_;
    std::string message = messages_[actual_index];

    if (rotate_) {
        current_index_ = (current_index_ + 1) % messages_.size();

        if (current_index_ == 0 && random_) {
            ShuffleIfNeeded();
        }
    }

    spdlog::debug("MessageProvider returning: '{}'", message);
    return message;
}

std::string_view MessageProvider::GetCurrent() const {
    if (messages_.empty()) {
        return "Take a break!";
    }

    std::size_t actual_index = random_ ? indices_[current_index_] : current_index_;
    return messages_[actual_index];
}

std::size_t MessageProvider::GetMessageCount() const {
    return messages_.size();
}

void MessageProvider::Reset() {
    current_index_ = 0;
    ShuffleIfNeeded();
    spdlog::debug("MessageProvider reset");
}

void MessageProvider::SetMessages(std::vector<std::string> messages) {
    messages_ = std::move(messages);
    if (messages_.empty()) {
        messages_.push_back("Take a break!");
    }

    indices_.resize(messages_.size());
    std::iota(indices_.begin(), indices_.end(), 0);
    current_index_ = 0;
    ShuffleIfNeeded();

    spdlog::debug("MessageProvider messages updated, count={}", messages_.size());
}

void MessageProvider::SetRotate(bool rotate) {
    rotate_ = rotate;
    spdlog::debug("MessageProvider rotate set to {}", rotate_);
}

void MessageProvider::SetRandom(bool random) {
    bool was_random = random_;
    random_ = random;

    if (random_ && !was_random) {
        ShuffleIfNeeded();
    }

    spdlog::debug("MessageProvider random set to {}", random_);
}

void MessageProvider::ShuffleIfNeeded() {
    if (random_ && indices_.size() > 1) {
        std::shuffle(indices_.begin(), indices_.end(), rng_);
        spdlog::debug("MessageProvider shuffled indices");
    }
}

}  // namespace blinkbreak