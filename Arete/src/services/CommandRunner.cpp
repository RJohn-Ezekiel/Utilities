#include "services/CommandRunner.h"
#include "app/AppContext.h"
#include "arete/settings/SettingsManager.h"
#include "arete/widgets/CommandPalette.h"
#include "core/ModuleRegistry.h"
#include "services/TimerService.h"
#include "services/MusicService.h"
#include "services/SearchService.h"
#include "data/TaskRepository.h"
#include "data/ProjectRepository.h"
#include "models/Task.h"
#include "ui/MainWindow.h"
#include "ui/dialogs/TaskCompleteDialog.h"
#include "arete/logging/Logger.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using arete::logging::Logger;

namespace arete::services {

CommandRunner::CommandRunner(app::AppContext* context, QObject* parent)
    : QObject(parent), m_context(context)
{
    const QString historyJson = m_context->settings()
        ->get(QStringLiteral("palette/history"), QString());
    const QJsonArray arr = QJsonDocument::fromJson(historyJson.toUtf8()).array();
    for (const QJsonValue& v : arr) {
        m_history.append(v.toString());
    }
}

QVector<arete::widgets::Command> CommandRunner::paletteCommands()
{
    QVector<arete::widgets::Command> commands;
    const QString group = QStringLiteral("Timer");
    commands.append({QStringLiteral("start-work"), QStringLiteral("Start focus session"),
                     QStringLiteral(""), group, [this] { run(QStringLiteral("start work")); }});
    commands.append({QStringLiteral("start-short-break"), QStringLiteral("Start short break"),
                     QStringLiteral(""), group, [this] { run(QStringLiteral("start short_break")); }});
    commands.append({QStringLiteral("start-long-break"), QStringLiteral("Start long break"),
                     QStringLiteral(""), group, [this] { run(QStringLiteral("start long_break")); }});
    commands.append({QStringLiteral("pause-timer"), QStringLiteral("Pause timer"),
                     QStringLiteral(""), group, [this] { m_context->timer()->pause(); }});
    commands.append({QStringLiteral("resume-timer"), QStringLiteral("Resume timer"),
                     QStringLiteral(""), group, [this] { m_context->timer()->resume(); }});
    commands.append({QStringLiteral("stop-timer"), QStringLiteral("Stop timer"),
                     QStringLiteral(""), group, [this] { m_context->timer()->stop(); }});

    const QString music = QStringLiteral("Music");
    commands.append({QStringLiteral("music-play"), QStringLiteral("Play music"),
                     QStringLiteral("play \"song\""), music,
                     [this] { m_context->music()->playPause(); }});
    commands.append({QStringLiteral("music-pause"), QStringLiteral("Pause music"),
                     QStringLiteral(""), music, [this] { m_context->music()->pause(); }});
    commands.append({QStringLiteral("music-next"), QStringLiteral("Next track"),
                     QStringLiteral(""), music, [this] { m_context->music()->next(); }});
    commands.append({QStringLiteral("music-prev"), QStringLiteral("Previous track"),
                     QStringLiteral(""), music, [this] { m_context->music()->previous(); }});

    commands.append({QStringLiteral("new-task"), QStringLiteral("New task"),
                     QStringLiteral("Ctrl+T"), QStringLiteral("Capture"),
                     [this] { run(QStringLiteral("task new")); }});
    commands.append({QStringLiteral("journal"), QStringLiteral("Open journal"),
                     QStringLiteral("Ctrl+J"), QStringLiteral("Capture"),
                     [this] { run(QStringLiteral("journal")); }});
    commands.append({QStringLiteral("quick-note"), QStringLiteral("Quick note"),
                     QStringLiteral(""), QStringLiteral("Capture"),
                     [this] { run(QStringLiteral("note \"\"")); }});

    const QString go = QStringLiteral("Navigate");
    const auto moduleIds = m_context->modules()->ids();
    for (const QString& id : moduleIds) {
        const auto info = m_context->modules()->info(id);
        commands.append({QStringLiteral("go-") + id, QStringLiteral("Go to ") + info.title,
                         QStringLiteral(""), go,
                         [this, id] { m_context->mainWindow()->activateModule(id); }});
    }
    return commands;
}

bool CommandRunner::run(const QString& commandLine, QString* error)
{
    if (commandLine.trimmed().isEmpty()) return false;
    addToHistory(commandLine);

    QString err;
    const bool ok = runQuoted(commandLine, &err);
    if (!ok && error) *error = err;
    return ok;
}

bool CommandRunner::runQuoted(const QString& raw, QString* error)
{
    const QString line = raw.trimmed();
    const QString lower = line.toLower();
    auto* window = m_context->mainWindow();

    // ── Timer ──
    if (lower == QLatin1String("start work") || lower == QLatin1String("start focus")) {
        m_context->timer()->startFocus();
        return true;
    }
    if (lower == QLatin1String("start short_break") || lower == QLatin1String("short break")) {
        m_context->timer()->startShortBreak();
        return true;
    }
    if (lower == QLatin1String("start long_break") || lower == QLatin1String("long break")) {
        m_context->timer()->startLongBreak();
        return true;
    }
    if (lower == QLatin1String("pause") || lower == QLatin1String("timer pause")) {
        if (m_context->timer()->isRunning()) {
            m_context->timer()->pause();
        } else {
            m_context->music()->pause();
        }
        return true;
    }
    if (lower == QLatin1String("resume") || lower == QLatin1String("timer resume")) {
        m_context->timer()->resume();
        return true;
    }
    if (lower == QLatin1String("stop") || lower == QLatin1String("timer stop")) {
        m_context->timer()->stop();
        return true;
    }

    // ── Music ──
    if (lower == QLatin1String("pause music")) {
        m_context->music()->pause();
        return true;
    }
    if (lower == QLatin1String("next") || lower == QLatin1String("next track")) {
        m_context->music()->next();
        return true;
    }
    if (lower == QLatin1String("previous") || lower == QLatin1String("prev")) {
        m_context->music()->previous();
        return true;
    }
    if (lower == QLatin1String("play")) {
        return m_context->music()->playPause(), true;
    }

    // play "song title" / play song title
    if (lower.startsWith(QLatin1String("play ")) || lower.startsWith(QLatin1String("play\""))) {
        QString term = line.section(QLatin1Char(' '), 1).trimmed();
        term = term.remove(QStringLiteral("\""));
        if (m_context->music()->play(term)) return true;
        if (error) *error = QStringLiteral("No track found for \"%1\"").arg(term);
        return false;
    }

    // volume 60
    if (lower.startsWith(QLatin1String("volume "))) {
        bool ok = false;
        const int percent = line.section(QLatin1Char(' '), 1).trimmed().toInt(&ok);
        if (!ok) return false;
        m_context->music()->setVolume(qBound(0, percent, 100));
        return true;
    }

    // ── Tasks ──
    if (lower == QLatin1String("task new") || lower == QLatin1String("new task")) {
        window->openQuickCapture(QLatin1String("task"));
        return true;
    }
    if (lower == QLatin1String("task done") || lower == QLatin1String("complete task")) {
        const qint64 taskId = m_context->timer()->attachedTaskId();
        if (taskId > 0) {
            completeTaskAndOfferNext(taskId, m_context->timer()->attachedTaskTitle());
        } else if (error) {
            *error = QStringLiteral("No running task to complete");
        }
        return taskId > 0;
    }

    // focus <task term>
    if (lower.startsWith(QLatin1String("focus "))) {
        const QString term = line.section(QLatin1Char(' '), 1).trimmed();
        const auto tasks = m_context->tasks()->active();
        for (const models::Task& t : tasks) {
            if (t.title.contains(term, Qt::CaseInsensitive)) {
                m_context->timer()->startFocus(t.id, t.title);
                window->activateModule(QStringLiteral("chronos"));
                return true;
            }
        }
        if (error) *error = QStringLiteral("No active task matching \"%1\"").arg(term);
        return false;
    }

    // ── Capture ──
    if (lower == QLatin1String("journal")) {
        window->activateModule(QStringLiteral("journal"));
        return true;
    }
    if (lower == QLatin1String("calendar today")) {
        window->activateModule(QStringLiteral("calendar"));
        window->sendCommandToModule(QStringLiteral("calendar"), QStringLiteral("today"));
        return true;
    }

    // note "text" / note text
    if (lower.startsWith(QLatin1String("note ")) || lower.startsWith(QLatin1String("note\""))) {
        QString text = line.section(QLatin1Char(' '), 1).trimmed().remove(QStringLiteral("\""));
        if (!text.isEmpty()) {
            window->captureNote(text);
            return true;
        }
        window->openQuickCapture(QLatin1String("note"));
        return true;
    }

    // ── Navigation ──
    if (lower.startsWith(QLatin1String("open logos "))) {
        const QString ref = line.section(QLatin1Char(' '), 2).trimmed();
        window->activateModule(QStringLiteral("logos"));
        window->sendCommandToModule(QStringLiteral("logos"), ref);
        return true;
    }
    if (lower == QLatin1String("open logos")) {
        window->activateModule(QStringLiteral("logos"));
        return true;
    }
    if (lower.startsWith(QLatin1String("open ")) || lower.startsWith(QLatin1String("go "))) {
        const QString target = line.section(QLatin1Char(' '), 1).trimmed().toLower();
        for (const QString& id : m_context->modules()->ids()) {
            const auto info = m_context->modules()->info(id);
            if (id == target || info.title.toLower() == target) {
                window->activateModule(id);
                return true;
            }
        }
        if (error) *error = QStringLiteral("Unknown module \"%1\"").arg(target);
        return false;
    }

    // search <term> → universal search
    if (lower.startsWith(QLatin1String("search "))) {
        const QString term = line.section(QLatin1Char(' '), 1).trimmed();
        window->openUniversalSearch(term);
        return true;
    }

    if (error) *error = QStringLiteral("Unknown command: %1").arg(line);
    return false;
}

void CommandRunner::completeTaskAndOfferNext(qint64 taskId, const QString& taskTitle)
{
    m_context->tasks()->setStatus(taskId, models::TaskStatus::Completed, true,
                                  QDateTime::currentDateTime());
    arete::ui::TaskCompleteDialog dialog(m_context, m_context->mainWindow());
    dialog.setCompletedTask(taskTitle);
    if (dialog.exec() == QDialog::Accepted) {
        const auto action = dialog.chosenAction();
        switch (action) {
        case arete::ui::TaskCompleteDialog::Action::StartNext:
            m_context->mainWindow()->activateModule(QStringLiteral("today"));
            break;
        case arete::ui::TaskCompleteDialog::Action::ShortBreak:
            m_context->timer()->startShortBreak();
            break;
        case arete::ui::TaskCompleteDialog::Action::LongBreak:
            m_context->timer()->startLongBreak();
            break;
        case arete::ui::TaskCompleteDialog::Action::Journal:
            m_context->mainWindow()->activateModule(QStringLiteral("journal"));
            break;
        case arete::ui::TaskCompleteDialog::Action::Dismiss:
            break;
        }
    }
}

QStringList CommandRunner::history() const
{
    return m_history;
}

void CommandRunner::addToHistory(const QString& command)
{
    m_history.removeAll(command);
    m_history.prepend(command);
    while (m_history.size() > 50) m_history.removeLast();

    QJsonArray arr;
    for (const QString& h : m_history) arr.append(h);
    m_context->settings()->set(QStringLiteral("palette/history"),
                               QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
    m_context->settings()->sync();
}

} // namespace arete::services
