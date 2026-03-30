/// @file app_controller.cpp
/// @brief Implementation of the AppController class.

#include "app_controller.hpp"

#include <chrono>
#include <cstdlib>
#include <format>
#include <limits>
#include <optional>
#include <slint.h>
#include <spdlog/spdlog.h>
#include <thread>

#ifdef _WIN32
    #include <windows.h>
    #include <shellapi.h>
#endif

#include "main_window.h"
#include "platform/platform_interface.hpp"
#include "tray_manager.hpp"

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
    if (value <= 0 || value > (std::numeric_limits<int>::max)()) {
        return std::nullopt;
    }

    return static_cast<int>(value);
}

template <typename T>
void ApplyThemeProperties(const slint::ComponentHandle<T>& component, const ThemeConfig& theme) {
    component->set_theme_follow_system(theme.follow_system);
    component->set_theme_dark_mode(theme.dark_mode);
}

#ifdef _WIN32
/// @brief Finds the native HWND of a window by its title.
/// FindWindowW does not search HWND_MESSAGE windows, so it won't return
/// the tray icon's hidden message window even though it shares the same title.
HWND FindWindowByTitle(const wchar_t* title) {
    HWND hwnd = FindWindowW(nullptr, title);
    if (!hwnd) {
        spdlog::warn("FindWindowByTitle: could not find '{}'",
                     std::string(title, title + wcslen(title)));
    }
    return hwnd;
}

bool IsDndActive() {
    QUERY_USER_NOTIFICATION_STATE state = QUNS_ACCEPTS_NOTIFICATIONS;
    const HRESULT result = SHQueryUserNotificationState(&state);
    if (FAILED(result)) {
        spdlog::debug("SHQueryUserNotificationState failed: 0x{:08x}",
                      static_cast<unsigned int>(result));
        return false;
    }

    switch (state) {
        case QUNS_BUSY:
        case QUNS_PRESENTATION_MODE:
        case QUNS_RUNNING_D3D_FULL_SCREEN:
        case QUNS_QUIET_TIME:
            return true;
        case QUNS_ACCEPTS_NOTIFICATIONS:
        case QUNS_APP:
        default:
            return false;
    }
}
#endif

}  // namespace

AppController::AppController()
    : running_(false),
      short_progress_(0.0f),
      long_progress_(0.0f),
      short_break_count_(0),
      long_break_count_(0),
      short_skipped_count_(0),
      long_skipped_count_(0),
      tracked_duration_(Duration::zero()),
      tracked_duration_ms_(DurationMs::zero()),
      skip_in_progress_(false),
      pending_notification_break_(std::nullopt),
      pending_notification_action_(std::nullopt),
      active_toast_id_(-1),
      current_state_(State::kIdle),
      is_running_(false),
      is_paused_by_idle_(false),
      show_idle_timer_(false),
      idle_time_("00:00") {
    spdlog::debug("AppController created");
}

AppController::~AppController() {
    running_.store(false);

    // Stop idle detector first
    if (idle_detector_) {
        idle_detector_->Stop();
    }

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
        "Configured breaks: short[enabled={}, interval={}s, duration={}s] long[enabled={}, "
        "interval={}s, duration={}s]",
        config_.short_break.enabled, config_.short_break.interval.count(),
        config_.short_break.duration.count(), config_.long_break.enabled,
        config_.long_break.interval.count(), config_.long_break.duration.count());

    // Create state machine
    state_machine_ = std::make_unique<StateMachine>();
    state_machine_->SetOnStateChange([this](State old_state, State new_state, const Event&) {
        spdlog::info("State: {} -> {}", StateToString(old_state), StateToString(new_state));
        std::lock_guard lock(mutex_);
        current_state_ = new_state;
        is_running_ = (new_state == State::kRunning || new_state == State::kSnoozed);
    });

    // Create break scheduler
    scheduler_ =
        std::make_unique<BreakScheduler>(config_.short_break, config_.long_break, config_.overlay);

    // Create monitor manager
    monitor_manager_ = platform::CreateMonitorManager();

    // Create overlay manager with multi-monitor support
    overlay_manager_ = std::make_unique<OverlayManager>();
    overlay_manager_->SetMonitorManager(monitor_manager_);
    overlay_manager_->SetShowOnAllMonitors(config_.overlay.show_on_all_monitors);
    overlay_manager_->SetOnSkip([this] { OnSkip(); });
    overlay_manager_->SetOnSnooze([this] { OnSnooze(); });
    overlay_manager_->UpdateOpacity(config_.overlay.opacity);

    // Create idle detector if enabled
    if (config_.idle.enabled) {
        idle_detector_ = platform::CreateIdleDetector();
        if (idle_detector_) {
            idle_detector_->SetIdleThreshold(
                std::chrono::duration_cast<std::chrono::seconds>(config_.idle.threshold));
            idle_detector_->SetOnIdle([this]() { OnUserIdle(); });
            idle_detector_->SetOnActive([this]() { OnUserActive(); });
            spdlog::info("Idle detection enabled with threshold={}s",
                         config_.idle.threshold.count());
        }
    } else {
        spdlog::info("Idle detection disabled");
    }

    scheduler_->SetOnBreakStart([this](const BreakInfo& info) {
        spdlog::info("Break started: {}", BreakTypeToString(info.type));
        const auto current_state = GetCurrentStateSnapshot();
        if (current_state == State::kSnoozed) {
            state_machine_->ProcessEvent(SnoozeExpiredEvent{});
        } else {
            state_machine_->ProcessEvent(TimerExpiredEvent{info.type});
        }

        if (notification_manager_) {
            int64_t toast_id = -1;
            {
                std::lock_guard lock(mutex_);
                toast_id = active_toast_id_;
                active_toast_id_ = -1;
            }
            if (toast_id >= 0) {
                notification_manager_->Hide(toast_id);
            }
        }

        std::optional<platform::NotificationAction> pending_action;
        {
            std::lock_guard lock(mutex_);
            if (pending_notification_action_ && pending_notification_break_ &&
                *pending_notification_break_ == info.type) {
                pending_action = pending_notification_action_;
                pending_notification_action_.reset();
                pending_notification_break_.reset();
            }
        }

        if (pending_action && *pending_action == platform::NotificationAction::SkipBreak) {
            slint::invoke_from_event_loop([this]() { OnSkip(); });
            return;
        }
        if (pending_action && *pending_action == platform::NotificationAction::SnoozeBreak) {
            slint::invoke_from_event_loop([this]() { OnSnooze(); });
            return;
        }

        float overlay_opacity = 0.7f;
        {
            std::lock_guard lock(mutex_);
            status_text_ = "Break active";
            overlay_opacity = config_.overlay.opacity;
        }
        if (overlay_manager_) {
            overlay_manager_->UpdateOpacity(overlay_opacity);
            overlay_manager_->Show(info);
        }
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
            } else {
                if (info.type == BreakType::kShort) {
                    ++short_skipped_count_;
                } else if (info.type == BreakType::kLong) {
                    ++long_skipped_count_;
                }
            }
            status_text_ = "Running";
            was_skip = skip_in_progress_;
            skip_in_progress_ = false;
        }
        if (overlay_manager_) {
            overlay_manager_->Hide();
        }
        if (was_skip) {
            state_machine_->ProcessEvent(SkipEvent{});
        } else {
            state_machine_->ProcessEvent(BreakCompletedEvent{});
        }
    });

    const auto warning_time = config_.notification.enabled ? config_.notification.warning_time
                                                            : Duration::zero();
    scheduler_->SetOnWarning(
        [this](BreakType type, Duration time_until) {
            spdlog::info("Warning: {} break in {}s", BreakTypeToString(type),
                         time_until.count());
            if (tray_manager_) {
                tray_manager_->UpdateStatus(is_running_, time_until, type);
            }
            slint::invoke_from_event_loop([this, type, time_until]() {
                ShowPreBreakNotification(type, time_until);
            });
        },
        warning_time);

    // Initialize UI state
    time_until_short_ = FormatDuration(config_.short_break.interval);
    time_until_long_ = FormatDuration(config_.long_break.interval);
    tracked_duration_ = Duration::zero();
    tracked_duration_ms_ = DurationMs::zero();
    tracked_time_ = FormatDuration(tracked_duration_);
    status_text_ = "Ready - Click Start";
    short_progress_ = 0.0f;
    long_progress_ = 0.0f;
    short_break_count_ = 0;
    long_break_count_ = 0;
    short_skipped_count_ = 0;
    long_skipped_count_ = 0;
    skip_in_progress_ = false;
    is_running_ = false;
    show_idle_timer_ = config_.idle.show_timer;
    idle_time_ = "00:00";

    main_window_ = std::make_unique<slint::ComponentHandle<MainWindow>>(MainWindow::create());
    main_window_weak_ = std::make_unique<slint::ComponentWeakHandle<MainWindow>>(*main_window_);
    auto& ui = *main_window_;

    ui->on_start_clicked([this] { OnStart(); });
    ui->on_pause_clicked([this] { OnPause(); });
    ui->on_settings_clicked([this] { OnOpenSettings(); });

    // Handle window close - hide to tray instead of minimizing/quitting.
    // We use Win32 ShowWindow(SW_HIDE) instead of Slint hide() because the
    // software renderer loses its rendering buffer across Slint hide/show
    // cycles, causing a blank white window on restore.
    ui->window().on_close_requested([this]() {
        spdlog::debug("Window close requested - hiding to tray");
#ifdef _WIN32
        HWND hwnd = FindWindowByTitle(L"BlinkBreak");
        if (hwnd) {
            ShowWindow(hwnd, SW_HIDE);
        }
#else
        if (main_window_) {
            (*main_window_)->hide();
        }
#endif
        return slint::CloseRequestResponse::KeepWindowShown;
    });

    ui->set_time_until_short(slint::SharedString(time_until_short_));
    ui->set_time_until_long(slint::SharedString(time_until_long_));
    ui->set_tracked_time(slint::SharedString(tracked_time_));
    ui->set_status_text(slint::SharedString(status_text_));
    ui->set_short_progress(short_progress_);
    ui->set_long_progress(long_progress_);
    ui->set_short_break_count(short_break_count_);
    ui->set_long_break_count(long_break_count_);
    ui->set_short_skipped_count(short_skipped_count_);
    ui->set_long_skipped_count(long_skipped_count_);
    ui->set_is_running(is_running_);
    ui->set_show_idle_timer(show_idle_timer_);
    ui->set_idle_time(slint::SharedString(idle_time_));
    ApplyThemeProperties(*main_window_, config_.theme);

    // Initialize notification manager
    notification_manager_ = platform::CreateNotificationManager();
    if (notification_manager_) {
        if (notification_manager_->IsSupported()) {
            if (!notification_manager_->Initialize()) {
                spdlog::warn("Notification manager initialization failed");
            }
        } else {
            spdlog::warn("Toast notifications are not supported on this system");
        }
        notification_manager_->SetOnAction(
            [this](platform::NotificationAction action) {
                slint::invoke_from_event_loop(
                    [this, action]() { OnNotificationAction(action); });
            });
    }

    // Initialize tray manager
    TrayManager::Callbacks tray_callbacks{
        .on_show_window =
            [this]() {
                spdlog::debug("Tray: Show window");
#ifdef _WIN32
                HWND hwnd = FindWindowByTitle(L"BlinkBreak");
                if (hwnd) {
                    ShowWindow(hwnd, SW_SHOW);
                    SetForegroundWindow(hwnd);

                    // Workaround for winit-software renderer bug:
                    // Force a physical resize event so Slint recreates its pixel buffer.
                    RECT rect;
                    if (GetWindowRect(hwnd, &rect)) {
                        int width = rect.right - rect.left;
                        int height = rect.bottom - rect.top;
                        SetWindowPos(hwnd, nullptr, 0, 0, width + 1, height,
                                     SWP_NOMOVE | SWP_NOZORDER);
                        slint::Timer::single_shot(
                            std::chrono::milliseconds(50), [hwnd, width, height]() {
                                SetWindowPos(hwnd, nullptr, 0, 0, width, height,
                                             SWP_NOMOVE | SWP_NOZORDER);
                            });
                    }
                }
#else
                if (main_window_) {
                    (*main_window_)->show();
                    (*main_window_)->window().set_minimized(false);
                }
#endif
                if (main_window_) {
                    (*main_window_)->window().request_redraw();
                }
            },
        .on_start_pause =
            [this]() {
                spdlog::debug("Tray: Start/Pause");
                const auto current_state = GetCurrentStateSnapshot();
                if (current_state == State::kRunning || current_state == State::kSnoozed) {
                    OnPause();
                } else {
                    OnStart();
                }
            },
        .on_skip =
            [this]() {
                spdlog::debug("Tray: Skip");
                OnSkip();
            },
        .on_settings =
            [this]() {
                spdlog::debug("Tray: Settings");
                OnOpenSettings();
            },
        .on_quit =
            [this]() {
                spdlog::debug("Tray: Quit");
                OnQuit();
            }};

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

    // Start idle detector if enabled
    if (idle_detector_) {
        idle_detector_->Start();
    }

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

    // Stop idle detector
    if (idle_detector_) {
        idle_detector_->Stop();
    }

    if (timer_thread_.joinable()) {
        timer_thread_.join();
    }

    return 0;
}

void AppController::OnStart() {
    spdlog::debug("OnStart called");

    const auto current_state = GetCurrentStateSnapshot();
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

    if (GetCurrentStateSnapshot() == State::kBreakActive) {
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

    if (GetCurrentStateSnapshot() == State::kBreakActive) {
        {
            std::lock_guard scheduler_lock(scheduler_mutex_);
            scheduler_->SnoozeBreak();
        }
        if (overlay_manager_) {
            overlay_manager_->Hide();
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
    if (overlay_manager_) {
        overlay_manager_->Hide();
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
                slint::invoke_from_event_loop([this]() {
                    if (settings_dialog_) {
                        settings_dialog_.reset();
                    }
                });
            }
        });

        // Handle settings window close button (X)
        dialog->window().on_close_requested([this]() {
            spdlog::debug("Settings close requested - destroying");

            slint::invoke_from_event_loop([this]() {
                if (settings_dialog_) {
                    settings_dialog_.reset();
                }
            });

            return slint::CloseRequestResponse::HideWindow;
        });

        dialog->on_save_clicked([this] {
            if (!settings_dialog_) {
                return;
            }

            auto& dialog = *settings_dialog_;
            dialog->set_validation_error(slint::SharedString(""));

            const auto short_interval_text = ToStdString(dialog->get_short_interval_minutes());
            const auto short_duration_text = ToStdString(dialog->get_short_duration_seconds());
            const auto long_interval_text = ToStdString(dialog->get_long_interval_minutes());
            const auto long_duration_text = ToStdString(dialog->get_long_duration_seconds());
            const auto snooze_duration_text = ToStdString(dialog->get_snooze_duration_minutes());
            const auto idle_threshold_text = ToStdString(dialog->get_idle_threshold_minutes());
            const auto notification_warning_text =
                ToStdString(dialog->get_notification_warning_seconds());

            const auto short_interval_minutes = ParsePositiveInt(short_interval_text);
            const auto short_duration_seconds = ParsePositiveInt(short_duration_text);
            const auto long_interval_minutes = ParsePositiveInt(long_interval_text);
            const auto long_duration_seconds = ParsePositiveInt(long_duration_text);
            const auto snooze_duration_minutes = ParsePositiveInt(snooze_duration_text);
            const auto idle_threshold_minutes = ParsePositiveInt(idle_threshold_text);
            const auto notification_warning_seconds =
                ParsePositiveInt(notification_warning_text);

            if (!short_interval_minutes || !short_duration_seconds || !long_interval_minutes ||
                !long_duration_seconds || !snooze_duration_minutes || !idle_threshold_minutes ||
                !notification_warning_seconds) {
                dialog->set_validation_error(
                    slint::SharedString("All fields must be positive integers."));
                return;
            }
            if (*notification_warning_seconds < 5 || *notification_warning_seconds > 300) {
                dialog->set_validation_error(
                    slint::SharedString("Warning time must be between 5 and 300 seconds."));
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
            updated.overlay.snooze_duration = Duration(*snooze_duration_minutes * 60);
            updated.overlay.show_on_all_monitors = dialog->get_overlay_all_monitors();
            updated.overlay.opacity = dialog->get_overlay_opaque() ? 1.0f : 0.7f;
            updated.theme.follow_system = dialog->get_theme_follow_system();
            updated.theme.dark_mode = dialog->get_theme_dark_mode();

            // Update notification config
            updated.notification.enabled = dialog->get_notification_enabled();
            updated.notification.warning_time = Duration(*notification_warning_seconds);
            updated.notification.respect_dnd = dialog->get_notification_respect_dnd();

            // Update idle config
            updated.idle.enabled = dialog->get_idle_enabled();
            updated.idle.threshold = Duration(*idle_threshold_minutes * 60);
            updated.idle.pause_on_idle = dialog->get_idle_pause_on_idle();
            updated.idle.reset_on_idle = dialog->get_idle_reset_on_idle();
            updated.idle.show_timer = dialog->get_idle_show_timer();

            auto errors = config_manager_->Validate(updated);
            if (!errors.empty()) {
                dialog->set_validation_error(slint::SharedString(errors[0].message));
                return;
            }

            if (config_manager_) {
                auto save_result = config_manager_->Save(updated, config_path_);
                if (!save_result) {
                    dialog->set_validation_error(slint::SharedString(save_result.error().message));
                    return;
                }
            }

            {
                std::lock_guard lock(mutex_);
                config_ = updated;
            }
            if (!updated.notification.enabled && notification_manager_) {
                int64_t toast_id = -1;
                {
                    std::lock_guard lock(mutex_);
                    toast_id = active_toast_id_;
                    active_toast_id_ = -1;
                }
                if (toast_id >= 0) {
                    notification_manager_->Hide(toast_id);
                }
            }
            bool scheduler_running = false;
            {
                std::lock_guard scheduler_lock(scheduler_mutex_);
                if (scheduler_) {
                    scheduler_->UpdateConfig(updated.short_break, updated.long_break,
                                             updated.overlay);
                    const auto warning_time = updated.notification.enabled
                                                  ? updated.notification.warning_time
                                                  : Duration::zero();
                    scheduler_->SetOnWarning(
                        [this](BreakType type, Duration time_until) {
                            spdlog::info("Warning: {} break in {}s", BreakTypeToString(type),
                                         time_until.count());
                            if (tray_manager_) {
                                tray_manager_->UpdateStatus(is_running_, time_until, type);
                            }
                            slint::invoke_from_event_loop(

                                [this, type, time_until]() {
                                    ShowPreBreakNotification(type, time_until);
                                });
                        },
                        warning_time);
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
            if (overlay_manager_) {
                overlay_manager_->UpdateOpacity(updated.overlay.opacity);
                overlay_manager_->UpdateActions(updated.overlay.allow_skip,
                                                updated.overlay.allow_snooze);
                overlay_manager_->SetShowOnAllMonitors(updated.overlay.show_on_all_monitors);
            }

            ApplyThemeToMainWindow();
            ApplyThemeToSettingsDialog();

            // Update idle detector settings
            if (updated.idle.enabled) {
                if (!idle_detector_) {
                    // Create idle detector if it didn't exist
                    idle_detector_ = platform::CreateIdleDetector();
                    if (idle_detector_) {
                        idle_detector_->SetOnIdle([this]() { OnUserIdle(); });
                        idle_detector_->SetOnActive([this]() { OnUserActive(); });
                    }
                }
                if (idle_detector_) {
                    idle_detector_->SetIdleThreshold(
                        std::chrono::duration_cast<std::chrono::seconds>(updated.idle.threshold));
                    if (!idle_detector_->IsRunning()) {
                        idle_detector_->Start();
                    }
                    spdlog::info(
                        "Idle detection updated: threshold={}s, pause={}, reset={}, show_timer={}",
                        updated.idle.threshold.count(), updated.idle.pause_on_idle,
                        updated.idle.reset_on_idle, updated.idle.show_timer);
                }
            } else {
                // Disable idle detection
                if (idle_detector_ && idle_detector_->IsRunning()) {
                    idle_detector_->Stop();
                    spdlog::info("Idle detection disabled");
                }
            }

            UpdateUI();

            (*settings_dialog_)->hide();
            slint::invoke_from_event_loop([this]() {
                if (settings_dialog_) {
                    settings_dialog_.reset();
                }
            });
        });
    }

    auto& dialog = *settings_dialog_;
    AppConfig snapshot{};
    {
        std::lock_guard lock(mutex_);
        snapshot = config_;
    }
    dialog->set_short_interval_minutes(slint::SharedString(
        std::to_string(static_cast<int>(snapshot.short_break.interval.count() / 60))));
    dialog->set_short_duration_seconds(slint::SharedString(
        std::to_string(static_cast<int>(snapshot.short_break.duration.count()))));
    dialog->set_long_interval_minutes(slint::SharedString(
        std::to_string(static_cast<int>(snapshot.long_break.interval.count() / 60))));
    dialog->set_long_duration_seconds(slint::SharedString(
        std::to_string(static_cast<int>(snapshot.long_break.duration.count()))));
    dialog->set_snooze_duration_minutes(slint::SharedString(
        std::to_string(static_cast<int>(snapshot.overlay.snooze_duration.count() / 60))));
    dialog->set_overlay_all_monitors(snapshot.overlay.show_on_all_monitors);
    dialog->set_overlay_opaque(snapshot.overlay.opacity > 0.8f);
    dialog->set_theme_follow_system(snapshot.theme.follow_system);
    dialog->set_theme_dark_mode(snapshot.theme.dark_mode);

    // Bind notification settings
    dialog->set_notification_enabled(snapshot.notification.enabled);
    dialog->set_notification_warning_seconds(slint::SharedString(
        std::to_string(static_cast<int>(snapshot.notification.warning_time.count()))));
    dialog->set_notification_respect_dnd(snapshot.notification.respect_dnd);

    // Bind idle settings
    dialog->set_idle_enabled(snapshot.idle.enabled);
    dialog->set_idle_threshold_minutes(slint::SharedString(
        std::to_string(static_cast<int>(snapshot.idle.threshold.count() / 60))));
    dialog->set_idle_pause_on_idle(snapshot.idle.pause_on_idle);
    dialog->set_idle_reset_on_idle(snapshot.idle.reset_on_idle);
    dialog->set_idle_show_timer(snapshot.idle.show_timer);

    dialog->set_validation_error(slint::SharedString(""));
    ApplyThemeToSettingsDialog();

    dialog->show();
}

void AppController::OnQuit() {
    spdlog::info("Quit requested");

    running_.store(false);

    // Stop idle detector
    if (idle_detector_) {
        idle_detector_->Stop();
    }

    if (main_window_weak_) {
        auto ui_weak = *main_window_weak_;
        slint::invoke_from_event_loop([ui_weak]() mutable {
            if (auto ui = ui_weak.lock()) {
                (*ui)->hide();
            }
        });
    }

    if (tray_manager_) {
        tray_manager_->Hide();
    }
    if (overlay_manager_) {
        overlay_manager_->Hide();
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

std::string AppController::GetTrackedTimeString() const {
    std::lock_guard lock(mutex_);
    return tracked_time_;
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

int AppController::GetShortSkippedCount() const {
    std::lock_guard lock(mutex_);
    return short_skipped_count_;
}

int AppController::GetLongSkippedCount() const {
    std::lock_guard lock(mutex_);
    return long_skipped_count_;
}

std::string AppController::GetStatusText() const {
    std::lock_guard lock(mutex_);
    return status_text_;
}

State AppController::GetCurrentStateSnapshot() const {
    std::lock_guard lock(mutex_);
    return current_state_;
}

void AppController::TimerThreadFunc() {
    spdlog::debug("Timer thread started");

    auto last_time = std::chrono::steady_clock::now();
    constexpr auto kTickInterval = 100ms;

    while (running_.load()) {
        auto now = std::chrono::steady_clock::now();
        auto delta = std::chrono::duration_cast<DurationMs>(now - last_time);
        last_time = now;

        bool should_track = false;
        {
            std::lock_guard scheduler_lock(scheduler_mutex_);
            scheduler_->Update(delta);
            should_track = scheduler_->IsRunning();
        }
        {
            std::lock_guard lock(mutex_);
            if (should_track) {
                tracked_duration_ms_ += delta;
                tracked_duration_ = std::chrono::duration_cast<Duration>(tracked_duration_ms_);
                tracked_time_ = FormatDuration(tracked_duration_);
            }
        }
        UpdateUI();

        std::this_thread::sleep_for(kTickInterval);
    }

    spdlog::debug("Timer thread stopped");
}

void AppController::UpdateUI() {
    std::string time_until_short;
    std::string time_until_long;
    std::string tracked_time;
    std::string status_text;
    float short_progress = 0.0f;
    float long_progress = 0.0f;
    int short_break_count = 0;
    int long_break_count = 0;
    int short_skipped_count = 0;
    int long_skipped_count = 0;
    bool is_running = false;
    bool show_idle_timer = false;
    std::string idle_time = "00:00";
    bool overlay_active = false;
    std::string overlay_time_remaining;

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

        if (time_until_short_opt) {
            const auto short_total = scheduler_->GetShortIntervalTotal();
            if (short_total.count() > 0) {
                short_progress_ = 1.0f - (static_cast<float>(time_until_short_opt->count()) /
                                          static_cast<float>(short_total.count()));
            }
        }

        if (time_until_long_opt) {
            const auto long_total = scheduler_->GetLongIntervalTotal();
            if (long_total.count() > 0) {
                long_progress_ = 1.0f - (static_cast<float>(time_until_long_opt->count()) /
                                         static_cast<float>(long_total.count()));
            }
        }

        const bool break_active = scheduler_->IsBreakActive();

        if (break_active) {
            auto break_remaining = scheduler_->GetTimeUntilBreakEnds();
            if (break_remaining) {
                overlay_time_remaining = FormatDuration(*break_remaining);
                overlay_active = true;
            }
        }

        time_until_short = time_until_short_;
        time_until_long = time_until_long_;
        tracked_time = tracked_time_;
        status_text = status_text_;
        short_progress = short_progress_;
        long_progress = long_progress_;
        short_break_count = short_break_count_;
        long_break_count = long_break_count_;
        short_skipped_count = short_skipped_count_;
        long_skipped_count = long_skipped_count_;
        is_running = is_running_;

        if (config_.idle.show_timer) {
            std::chrono::milliseconds curr_idle{0};
            if (idle_detector_ && idle_detector_->IsRunning()) {
                curr_idle = idle_detector_->GetIdleTime();
            }
            auto idle_sec = std::chrono::duration_cast<std::chrono::seconds>(curr_idle);
            idle_time_ = FormatDuration(std::chrono::duration_cast<Duration>(idle_sec));
            show_idle_timer_ = true;
        } else {
            show_idle_timer_ = false;
        }

        show_idle_timer = show_idle_timer_;
        idle_time = idle_time_;

        // Update tray status
        if (tray_manager_ && time_until_next) {
            tray_manager_->UpdateStatus(is_running_, *time_until_next,
                                        scheduler_->GetNextBreakType());
        }
    }

    if (overlay_manager_) {
        if (overlay_active && !overlay_time_remaining.empty()) {
            overlay_manager_->UpdateTimeRemaining(overlay_time_remaining);
        } else if (overlay_manager_->IsVisible()) {
            overlay_manager_->Hide();
        }
    }

    if (!main_window_weak_) {
        return;
    }

    auto ui_weak = *main_window_weak_;
    slint::invoke_from_event_loop([ui_weak, time_until_short, time_until_long, tracked_time,
                                   status_text, short_progress, long_progress, short_break_count,
                                   long_break_count, short_skipped_count, long_skipped_count,
                                   is_running, show_idle_timer, idle_time]() mutable {
        if (auto ui_handle = ui_weak.lock()) {
            (*ui_handle)->set_time_until_short(slint::SharedString(time_until_short));
            (*ui_handle)->set_time_until_long(slint::SharedString(time_until_long));
            (*ui_handle)->set_tracked_time(slint::SharedString(tracked_time));
            (*ui_handle)->set_status_text(slint::SharedString(status_text));
            (*ui_handle)->set_short_progress(short_progress);
            (*ui_handle)->set_long_progress(long_progress);
            (*ui_handle)->set_short_break_count(short_break_count);
            (*ui_handle)->set_long_break_count(long_break_count);
            (*ui_handle)->set_short_skipped_count(short_skipped_count);
            (*ui_handle)->set_long_skipped_count(long_skipped_count);
            (*ui_handle)->set_is_running(is_running);
            (*ui_handle)->set_show_idle_timer(show_idle_timer);
            (*ui_handle)->set_idle_time(slint::SharedString(idle_time));
        }
    });
}

std::string AppController::FormatDuration(Duration duration) {
    auto total_seconds = duration.count();
    auto seconds = total_seconds % 60;
    auto minutes = (total_seconds % 3600) / 60;
    auto hours = (total_seconds % 86400) / 3600;
    auto days = total_seconds / 86400;

    if (total_seconds < 3600) {
        return std::format("{:02}:{:02}", minutes, seconds);
    } else if (total_seconds < 86400) {
        return std::format("{:02}:{:02}:{:02}", hours, minutes, seconds);
    } else {
        return std::format("{}:{:02}:{:02}:{:02}", days, hours, minutes, seconds);
    }
}

void AppController::OnUserIdle() {
    bool should_pause = false;
    bool should_reset = false;
    {
        std::lock_guard lock(mutex_);
        should_pause = config_.idle.pause_on_idle;
        should_reset = config_.idle.reset_on_idle;
    }

    spdlog::info("AppController: User became idle (config: pause_on_idle={}, reset_on_idle={})",
                 should_pause, should_reset);

    const auto current_state = GetCurrentStateSnapshot();

    if (should_reset && current_state == State::kRunning) {
        // Reset timers on idle if configured
        spdlog::info("Resetting timers due to idle");
        {
            std::lock_guard scheduler_lock(scheduler_mutex_);
            scheduler_->Reset();
        }
        state_machine_->ProcessEvent(UserIdleEvent{});
        {
            std::lock_guard lock(mutex_);
            status_text_ = "Idle - Timers reset";
            is_paused_by_idle_ = true;
            short_progress_ = 0.0f;
            long_progress_ = 0.0f;
        }
    } else if (should_pause && current_state == State::kRunning) {
        // Pause on idle
        spdlog::info("Pausing due to idle");
        auto result = state_machine_->ProcessEvent(UserIdleEvent{});
        if (result.success) {
            {
                std::lock_guard scheduler_lock(scheduler_mutex_);
                scheduler_->Pause();
            }
            {
                std::lock_guard lock(mutex_);
                status_text_ = "Idle - Paused";
                is_paused_by_idle_ = true;
            }
            if (tray_manager_) {
                tray_manager_->UpdateMenu(false);
            }
        }
    }
}

void AppController::OnUserActive() {
    bool was_paused_by_idle = false;
    {
        std::lock_guard lock(mutex_);
        was_paused_by_idle = is_paused_by_idle_;
    }

    spdlog::info("AppController: User became active (was_paused_by_idle={})", was_paused_by_idle);

    const auto current_state = GetCurrentStateSnapshot();

    // Only auto-resume if we paused due to idle
    if (was_paused_by_idle && current_state == State::kPaused) {
        spdlog::info("Resuming after idle");
        auto result = state_machine_->ProcessEvent(UserActiveEvent{});
        if (result.success) {
            {
                std::lock_guard scheduler_lock(scheduler_mutex_);
                scheduler_->Resume();
            }
            {
                std::lock_guard lock(mutex_);
                status_text_ = "Running";
                is_paused_by_idle_ = false;
            }
            if (tray_manager_) {
                tray_manager_->UpdateMenu(true);
            }
        }
    }
}

void AppController::OnNotificationAction(platform::NotificationAction action) {
    spdlog::info("Notification action: {}", static_cast<int>(action));

    if (action == platform::NotificationAction::Clicked) {
#ifdef _WIN32
        HWND hwnd = FindWindowByTitle(L"BlinkBreak");
        if (hwnd) {
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);

            RECT rect;
            if (GetWindowRect(hwnd, &rect)) {
                int width = rect.right - rect.left;
                int height = rect.bottom - rect.top;
                SetWindowPos(hwnd, nullptr, 0, 0, width + 1, height, SWP_NOMOVE | SWP_NOZORDER);
                slint::Timer::single_shot(std::chrono::milliseconds(50),
                                          [hwnd, width, height]() {
                                              SetWindowPos(hwnd, nullptr, 0, 0, width, height,
                                                           SWP_NOMOVE | SWP_NOZORDER);
                                          });
            }
        }
#else
        if (main_window_) {
            (*main_window_)->show();
            (*main_window_)->window().set_minimized(false);
        }
#endif
        if (main_window_) {
            (*main_window_)->window().request_redraw();
        }
    }

    bool handled_immediately = false;
    if (action == platform::NotificationAction::SkipBreak ||
        action == platform::NotificationAction::SnoozeBreak) {
        if (GetCurrentStateSnapshot() == State::kBreakActive) {
            if (action == platform::NotificationAction::SkipBreak) {
                OnSkip();
            } else {
                OnSnooze();
            }
            handled_immediately = true;
        }
    }

    {
        std::lock_guard lock(mutex_);
        if (action == platform::NotificationAction::SkipBreak ||
            action == platform::NotificationAction::SnoozeBreak) {
            if (handled_immediately) {
                pending_notification_action_.reset();
                pending_notification_break_.reset();
            } else {
                pending_notification_action_ = action;
            }
        }
        if (action == platform::NotificationAction::Dismissed ||
            action == platform::NotificationAction::Clicked) {
            pending_notification_action_.reset();
        }
        active_toast_id_ = -1;
    }
}

void AppController::ShowPreBreakNotification(BreakType type, Duration time_until) {
    NotificationConfig notification_config;
    {
        std::lock_guard lock(mutex_);
        notification_config = config_.notification;
        pending_notification_break_ = type;
        pending_notification_action_.reset();
    }

    if (!notification_config.enabled) {
        return;
    }
    if (!notification_manager_ || !notification_manager_->IsSupported()) {
        return;
    }

    if (notification_config.respect_dnd) {
        bool dnd_active = false;
#ifdef _WIN32
        dnd_active = IsDndActive();
#endif
        if (dnd_active) {
            spdlog::info("Notifications suppressed due to DND/Focus Assist");
            return;
        }
    }

    std::string message;
    {
        std::lock_guard scheduler_lock(scheduler_mutex_);
        if (scheduler_) {
            message = scheduler_->GetUpcomingMessage(type);
        }
    }
    if (message.empty()) {
        message = "Time to take a break";
    }

    const std::string title = std::format("{} break in {} seconds",
                                          BreakTypeToString(type), time_until.count());
    const auto toast_id = notification_manager_->Show(title, message);
    {
        std::lock_guard lock(mutex_);
        active_toast_id_ = toast_id;
    }
}

void AppController::ApplyThemeToMainWindow() {
    if (!main_window_) {
        return;
    }

    ThemeConfig theme;
    {
        std::lock_guard lock(mutex_);
        theme = config_.theme;
    }

    ApplyThemeProperties(*main_window_, theme);
}

void AppController::ApplyThemeToSettingsDialog() {
    if (!settings_dialog_) {
        return;
    }

    ThemeConfig theme;
    {
        std::lock_guard lock(mutex_);
        theme = config_.theme;
    }

    ApplyThemeProperties(*settings_dialog_, theme);
}

}  // namespace blinkbreak
