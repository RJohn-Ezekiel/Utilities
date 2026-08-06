#include "services/SearchService.h"
#include "app/AppContext.h"
#include "data/TaskRepository.h"
#include "data/ProjectRepository.h"
#include "data/EventRepository.h"
#include "data/JournalRepository.h"
#include "services/VerseService.h"
#include "services/MusicService.h"
#include "core/ModuleRegistry.h"
#include "ui/MainWindow.h"
#include "app/App.h"
#include "library/LibraryManager.h"
#include "models/Task.h"
#include "models/Project.h"
#include "models/CalendarEvent.h"
#include "models/JournalEntry.h"

namespace arete::services {

namespace phonio = ::phonio;

SearchService::SearchService(app::AppContext* context, QObject* parent)
    : QObject(parent), m_context(context)
{
}

QVector<SearchHit> SearchService::search(const QString& query, int maxPerGroup) const
{
    QVector<SearchHit> hits;
    const QString term = query.trimmed();
    auto* window = m_context->mainWindow();

    if (!term.isEmpty()) {
        // ── Tasks ──
        const auto tasks = m_context->tasks()->all();
        for (const models::Task& t : tasks) {
            if (t.title.contains(term, Qt::CaseInsensitive)) {
                hits.append({
                    QStringLiteral("Tasks"),
                    t.title,
                    t.status == models::TaskStatus::Completed ? QStringLiteral("done")
                                                              : QStringLiteral("active"),
                    [window, id = t.id] {
                        window->activateModule(QStringLiteral("today"));
                        window->highlightTask(id);
                    },
                });
                if (hits.size() >= maxPerGroup) break;
            }
        }

        // ── Projects ──
        for (const models::Project& p : m_context->projects()->all()) {
            if (p.name.contains(term, Qt::CaseInsensitive)) {
                hits.append({
                    QStringLiteral("Projects"),
                    p.name,
                    p.description,
                    [window, id = p.id] {
                        window->activateModule(QStringLiteral("projects"));
                        window->highlightProject(id);
                    },
                });
                break;
            }
        }

        // ── Journal ──
        for (const models::JournalEntry& e : m_context->journal()->search(term)) {
            const QString snippet = e.reflection.left(80).replace(QLatin1Char('\n'), QLatin1Char(' '));
            hits.append({
                QStringLiteral("Journal"),
                e.date.toString(Qt::ISODate),
                snippet,
                [window, date = e.date] {
                    window->activateModule(QStringLiteral("journal"));
                    window->openJournalDate(date);
                },
            });
            if (hits.size() >= maxPerGroup) break;
        }

        // ── Calendar ──
        const auto events = m_context->events()->all();
        for (const models::CalendarEvent& e : events) {
            if (e.title.contains(term, Qt::CaseInsensitive)) {
                hits.append({
                    QStringLiteral("Calendar"),
                    e.title,
                    e.start.toString(Qt::ISODate),
                    [window] { window->activateModule(QStringLiteral("calendar")); },
                });
                if (hits.size() >= maxPerGroup) break;
            }
        }

        // ── Songs ──
        for (const phonio::Track& t : m_context->phonioApp()->library()->tracks()) {
            if (t.title.contains(term, Qt::CaseInsensitive)
                || t.artist.contains(term, Qt::CaseInsensitive)) {
                hits.append({
                    QStringLiteral("Songs"),
                    t.title,
                    t.artist + (t.album.isEmpty() ? QString() : QStringLiteral(" — ") + t.album),
                    [this, t] { m_context->music()->play(t.title); },
                });
                if (hits.size() >= maxPerGroup) break;
            }
        }

        // ── Bible ──
        const QStringList verses = m_context->verse()->search(term, maxPerGroup);
        for (const QString& v : verses) {
            hits.append({
                QStringLiteral("Bible"),
                v,
                QString(),
                [window] { window->activateModule(QStringLiteral("logos")); },
            });
        }
    }

    // ── Modules (always available) ──
    for (const QString& id : m_context->modules()->ids()) {
        const auto info = m_context->modules()->info(id);
        if (term.isEmpty() || info.title.contains(term, Qt::CaseInsensitive)) {
            hits.append({
                QStringLiteral("Modules"),
                info.icon + QStringLiteral("  ") + info.title,
                QStringLiteral("tab"),
                [window, id] { window->activateModule(id); },
            });
        }
    }

    return hits;
}

} // namespace arete::services
