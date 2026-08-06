#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QVector>
#include <memory>
#include <vector>

class Bible;
class SearchService;

namespace arete {
namespace app { class AppContext; }
namespace services {

// A formation-oriented daily passage. `reference` is the display string
// ("Ecclesiastes 3:1-8"); `book/chapter/verseStart/verseEnd` carry the
// structured range so the Logos module can open the full passage.
struct DailyVerse
{
    bool valid = false;
    QString reference;
    QString text;
    QString translation;
    QString book;
    int chapter = 0;
    int verseStart = 0;
    int verseEnd = 0;
};

// Loads the Logos Bible data lazily and serves the daily scripture to the
// top bar and Home dashboard.
//
// Daily Scripture philosophy: the passage is chosen for *formation*, not
// random inspiration — only a curated set of books is considered, verses
// are scored against discipline themes (diligence, wisdom, stewardship,
// charity, humility, patience, perseverance, obedience, honest work,
// self-control, faithfulness, justice, prayer, service), and the result is
// a complete thought of consecutive verses with the reference beneath.
class VerseService : public QObject
{
    Q_OBJECT

public:
    explicit VerseService(app::AppContext* context, QObject* parent = nullptr);
    ~VerseService() override;

    // Deterministic "daily scripture" based on the calendar date.
    [[nodiscard]] DailyVerse dailyVerse() const;

    // Searches the active translation. Returns up to `limit` hits.
    [[nodiscard]] QStringList search(const QString& query, int limit = 10) const;

    [[nodiscard]] bool isLoaded() const;
    [[nodiscard]] QString biblesDirectory() const;

public slots:
    void ensureLoaded();

signals:
    void loaded(bool success);

private:
    // One scored verse in a preferred book.
    struct Candidate
    {
        QString book;
        int chapter = 0;
        int verse = 0;
        int score = 0;
    };

    void buildPool() const;
    [[nodiscard]] DailyVerse passageFrom(const Candidate& candidate) const;

    app::AppContext* m_context;
    std::unique_ptr<SearchService> m_search;
    std::vector<std::unique_ptr<Bible>> m_translations;
    const Bible* m_active = nullptr;
    bool m_loaded = false;

    mutable bool m_poolBuilt = false;
    mutable QHash<QString, QVector<Candidate>> m_pool; // book name → candidates
};

} // namespace services
} // namespace arete
