#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>

namespace arete {
namespace app { class AppContext; }

namespace services {

struct SearchHit
{
    QString group;    // "Tasks", "Projects", "Journal", "Notes", "Bible", "Songs", "Calendar"
    QString title;
    QString detail;
    std::function<void()> action;
};

// Universal search (Ctrl+K). Aggregates results from the SQLite stores,
// the Codex vault index and the Phonio library, grouped by category.
class SearchService : public QObject
{
    Q_OBJECT

public:
    explicit SearchService(app::AppContext* context, QObject* parent = nullptr);

    [[nodiscard]] QVector<SearchHit> search(const QString& query, int maxPerGroup = 8) const;

private:
    app::AppContext* m_context;
};

} // namespace services
} // namespace arete
