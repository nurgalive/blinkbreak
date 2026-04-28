/// @file tray_manager.cpp
/// @brief Implementation of the TrayManager class.

#include "tray_manager.hpp"

#include <format>
#include <spdlog/spdlog.h>

namespace blinkbreak {

TrayManager::TrayManager(Callbacks callbacks)
    : callbacks_(std::move(callbacks)),
      is_running_(false) {
    tray_icon_ = platform::CreateTrayIcon();
    tray_icon_->SetIcon(0);

    tray_icon_->SetOnDoubleClick([this]() {
        if (callbacks_.on_show_window) {
            callbacks_.on_show_window();
        }
    });

    UpdateMenu(false);
    spdlog::debug("TrayManager created");
}

TrayManager::~TrayManager() {
    Hide();
    spdlog::debug("TrayManager destroyed");
}

bool TrayManager::Show() {
    return tray_icon_->Show();
}

void TrayManager::SetHostWindow(std::uintptr_t native_window_handle) {
    tray_icon_->SetHostWindow(native_window_handle);
}

void TrayManager::Hide() {
    tray_icon_->Hide();
}

void TrayManager::UpdateStatus(bool is_running, Duration time_until_break, BreakType break_type) {
    std::string tooltip;

    if (is_running) {
        auto minutes = time_until_break.count() / 60;
        auto seconds = time_until_break.count() % 60;
        tooltip = std::format("BlinkBreak - {} break in {:02}:{:02}", BreakTypeToString(break_type),
                              minutes, seconds);
    } else {
        tooltip = "BlinkBreak - Paused";
    }

    tray_icon_->SetTooltip(tooltip);
    tray_icon_->SetIcon(is_running ? 1 : 0);

    if (is_running != is_running_) {
        UpdateMenu(is_running);
        is_running_ = is_running;
    }
}

void TrayManager::UpdateMenu(bool is_running) {
    std::vector<platform::MenuItem> items;
    tray_icon_->SetIcon(is_running ? 1 : 0);

    // Show window
    items.push_back(
        {.text = "Show BlinkBreak", .callback = callbacks_.on_show_window, .enabled = true});

    items.push_back({.is_separator = true});

    // Start/Pause
    items.push_back({.text = is_running ? "Pause" : "Start",
                     .callback = callbacks_.on_start_pause,
                     .enabled = true});

    // Skip next break
    items.push_back(
        {.text = "Skip Next Break", .callback = callbacks_.on_skip, .enabled = is_running});

    items.push_back({.is_separator = true});

    // Settings
    items.push_back({.text = "Settings...", .callback = callbacks_.on_settings, .enabled = true});

    items.push_back({.is_separator = true});

    // Quit
    items.push_back({.text = "Quit", .callback = callbacks_.on_quit, .enabled = true});

    tray_icon_->SetMenu(items);
}

}  // namespace blinkbreak
