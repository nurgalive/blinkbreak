/// @file app_controller.cpp
/// @brief Implementation of the AppController class.

#include "app_controller.hpp"

#include "main_window.h"
#include "tray_manager.hpp"

#include <slint.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdlib>
#include <format>
#include <limits>
#include <optional>
#include <thread>

namespace blinkbreak {

using namespace std::chrono_literals;

namespace {
std::string ToStdString(const slint::SharedString& value) {
    return std::string(value.data(), value.size());
}

std::optional<int> ParsePositiveInt(const std::string& text) {
    if (text.empty()) {
        return std::nullopt;
    }

    char* end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0') {
        return std::nullopt;
    }
    if (value <= 0 || value > std::numeric_limits<int>::max()) {
        return std::nullopt;
    }

    return static_cast<int>(value);
}
}  // namespace

AppController::AppController()
    : running_(false),
      short_progress_(0.0f),
      long_progress_(0.0f),
      short_break_count_(0),
      long_break_count_(0),
      can_skip_(false),
      skip_in_progress_(false),
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
    config_path_ = ConfigManager::GetDefaultPath();

    auto loaded = config_manager_->Load(config_path_);
    if (loaded) {
        config_ = loaded.value();
        spdlog::info("Configuration loaded from {}", config_path_.string());
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

    spdlog::info(
        "Configured breaks: short[enabled={}, interval={}s, duration={}s] long[enabled={}, interval={}s, duration={}s]",
        config_.short_break.enabled,
        config_.short_break.interval.count(),
        config_.short_break.duration.count(),
        config_.long_break.enabled,
        config_.long_break.interval.count(),
        config_.long_break.duration.count());

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
        bool was_skip = false;
        {
            std::lock_guard lock(mutex_);
            if (info.duration.count() > 0) {
                if (info.type == BreakType::kShort) {
                    ++short_break_count_;
                } else if (info.type == BreakType::kLong) {
                    ++long_break_count_;
                }
            }
            was_skip = skip_in_progress_;
            skip_in_progress_ = false;
        }
        if (was_skip) {
            state_machine_->ProcessEvent(SkipEvent{});
        } else {
            state_machine_->ProcessEvent(BreakCompletedEvent{});
        }
    });

    if (config_.notification.enabled) {
        scheduler_->SetOnWarning(
            [this](BreakType type, Duration time_until) {
                spdlog::info("Warning: {} break in {}s", BreakTypeToString(type),
                             time_until.count());
                if (tray_manager_) {
                    tray_manager_->UpdateStatus(is_running_, time_until, type);
                }
            },
            config_.notification.warning_time);
    }

    // Initialize UI state
    time_until_short_ = FormatDuration(config_.short_break.interval);
    time_until_long_ = FormatDuration(config_.long_break.interval);
    status_text_ = "Ready - Click Start";
    short_progress_ = 0.0f;
    long_progress_ = 0.0f;
    short_break_count_ = 0;
    long_break_count_ = 0;
    can_skip_ = false;
    skip_in_progress_ = false;
    is_running_ = false;

    main_window_ = std::make_unique<slint::ComponentHandle<MainWindow>>(MainWindow::create());
    auto& ui = *main_window_;

    ui->on_start_clicked([this] { OnStart(); });
    ui->on_pause_clicked([this] { OnPause(); });
    ui->on_skip_clicked([this] { OnSkip(); });
    ui->on_settings_clicked([this] { OnOpenSettings(); });
    
    // Handle window close - hide to tray instead of minimizing/quitting
    ui->window().on_close_requested([this]() {
        spdlog::debug("Window close requested - hiding to tray");
        if (main_window_) {
            (*main_window_)->hide();
        }
        return slint::CloseRequestResponse::KeepWindowShown;
    });

    ui->set_time_until_short(slint::SharedString(time_until_short_));
    ui->set_time_until_long(slint::SharedString(time_until_long_));
    ui->set_status_text(slint::SharedString(status_text_));
    ui->set_short_progress(short_progress_);
    ui->set_long_progress(long_progress_);
    ui->set_short_break_count(short_break_count_);
    ui->set_long_break_count(long_break_count_);
    ui->set_can_skip(can_skip_);
    ui->set_is_running(is_running_);

    // Initialize tray manager
    TrayManager::Callbacks tray_callbacks{
        .on_show_window = [this]() {
            spdlog::debug("Tray: Show window");
            if (main_window_) {
                (*main_window_)->show();
                (*main_window_)->window().set_minimized(false);
            }
        },
        .on_start_pause = [this]() {
            spdlog::debug("Tray: Start/Pause");
            if (state_machine_->GetCurrentState() == State::kRunning) {
                OnPause();
            } else {
                OnStart();
            }
        },
        .on_skip = [this]() {
            spdlog::debug("Tray: Skip");
            OnSkip();
        },
        .on_settings = [this]() {
            spdlog::debug("Tray: Settings");
            OnOpenSettings();
        },
        .on_quit = [this]() {
            spdlog::debug("Tray: Quit");
            OnQuit();
        }
    };

    tray_manager_ = std::make_unique<TrayManager>(std::move(tray_callbacks));
    tray_manager_->Show();
    tray_manager_->UpdateStatus(is_running_, config_.short_break.interval,
                                scheduler_->GetNextBreakType());

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

    // Show the main window
    if (main_window_) {
        (*main_window_)->show();
    }

    // Run the event loop (keep running even if no windows are visible)
    slint::run_event_loop(slint::EventLoopMode::RunUntilQuit);

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
            {
                std::lock_guard scheduler_lock(scheduler_mutex_);
                scheduler_->Resume();
            }
            std::lock_guard lock(mutex_);
            status_text_ = "Running";
            if (tray_manager_) {
                tray_manager_->UpdateMenu(true);
            }
        }
        return;
    }

    auto result = state_machine_->ProcessEvent(StartEvent{});
    if (result.success) {
        {
            std::lock_guard scheduler_lock(scheduler_mutex_);
            scheduler_->Start();
        }
        std::lock_guard lock(mutex_);
        status_text_ = "Running";
        if (tray_manager_) {
            tray_manager_->UpdateMenu(true);
        }
    }
}

void AppController::OnPause() {
    spdlog::debug("OnPause called");

    auto result = state_machine_->ProcessEvent(PauseEvent{});
    if (result.success) {
        {
            std::lock_guard scheduler_lock(scheduler_mutex_);
            scheduler_->Pause();
        }
        std::lock_guard lock(mutex_);
        status_text_ = "Paused";
        if (tray_manager_) {
            tray_manager_->UpdateMenu(false);
        }
    }
}

void AppController::OnSkip() {
    spdlog::debug("OnSkip called");

    if (state_machine_->GetCurrentState() == State::kBreakActive) {
        {
            std::lock_guard lock(mutex_);
            skip_in_progress_ = true;
        }
        {
            std::lock_guard scheduler_lock(scheduler_mutex_);
            scheduler_->SkipBreak();
        }
    }
}

void AppController::OnSnooze() {
    spdlog::debug("OnSnooze called");

    if (state_machine_->GetCurrentState() == State::kBreakActive) {
        {
            std::lock_guard scheduler_lock(scheduler_mutex_);
            scheduler_->SnoozeBreak();
        }
        Duration snooze_duration = Duration::zero();
        {
            std::lock_guard lock(mutex_);
            snooze_duration = config_.overlay.snooze_duration;
            status_text_ = "Snoozed";
        }
        state_machine_->ProcessEvent(SnoozeEvent{snooze_duration});
    }
}

void AppController::OnReset() {
    spdlog::debug("OnReset called");

    state_machine_->ProcessEvent(ResetEvent{});
    {
        std::lock_guard scheduler_lock(scheduler_mutex_);
        scheduler_->Reset();
    }
    std::lock_guard lock(mutex_);
    status_text_ = "Ready - Click Start";
    short_progress_ = 0.0f;
    long_progress_ = 0.0f;
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
            if (!settings_dialog_) {
                return;
            }

            auto& dialog = *settings_dialog_;
            dialog->set_validation_error(slint::SharedString(""));

            const auto short_interval_text =
                ToStdString(dialog->get_short_interval_minutes());
            const auto short_duration_text =
                ToStdString(dialog->get_short_duration_seconds());
            const auto long_interval_text =
                ToStdString(dialog->get_long_interval_minutes());
            const auto long_duration_text =
                ToStdString(dialog->get_long_duration_seconds());

            const auto short_interval_minutes = ParsePositiveInt(short_interval_text);
            const auto short_duration_seconds = ParsePositiveInt(short_duration_text);
            const auto long_interval_minutes = ParsePositiveInt(long_interval_text);
            const auto long_duration_seconds = ParsePositiveInt(long_duration_text);

            if (!short_interval_minutes || !short_duration_seconds ||
                !long_interval_minutes || !long_duration_seconds) {
                dialog->set_validation_error(
                    slint::SharedString("All fields must be positive integers."));
                return;
            }

            AppConfig updated{};
            {
                std::lock_guard lock(mutex_);
                updated = config_;
            }
            updated.short_break.interval = Duration(*short_interval_minutes * 60);
            updated.short_break.duration = Duration(*short_duration_seconds);
            updated.long_break.interval = Duration(*long_interval_minutes * 60);
            updated.long_break.duration = Duration(*long_duration_seconds);

            auto errors = config_manager_->Validate(updated);
            if (!errors.empty()) {
                dialog->set_validation_error(slint::SharedString(errors[0].message));
                return;
            }

            if (config_manager_) {
                auto save_result = config_manager_->Save(updated, config_path_);
                if (!save_result) {
                    dialog->set_validation_error(
                        slint::SharedString(save_result.error().message));
                    return;
                }
            }

            {
                std::lock_guard lock(mutex_);
                config_ = updated;
            }
            bool scheduler_running = false;
            {
                std::lock_guard scheduler_lock(scheduler_mutex_);
                if (scheduler_) {
                    scheduler_->UpdateConfig(updated.short_break, updated.long_break,
                                             updated.overlay);
                    scheduler_running = scheduler_->IsRunning();
                }
            }
            if (!scheduler_running) {
                std::lock_guard lock(mutex_);
                time_until_short_ = FormatDuration(updated.short_break.interval);
                time_until_long_ = FormatDuration(updated.long_break.interval);
                short_progress_ = 0.0f;
                long_progress_ = 0.0f;
                status_text_ = "Ready - Click Start";
            }

            BreakType next_break_type = BreakType::kShort;
            {
                std::lock_guard scheduler_lock(scheduler_mutex_);
                if (scheduler_) {
                    next_break_type = scheduler_->GetNextBreakType();
                }
            }

            bool is_running_snapshot = false;
            {
                std::lock_guard lock(mutex_);
                is_running_snapshot = is_running_;
            }
            if (tray_manager_) {
                tray_manager_->UpdateStatus(is_running_snapshot, updated.short_break.interval,
                                            next_break_type);
            }

            UpdateUI();
            dialog->hide();
        });
    }

    auto& dialog = *settings_dialog_;
    AppConfig snapshot{};
    {
        std::lock_guard lock(mutex_);
        snapshot = config_;
    }
    dialog->set_short_interval_minutes(
        slint::SharedString(std::to_string(
            static_cast<int>(snapshot.short_break.interval.count() / 60))));
    dialog->set_short_duration_seconds(
        slint::SharedString(std::to_string(
            static_cast<int>(snapshot.short_break.duration.count()))));
    dialog->set_long_interval_minutes(
        slint::SharedString(std::to_string(
            static_cast<int>(snapshot.long_break.interval.count() / 60))));
    dialog->set_long_duration_seconds(
        slint::SharedString(std::to_string(
            static_cast<int>(snapshot.long_break.duration.count()))));
    dialog->set_validation_error(slint::SharedString(""));

    dialog->show();
}

void AppController::OnQuit() {
    spdlog::info("Quit requested");
    
    running_.store(false);
    
    if (main_window_) {
        slint::invoke_from_event_loop([ui = *main_window_]() mutable {
            ui->hide();
        });
    }
    
    if (tray_manager_) {
        tray_manager_->Hide();
    }
    
    slint::quit_event_loop();
}

std::string AppController::GetTimeRemainingString() const {
    std::lock_guard lock(mutex_);
    return time_until_short_;
}

std::string AppController::GetTimeUntilShortBreakString() const {
    std::lock_guard lock(mutex_);
    return time_until_short_;
}

std::string AppController::GetTimeUntilLongBreakString() const {
    std::lock_guard lock(mutex_);
    return time_until_long_;
}

float AppController::GetShortProgress() const {
    std::lock_guard lock(mutex_);
    return short_progress_;
}

float AppController::GetLongProgress() const {
    std::lock_guard lock(mutex_);
    return long_progress_;
}

int AppController::GetShortBreakCount() const {
    std::lock_guard lock(mutex_);
    return short_break_count_;
}

int AppController::GetLongBreakCount() const {
    std::lock_guard lock(mutex_);
    return long_break_count_;
}

bool AppController::GetCanSkip() const {
    std::lock_guard lock(mutex_);
    return can_skip_;
}

bool AppController::IsSkipInProgress() const {
    std::lock_guard lock(mutex_);
    return skip_in_progress_;
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

        {
            std::lock_guard scheduler_lock(scheduler_mutex_);
            scheduler_->Update(delta);
        }
        UpdateUI();

        std::this_thread::sleep_for(kTickInterval);
    }

    spdlog::debug("Timer thread stopped");
}

void AppController::UpdateUI() {
    std::string time_until_short;
    std::string time_until_long;
    std::string status_text;
    float short_progress = 0.0f;
    float long_progress = 0.0f;
    int short_break_count = 0;
    int long_break_count = 0;
    bool can_skip = false;
    bool is_running = false;

    {
        std::lock_guard scheduler_lock(scheduler_mutex_);
        std::lock_guard lock(mutex_);

        auto time_until_short_opt = scheduler_->GetTimeUntilShortBreak();
        auto time_until_long_opt = scheduler_->GetTimeUntilLongBreak();
        auto time_until_next = scheduler_->GetTimeUntilNextBreak();

        if (time_until_short_opt) {
            time_until_short_ = FormatDuration(*time_until_short_opt);
        }
        if (time_until_long_opt) {
            time_until_long_ = FormatDuration(*time_until_long_opt);
        }

        if (time_until_short_opt && config_.short_break.interval.count() > 0) {
            short_progress_ =
                1.0f - (static_cast<float>(time_until_short_opt->count()) /
                        static_cast<float>(config_.short_break.interval.count()));
        }

        if (time_until_long_opt && config_.long_break.interval.count() > 0) {
            long_progress_ =
                1.0f - (static_cast<float>(time_until_long_opt->count()) /
                        static_cast<float>(config_.long_break.interval.count()));
        }

        can_skip_ = scheduler_->IsBreakActive() && config_.overlay.allow_skip;

        time_until_short = time_until_short_;
        time_until_long = time_until_long_;
        status_text = status_text_;
        short_progress = short_progress_;
        long_progress = long_progress_;
        short_break_count = short_break_count_;
        long_break_count = long_break_count_;
        can_skip = can_skip_;
        is_running = is_running_;

        // Update tray status
        if (tray_manager_ && time_until_next) {
            tray_manager_->UpdateStatus(is_running_, *time_until_next,
                                        scheduler_->GetNextBreakType());
        }
    }

    if (!main_window_) {
        return;
    }

    auto ui_handle = *main_window_;
    slint::invoke_from_event_loop([ui_handle,
                                   time_until_short = slint::SharedString(time_until_short),
                                   time_until_long = slint::SharedString(time_until_long),
                                   status_text = slint::SharedString(status_text),
                                   short_progress,
                                   long_progress,
                                   short_break_count,
                                   long_break_count,
                                   can_skip,
                                   is_running]() mutable {
        ui_handle->set_time_until_short(time_until_short);
        ui_handle->set_time_until_long(time_until_long);
        ui_handle->set_status_text(status_text);
        ui_handle->set_short_progress(short_progress);
        ui_handle->set_long_progress(long_progress);
        ui_handle->set_short_break_count(short_break_count);
        ui_handle->set_long_break_count(long_break_count);
        ui_handle->set_can_skip(can_skip);
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