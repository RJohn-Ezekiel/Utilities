#pragma once

#include <QObject>
#include <QString>
#include <memory>

namespace arete::settings { class SettingsManager; }

namespace arete {

namespace core { class ModuleRegistry; }
namespace data { class DatabaseManager; class TaskRepository; class ProjectRepository; class EventRepository; class JournalRepository; class HabitRepository; class CheckinRepository; class StatsRepository; }
namespace services { class CommandRunner; class SearchService; class WeatherService; class VerseService; class TimerService; class MusicService; class ReminderService; }
namespace ui { class MainWindow; }

} // namespace arete

// Forward declarations of vendored engines (global namespaces).
namespace chronos { class StorageManager; class TimerService; class TaskService; class StatisticsService; class NotificationService; class AudioService; class ReminderScheduler; }
namespace phonio { class App; }

namespace arete {

namespace app {

// Owns every long-lived service and repository in the application.
// Modules receive this context and use it to talk to the rest of Arete,
// keeping modules decoupled from each other.
class AppContext : public QObject
{
    Q_OBJECT

public:
    explicit AppContext(QObject* parent = nullptr);
    ~AppContext() override;

    [[nodiscard]] QString dataDirectory() const;
    [[nodiscard]] QString databasePath() const;

    [[nodiscard]] arete::settings::SettingsManager* settings() const;
    [[nodiscard]] arete::data::DatabaseManager* database() const;
    [[nodiscard]] arete::core::ModuleRegistry* modules() const;

    [[nodiscard]] arete::data::TaskRepository* tasks() const;
    [[nodiscard]] arete::data::ProjectRepository* projects() const;
    [[nodiscard]] arete::data::EventRepository* events() const;
    [[nodiscard]] arete::data::JournalRepository* journal() const;
    [[nodiscard]] arete::data::HabitRepository* habits() const;
    [[nodiscard]] arete::data::CheckinRepository* checkins() const;
    [[nodiscard]] arete::data::StatsRepository* stats() const;

    [[nodiscard]] arete::services::CommandRunner* commands() const;
    [[nodiscard]] arete::services::SearchService* search() const;
    [[nodiscard]] arete::services::WeatherService* weather() const;
    [[nodiscard]] arete::services::VerseService* verse() const;
    [[nodiscard]] arete::services::TimerService* timer() const;
    [[nodiscard]] arete::services::MusicService* music() const;

    // Vendored Chronos engine (focus sessions, statistics, history).
    [[nodiscard]] chronos::StorageManager* chronosStorage() const;
    [[nodiscard]] chronos::TimerService* chronosTimer() const;
    [[nodiscard]] chronos::TaskService* chronosTasks() const;
    [[nodiscard]] chronos::StatisticsService* chronosStats() const;
    [[nodiscard]] chronos::AudioService* chronosAudio() const;
    [[nodiscard]] chronos::ReminderScheduler* chronosReminders() const;
    [[nodiscard]] chronos::NotificationService* chronosNotifications() const;

    // Vendored Phonio engine (music library, playback).
    [[nodiscard]] phonio::App* phonioApp() const;

    void setMainWindow(arete::ui::MainWindow* window);
    [[nodiscard]] arete::ui::MainWindow* mainWindow() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace app
} // namespace arete
