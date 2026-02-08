/// @file app_controller.cpp
/// @brief Implementation of the AppController class.

#include "app_controller.hpp"

#include "main_window.h"

#include <slint.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <format>
#include <thread>

namespace blinkbreak {

using namespace std::chrono_literals;

AppController::AppController()
    : running_(false),
      progress_(0.0f),
      is_running_(false) {
    spdlog::debug("AppController created");
}

AppController::~AppController() {
    running_.store(false);
    if (timer_thread_.joinable()) {
        timer_thread_.join();
    }
    spdlog::debug("AppController destroyed");
}

bool AppController::Initialize() {
    spdlog::info("Initializing AppController");

    // Load configuration
    config_manager_ = std::make_unique<ConfigManager>();
    auto config_path = ConfigManager::GetDefaultPath();

    auto loaded = config_manager_->Load(config_path);
    if (loaded) {
        config_ = loaded.value();
        spdlog::info("Configuration loaded from {}", config_path.string());
    } else {
        config_ = ConfigManager::GetDefault();
        spdlog::info("Using default configuration");
    }

    // Validate configuration
    auto errors = config_manager_->Validate(config_);
    if (!errors.empty()) {
        spdlog::error("Configuration validation failed");
        for (const auto& error : errors) {
            spdlog::error("  {}: {}", error.field, error.message);
        }
        return false;
    }

    // Create state machine
    state_machine_ = std::make_unique<StateMachine>();
    state_machine_->SetOnStateChange([this](State old_state, State new_state, const Event&) {
        spdlog::info("State: {} -> {}", StateToString(old_state), StateToString(new_state));
        std::lock_guard lock(mutex_);
        is_running_ = (new_state == State::kRunning);
    });

    // Create break scheduler
    scheduler_ = std::make_unique<BreakScheduler>(config_.short_break, config_.long_break,
                                                  config_.overlay);

    scheduler_->SetOnBreakStart([this](const BreakInfo& info) {
        spdlog::info("Break started: {}", BreakTypeToString(info.type));
        state_machine_->ProcessEvent(TimerExpiredEvent{info.type});
    });

    scheduler_->SetOnBreakEnd([this](const BreakInfo& info) {
        spdlog::info("Break ended: {}", BreakTypeToString(info.type));
        state_machine_->ProcessEvent(BreakCompletedEvent{});
    });

    if (config_.notification.enabled) {
        scheduler_->SetOnWarning(
            [](BreakType type, Duration time_until) {
                spdlog::info("Warning: {} break in {}s", BreakTypeToString(type),
                             time_until.count());
            },
            config_.notification.warning_time);
    }

    // Initialize UI state
    time_remaining_ = FormatDuration(config_.short_break.interval);
    status_text_ = "Ready - Click Start";
    progress_ = 0.0f;
    is_running_ = false;

    main_window_ = std::make_unique<slint::ComponentHandle<MainWindow>>(MainWindow::create());
    auto& ui = *main_window_;

    ui->on_start_clicked([this] { OnStart(); });
    ui->on_pause_clicked([this] { OnPause(); });
    ui->on_skip_clicked([this] { OnSkip(); });
    ui->on_settings_clicked([this] { OnOpenSettings(); });

    ui->set_time_remaining(slint::SharedString(time_remaining_));
    ui->set_status_text(slint::SharedString(status_text_));
    ui->set_progress(progress_);
    ui->set_is_running(is_running_);
    ui->set_next_break_type(
        slint::SharedString(BreakTypeToString(scheduler_->GetNextBreakType())));

    spdlog::info("AppController initialized successfully");
    return true;
}

int AppController::Run() {
    spdlog::info("Starting application");

    // Start timer thread
    running_.store(true);
    timer_thread_ = std::thread(&AppController::TimerThreadFunc, this);

    // Auto-start if configured
    if (config_.auto_start) {
        OnStart();
    }

    spdlog::info("Application running...");

    if (main_window_) {
        (*main_window_)->run();
    }

    running_.store(false);
    if (timer_thread_.joinable()) {
        timer_thread_.join();
    }

    return 0;
}

void AppController::OnStart() {
    spdlog::debug("OnStart called");

    const auto current_state = state_machine_->GetCurrentState();
    if (current_state == State::kPaused) {
        auto result = state_machine_->ProcessEvent(ResumeEvent{});
        if (result.success) {
            scheduler_->Resume();
            std::lock_guard lock(mutex_);
            status_text_ = "Running";
        }
        return;
    }

    auto result = state_machine_->ProcessEvent(StartEvent{});
    if (result.success) {
        scheduler_->Start();
        std::lock_guard lock(mutex_);
        status_text_ = "Running";
    }
}

void AppController::OnPause() {
    spdlog::debug("OnPause called");

    auto result = state_machine_->ProcessEvent(PauseEvent{});
    if (result.success) {
        scheduler_->Pause();
        std::lock_guard lock(mutex_);
        status_text_ = "Paused";
    }
}

void AppController::OnSkip() {
    spdlog::debug("OnSkip called");

    if (state_machine_->GetCurrentState() == State::kBreakActive) {
        scheduler_->SkipBreak();
        state_machine_->ProcessEvent(SkipEvent{});
    }
}

void AppController::OnSnooze() {
    spdlog::debug("OnSnooze called");

    if (state_machine_->GetCurrentState() == State::kBreakActive) {
        scheduler_->SnoozeBreak();
        state_machine_->ProcessEvent(SnoozeEvent{config_.overlay.snooze_duration});
        std::lock_guard lock(mutex_);
        status_text_ = "Snoozed";
    }
}

void AppController::OnReset() {
    spdlog::debug("OnReset called");

    state_machine_->ProcessEvent(ResetEvent{});
    scheduler_->Reset();
    std::lock_guard lock(mutex_);
    status_text_ = "Ready - Click Start";
    progress_ = 0.0f;
}

void AppController::OnOpenSettings() {
    spdlog::debug("OnOpenSettings called");

    if (!settings_dialog_) {
        settings_dialog_ =
            std::make_unique<slint::ComponentHandle<SettingsDialog>>(SettingsDialog::create());
        auto& dialog = *settings_dialog_;

        dialog->on_cancel_clicked([this] {
            if (settings_dialog_) {
                (*settings_dialog_)->hide();
            }
        });

        dialog->on_save_clicked([this] {
            if (settings_dialog_) {
                (*settings_dialog_)->hide();
            }
        });
    }

    (*settings_dialog_)->show();
}

std::string AppController::GetTimeRemainingString() const {
    std::lock_guard lock(mutex_);
    return time_remaining_;
}

float AppController::GetProgress() const {
    std::lock_guard lock(mutex_);
    return progress_;
}

std::string AppController::GetStatusText() const {
    std::lock_guard lock(mutex_);
    return status_text_;
}

void AppController::TimerThreadFunc() {
    spdlog::debug("Timer thread started");

    auto last_time = std::chrono::steady_clock::now();
    constexpr auto kTickInterval = 100ms;

    while (running_.load()) {
        auto now = std::chrono::steady_clock::now();
        auto delta = std::chrono::duration_cast<DurationMs>(now - last_time);
        last_time = now;

        scheduler_->Update(delta);
        UpdateUI();

        std::this_thread::sleep_for(kTickInterval);
    }

    spdlog::debug("Timer thread stopped");
}

void AppController::UpdateUI() {
    std::string time_remaining;
    std::string status_text;
    std::string next_break_type;
    float progress = 0.0f;
    bool is_running = false;

    {
        std::lock_guard lock(mutex_);

        auto time_until = scheduler_->GetTimeUntilNextBreak();
        if (time_until) {
            time_remaining_ = FormatDuration(*time_until);

            // Calculate progress based on next break type
            Duration total = scheduler_->GetNextBreakType() == BreakType::kShort
                                 ? config_.short_break.interval
                                 : config_.long_break.interval;

            if (total.count() > 0) {
                progress_ = 1.0f - (static_cast<float>(time_until->count()) /
                                    static_cast<float>(total.count()));
            }
        }

        time_remaining = time_remaining_;
        status_text = status_text_;
        progress = progress_;
        is_running = is_running_;
        next_break_type = BreakTypeToString(scheduler_->GetNextBreakType());
    }

    if (!main_window_) {
        return;
    }

    auto ui_handle = *main_window_;
    slint::invoke_from_event_loop([ui_handle,
                                   time_remaining = slint::SharedString(time_remaining),
                                   status_text = slint::SharedString(status_text),
                                   next_break_type = slint::SharedString(next_break_type),
                                   progress,
                                   is_running]() mutable {
        ui_handle->set_time_remaining(time_remaining);
        ui_handle->set_status_text(status_text);
        ui_handle->set_next_break_type(next_break_type);
        ui_handle->set_progress(progress);
        ui_handle->set_is_running(is_running);
    });
}

std::string AppController::FormatDuration(Duration duration) {
    auto total_seconds = duration.count();
    auto minutes = total_seconds / 60;
    auto seconds = total_seconds % 60;
    return std::format("{:02}:{:02}", minutes, seconds);
}

}  // namespace blinkbreak