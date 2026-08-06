#include "app/AppContext.h"
#include "arete/settings/SettingsManager.h"
#include "core/ModuleRegistry.h"
#include "data/DatabaseManager.h"
#include "data/TaskRepository.h"
#include "data/ProjectRepository.h"
#include "data/EventRepository.h"
#include "data/JournalRepository.h"
#include "data/HabitRepository.h"
#include "data/CheckinRepository.h"
#include "data/StatsRepository.h"
#include "services/CommandRunner.h"
#include "services/SearchService.h"
#include "services/WeatherService.h"
#include "services/VerseService.h"
#include "services/TimerService.h"
#include "services/MusicService.h"
#include "services/ReminderService.h"
#include "ui/MainWindow.h"
#include "arete/logging/Logger.h"

#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QApplication>

#include "storage/StorageManager.h"
#include "vendor/chronos/services/TimerService.h"
#include "vendor/chronos/services/TaskService.h"
#include "vendor/chronos/services/StatisticsService.h"
#include "vendor/chronos/services/AudioService.h"
#include "vendor/chronos/services/ReminderScheduler.h"
#include "vendor/chronos/services/NotificationService.h"
#include "vendor/phonio/app/App.h"

using arete::logging::Logger;

namespace arete::app {

class AppContext::Private
{
public:
    QString dataDirectory;
    std::unique_ptr<arete::settings::SettingsManager> settings;
    std::unique_ptr<arete::data::DatabaseManager> database;
    std::unique_ptr<arete::data::TaskRepository> tasks;
    std::unique_ptr<arete::data::ProjectRepository> projects;
    std::unique_ptr<arete::data::EventRepository> events;
    std::unique_ptr<arete::data::JournalRepository> journal;
    std::unique_ptr<arete::data::HabitRepository> habits;
    std::unique_ptr<arete::data::CheckinRepository> checkins;
    std::unique_ptr<arete::data::StatsRepository> stats;
    std::unique_ptr<arete::core::ModuleRegistry> modules;
    std::unique_ptr<arete::services::CommandRunner> commands;
    std::unique_ptr<arete::services::SearchService> search;
    std::unique_ptr<arete::services::WeatherService> weather;
    std::unique_ptr<arete::services::VerseService> verse;
    std::unique_ptr<arete::services::TimerService> timer;
    std::unique_ptr<arete::services::MusicService> music;
    std::unique_ptr<arete::services::ReminderService> reminders;

    // Vendored Chronos engine wiring.
    std::unique_ptr<chronos::StorageManager> chronosStorage;
    std::unique_ptr<chronos::TimerService> chronosTimer;
    std::unique_ptr<chronos::TaskService> chronosTasks;
    std::unique_ptr<chronos::StatisticsService> chronosStats;
    std::unique_ptr<chronos::AudioService> chronosAudio;
    std::unique_ptr<chronos::ReminderScheduler> chronosReminders;
    std::unique_ptr<chronos::NotificationService> chronosNotifications;

    // Vendored Phonio engine.
    std::unique_ptr<phonio::App> phonio;

    arete::ui::MainWindow* mainWindow = nullptr;
};

AppContext::AppContext(QObject* parent)
    : QObject(parent), d(std::make_unique<Private>())
{
    d->dataDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (d->dataDirectory.isEmpty()) {
        d->dataDirectory = QDir::homePath() + QStringLiteral("/.local/share/Arete");
    }
    if (!QDir().mkpath(d->dataDirectory)) {
        Logger::instance().error(QStringLiteral("Cannot create data directory: %1").arg(d->dataDirectory),
                                 QStringLiteral("app"));
    }

    d->settings = std::make_unique<arete::settings::SettingsManager>(
        QStringLiteral("Arete"), QStringLiteral("Arete"),
        arete::settings::SettingsManager::Scope::User,
        arete::settings::SettingsManager::Format::Ini, this);

    d->database = std::make_unique<arete::data::DatabaseManager>(
        d->dataDirectory + QStringLiteral("/arete.db"), this);
    d->database->open();

    d->tasks = std::make_unique<arete::data::TaskRepository>(d->database.get(), this);
    d->projects = std::make_unique<arete::data::ProjectRepository>(d->database.get(), this);
    d->events = std::make_unique<arete::data::EventRepository>(d->database.get(), this);
    d->journal = std::make_unique<arete::data::JournalRepository>(d->database.get(), this);
    d->habits = std::make_unique<arete::data::HabitRepository>(d->database.get(), this);
    d->checkins = std::make_unique<arete::data::CheckinRepository>(d->database.get(), this);
    d->stats = std::make_unique<arete::data::StatsRepository>(d->database.get(), this);

    // ── Vendored Chronos engine ──
    d->chronosStorage = std::make_unique<chronos::StorageManager>(this);
    d->chronosTasks = std::make_unique<chronos::TaskService>(d->chronosStorage.get(), this);
    d->chronosTimer = std::make_unique<chronos::TimerService>(d->chronosStorage.get(), this);
    d->chronosStats = std::make_unique<chronos::StatisticsService>(d->chronosStorage.get(),
                                                                   d->chronosTasks.get(), this);
    d->chronosAudio = std::make_unique<chronos::AudioService>(this);
    d->chronosReminders = std::make_unique<chronos::ReminderScheduler>(this);
    d->chronosNotifications = std::make_unique<chronos::NotificationService>(d->chronosAudio.get(), this);
    d->chronosReminders->configure(d->chronosTimer->currentSettings());
    d->chronosTimer->attemptRestore();

    // ── Vendored Phonio engine ──
    d->phonio = std::make_unique<phonio::App>(static_cast<QApplication&>(*QCoreApplication::instance()),
                                              this);

    // ── Arete services ──
    d->modules = std::make_unique<arete::core::ModuleRegistry>(this, this);
    d->weather = std::make_unique<arete::services::WeatherService>(this, this);
    d->verse = std::make_unique<arete::services::VerseService>(this, this);
    d->timer = std::make_unique<arete::services::TimerService>(this, this);
    d->music = std::make_unique<arete::services::MusicService>(this, this);
    d->commands = std::make_unique<arete::services::CommandRunner>(this, this);
    d->search = std::make_unique<arete::services::SearchService>(this, this);
    d->reminders = std::make_unique<arete::services::ReminderService>(this, this);
    d->reminders->start();

    Logger::instance().info(QStringLiteral("AppContext ready (data: %1)").arg(d->dataDirectory),
                            QStringLiteral("app"));
}

AppContext::~AppContext() = default;

QString AppContext::dataDirectory() const { return d->dataDirectory; }

QString AppContext::databasePath() const
{
    return d->database->path();
}

arete::settings::SettingsManager* AppContext::settings() const { return d->settings.get(); }
arete::data::DatabaseManager* AppContext::database() const { return d->database.get(); }
arete::core::ModuleRegistry* AppContext::modules() const { return d->modules.get(); }
arete::data::TaskRepository* AppContext::tasks() const { return d->tasks.get(); }
arete::data::ProjectRepository* AppContext::projects() const { return d->projects.get(); }
arete::data::EventRepository* AppContext::events() const { return d->events.get(); }
arete::data::JournalRepository* AppContext::journal() const { return d->journal.get(); }
arete::data::HabitRepository* AppContext::habits() const { return d->habits.get(); }
arete::data::CheckinRepository* AppContext::checkins() const { return d->checkins.get(); }
arete::data::StatsRepository* AppContext::stats() const { return d->stats.get(); }
arete::services::CommandRunner* AppContext::commands() const { return d->commands.get(); }
arete::services::SearchService* AppContext::search() const { return d->search.get(); }
arete::services::WeatherService* AppContext::weather() const { return d->weather.get(); }
arete::services::VerseService* AppContext::verse() const { return d->verse.get(); }
arete::services::TimerService* AppContext::timer() const { return d->timer.get(); }
arete::services::MusicService* AppContext::music() const { return d->music.get(); }

chronos::StorageManager* AppContext::chronosStorage() const { return d->chronosStorage.get(); }
chronos::TimerService* AppContext::chronosTimer() const { return d->chronosTimer.get(); }
chronos::TaskService* AppContext::chronosTasks() const { return d->chronosTasks.get(); }
chronos::StatisticsService* AppContext::chronosStats() const { return d->chronosStats.get(); }
chronos::AudioService* AppContext::chronosAudio() const { return d->chronosAudio.get(); }
chronos::ReminderScheduler* AppContext::chronosReminders() const { return d->chronosReminders.get(); }
chronos::NotificationService* AppContext::chronosNotifications() const { return d->chronosNotifications.get(); }

phonio::App* AppContext::phonioApp() const { return d->phonio.get(); }

void AppContext::setMainWindow(arete::ui::MainWindow* window) { d->mainWindow = window; }
arete::ui::MainWindow* AppContext::mainWindow() const { return d->mainWindow; }

} // namespace arete::app
