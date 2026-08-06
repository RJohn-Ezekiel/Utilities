#include "services/VerseService.h"
#include "app/AppContext.h"
#include "arete/settings/SettingsManager.h"
#include "arete/logging/Logger.h"
#include "core/Bible.h"
#include "core/Reference.h"
#include "data/Loader.h"
#include "vendor/logos/services/SearchService.h"

#include <QDir>
#include <QFileInfo>
#include <QDate>
#include <QRegularExpression>
#include <algorithm>
#include <numeric>

using arete::logging::Logger;

namespace arete::services {

namespace {

// ── Curated canon ────────────────────────────────────────────────────────
// Daily Scripture is for formation, not random inspiration: only these
// books feed the daily passage. Names are the canonical ones produced by
// the Logos BookNameMatcher.
const QStringList kPreferredBooks = {
    QStringLiteral("Ecclesiastes"),
    QStringLiteral("Matthew"), QStringLiteral("Mark"), QStringLiteral("Luke"), QStringLiteral("John"),
    QStringLiteral("Acts"),
    QStringLiteral("Romans"),
    QStringLiteral("1 Corinthians"), QStringLiteral("2 Corinthians"),
    QStringLiteral("Galatians"), QStringLiteral("Ephesians"), QStringLiteral("Philippians"),
    QStringLiteral("Colossians"),
    QStringLiteral("1 Thessalonians"), QStringLiteral("2 Thessalonians"),
    QStringLiteral("1 Timothy"), QStringLiteral("2 Timothy"),
    QStringLiteral("Titus"), QStringLiteral("Philemon"),
    QStringLiteral("Hebrews"), QStringLiteral("James"),
    QStringLiteral("1 Peter"), QStringLiteral("2 Peter"),
    QStringLiteral("1 John"), QStringLiteral("2 John"), QStringLiteral("3 John"),
    QStringLiteral("Jude"),
    QStringLiteral("Deuteronomy"), QStringLiteral("Leviticus"),
    QStringLiteral("Wisdom"), QStringLiteral("Sirach"),
};

// ── Formation themes ──
// Word stems that mark the virtues Arete wants to cultivate: diligence,
// wisdom, discipline, stewardship, charity, humility, patience,
// perseverance, obedience, honest work, self-control, faithfulness,
// justice, prayer, service. Matched as word-boundary prefixes.
const QStringList kThemeStems = {
    // diligence
    QStringLiteral("dilig"), QStringLiteral("industri"), QStringLiteral("sloth"),
    QStringLiteral("sluggard"),
    // wisdom
    QStringLiteral("wisd"), QStringLiteral("prudent"), QStringLiteral("discret"),
    // discipline / instruction
    QStringLiteral("disciplin"), QStringLiteral("chasten"), QStringLiteral("correction"),
    QStringLiteral("instruction"), QStringLiteral("rebuke"),
    // stewardship
    QStringLiteral("steward"), QStringLiteral("treasure"), QStringLiteral("inherit"),
    QStringLiteral("husbandman"), QStringLiteral("vineyard"), QStringLiteral("talents"),
    QStringLiteral("store up"), QStringLiteral("multiply"),
    // charity
    QStringLiteral("charity"), QStringLiteral("mercy"), QStringLiteral("alms"),
    QStringLiteral("bountif"), QStringLiteral("liberal"), QStringLiteral("lend"),
    QStringLiteral("share with"),
    // humility
    QStringLiteral("humble"), QStringLiteral("humility"), QStringLiteral("lowliness"),
    QStringLiteral("lowly"), QStringLiteral("meek"),
    // patience
    QStringLiteral("patient"), QStringLiteral("patience"), QStringLiteral("longsuffering"),
    // perseverance
    QStringLiteral("persever"), QStringLiteral("endure"), QStringLiteral("steadfast"),
    QStringLiteral("continue"), QStringLiteral("overcome"), QStringLiteral("hold fast"),
    QStringLiteral("press toward"), QStringLiteral("faint not"), QStringLiteral("run the race"),
    // obedience
    QStringLiteral("obedien"), QStringLiteral("obey"), QStringLiteral("hearken"),
    QStringLiteral("commandment"), QStringLiteral("statute"), QStringLiteral("ordinance"),
    // honest work
    QStringLiteral("labour"), QStringLiteral("labor"), QStringLiteral("toil"),
    QStringLiteral("sow"), QStringLiteral("reap"), QStringLiteral("harvest"),
    QStringLiteral("plant"), QStringLiteral("plough"), QStringLiteral("handiwork"),
    QStringLiteral("till the ground"),
    // self-control
    QStringLiteral("temperance"), QStringLiteral("sober"), QStringLiteral("vigilant"),
    QStringLiteral("bridle"), QStringLiteral("slow to anger"), QStringLiteral("abstain"),
    QStringLiteral("deny"), QStringLiteral("rule his own spirit"),
    // faithfulness
    QStringLiteral("faithful"), QStringLiteral("loyal"), QStringLiteral("keep the faith"),
    // justice
    QStringLiteral("justice"), QStringLiteral("righteous"), QStringLiteral("upright"),
    QStringLiteral("equity"), QStringLiteral("oppress"),
    // prayer
    QStringLiteral("prayer"), QStringLiteral("supplication"), QStringLiteral("intercession"),
    QStringLiteral("petition"), QStringLiteral("call upon"),
    // service
    QStringLiteral("servant"), QStringLiteral("ministry"), QStringLiteral("wait upon"),
    QStringLiteral("good works"), QStringLiteral("do good"), QStringLiteral("minister"),
    // formation generally
    QStringLiteral("meditate"), QStringLiteral("study"), QStringLiteral("learn"),
    QStringLiteral("admonish"), QStringLiteral("exhort"), QStringLiteral("edify"),
    QStringLiteral("redeem the time"), QStringLiteral("walk worthy"), QStringLiteral("blameless"),
    QStringLiteral("sanctif"), QStringLiteral("exercise"),
};

// How many consecutive verses may form a passage at most.
const int kMaxPassageVerses = 6;
// Complete thoughts need at least this many verses when the chapter allows.
const int kMinPassageVerses = 2;
// A verse needs at least this many words to anchor a passage.
const int kMinVerseWords = 6;

bool isTerminalPunctuation(QChar ch)
{
    return ch == QLatin1Char('.') || ch == QLatin1Char('!') || ch == QLatin1Char('?');
}

QString cleanVerseText(const std::string& raw)
{
    QString text = QString::fromStdString(raw);
    text.remove(QChar(0x00B6)); // pilcrow
    text = text.trimmed();
    return text.simplified();
}

} // namespace

VerseService::VerseService(app::AppContext* context, QObject* parent)
    : QObject(parent), m_context(context)
{
    m_search = std::make_unique<::SearchService>();
}

VerseService::~VerseService() = default;

QString VerseService::biblesDirectory() const
{
    QString dir = m_context->settings()->get(QStringLiteral("logos/biblesDir"), QString());
    if (!dir.isEmpty() && QFileInfo::exists(dir)) return dir;

    // Default to the sibling Logos checkout if present.
    const QString candidate = QStringLiteral(ARETE_LOGOS_BIBLES_DIR);
    if (QFileInfo::exists(candidate)) {
        m_context->settings()->set(QStringLiteral("logos/biblesDir"), candidate);
        m_context->settings()->sync();
        return candidate;
    }
    return dir;
}

void VerseService::ensureLoaded()
{
    if (m_loaded) return;

    const QString dir = biblesDirectory();
    if (!QFileInfo::exists(dir)) {
        Logger::instance().warning(QStringLiteral("Bibles directory not found: %1").arg(dir),
                                   QStringLiteral("logos"));
        m_loaded = true; // don't retry every second
        emit loaded(false);
        return;
    }

    Loader::Result result = Loader::loadAll(dir.toStdString());
    m_translations = std::move(result.translations);
    for (const auto& resultError : result.errors) {
        Logger::instance().warning(QString::fromStdString(resultError), QStringLiteral("logos"));
    }

    // Prefer KJV, otherwise the first translation.
    for (const auto& bible : m_translations) {
        if (bible->name().find("King James") != std::string::npos
            || bible->shortName().find("KJV") != std::string::npos) {
            m_active = bible.get();
            break;
        }
    }
    if (!m_active && !m_translations.empty()) {
        m_active = m_translations.front().get();
    }
    if (m_active) {
        m_search->buildIndex(*m_active);
    }

    m_loaded = true;
    Logger::instance().info(QStringLiteral("Loaded %1 bible translation(s)").arg(m_translations.size()),
                            QStringLiteral("logos"));
    emit loaded(m_active != nullptr);
}

void VerseService::buildPool() const
{
    if (m_poolBuilt || !m_active) return;
    m_poolBuilt = true;

    // Compile one regex per stem once.
    struct StemPattern
    {
        QRegularExpression regex;
    };
    QVector<StemPattern> patterns;
    patterns.reserve(kThemeStems.size());
    for (const QString& stem : kThemeStems) {
        const QString escaped = QRegularExpression::escape(stem);
        const bool phrase = stem.contains(QLatin1Char(' '));
        const QString pattern = phrase
            ? QStringLiteral("\\b%1\\b").arg(escaped)
            : QStringLiteral("\\b%1\\w*").arg(escaped);
        patterns.append({QRegularExpression(pattern,
                                            QRegularExpression::CaseInsensitiveOption)});
    }

    struct Span
    {
        int start;
        int length;
    };

    const auto scoreVerse = [&patterns](const QString& text) {
        int score = 0;
        QVector<Span> spans;
        for (const StemPattern& pattern : patterns) {
            const QRegularExpressionMatch match = pattern.regex.match(text);
            if (!match.hasMatch()) continue;
            const int start = match.capturedStart();
            const int length = match.capturedLength();
            bool covered = false;
            for (const Span& span : spans) {
                if (start >= span.start && start < span.start + span.length) {
                    covered = true;
                    break;
                }
            }
            if (!covered) {
                spans.append({start, length});
                ++score;
            }
        }
        return score;
    };

    // Score every verse in the preferred books; keep score > 0, word count
    // over the threshold. Fall back to all books if the canon is empty in
    // this translation.
    auto collect = [this, &scoreVerse](const QStringList& books) {
        for (const QString& bookName : books) {
            const Book* book = m_active->findBook(bookName.toStdString());
            if (!book) continue;
            for (const Chapter& chapter : book->chapters) {
                for (const Verse& verse : chapter.verses) {
                    const QString text = cleanVerseText(verse.text);
                    if (text.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts).size()
                        < kMinVerseWords) {
                        continue;
                    }
                    const int score = scoreVerse(text.toLower());
                    if (score > 0) {
                        m_pool[bookName].append({bookName, chapter.number, verse.number, score});
                    }
                }
            }
        }
    };

    collect(kPreferredBooks);
    bool canonEmpty = true;
    for (const QString& book : kPreferredBooks) {
        if (m_pool.contains(book) && !m_pool.value(book).isEmpty()) {
            canonEmpty = false;
            break;
        }
    }
    if (canonEmpty) {
        QStringList allBooks;
        for (const Book& book : m_active->books()) {
            allBooks.append(QString::fromStdString(book.name));
        }
        m_pool.clear();
        collect(allBooks);
    }

    // Keep only the best candidates per book, highest score first.
    for (auto it = m_pool.begin(); it != m_pool.end(); ++it) {
        QVector<VerseService::Candidate>& candidates = it.value();
        std::sort(candidates.begin(), candidates.end(),
                  [](const VerseService::Candidate& a, const VerseService::Candidate& b) {
                      if (a.score != b.score) return a.score > b.score;
                      if (a.chapter != b.chapter) return a.chapter < b.chapter;
                      return a.verse < b.verse;
                  });
        if (candidates.size() > 32) {
            candidates.resize(32);
        }
    }

    Logger::instance().info(
        QStringLiteral("Daily scripture pool: %1 books, %2 candidates")
            .arg(m_pool.size())
            .arg(std::accumulate(m_pool.cbegin(), m_pool.cend(), 0,
                                 [](int sum, const QVector<Candidate>& candidates) {
                                     return sum + candidates.size();
                                 })),
        QStringLiteral("logos"));
}

DailyVerse VerseService::passageFrom(const Candidate& candidate) const
{
    DailyVerse verse;
    const Book* book = m_active->findBook(candidate.book.toStdString());
    if (!book) return verse;
    const Chapter& chapter = book->chapter(candidate.chapter);
    if (chapter.verses.empty()) return verse;

    // Extend from the anchor verse into a complete thought.
    int last = candidate.verse - 1;
    int count = 1;
    while (last + 1 < static_cast<int>(chapter.verses.size()) && count < kMaxPassageVerses) {
        const QString text = cleanVerseText(chapter.verses.at(last).text);
        if (count >= kMinPassageVerses && !text.isEmpty()
            && isTerminalPunctuation(text.at(text.size() - 1))) {
            break;
        }
        ++last;
        ++count;
    }

    QStringList parts;
    for (int i = candidate.verse - 1; i <= last; ++i) {
        parts.append(cleanVerseText(chapter.verses.at(i).text));
    }

    verse.valid = true;
    verse.book = candidate.book;
    verse.chapter = candidate.chapter;
    verse.verseStart = candidate.verse;
    verse.verseEnd = last + 1;
    verse.reference = candidate.verse == last + 1
        ? QStringLiteral("%1 %2:%3").arg(candidate.book).arg(candidate.chapter).arg(candidate.verse)
        : QStringLiteral("%1 %2:%3-%4").arg(candidate.book).arg(candidate.chapter)
              .arg(candidate.verse).arg(last + 1);
    verse.text = parts.join(QStringLiteral(" "));
    verse.translation = QString::fromStdString(m_active->shortName());
    return verse;
}

DailyVerse VerseService::dailyVerse() const
{
    if (!m_loaded || !m_active) {
        const_cast<VerseService*>(this)->ensureLoaded();
    }
    if (!m_active) return {};
    buildPool();

    // Deterministic per-day rotation: pick a book by the day number, then
    // rotate through that book's best candidates.
    const qint64 day = QDate::currentDate().toJulianDay();
    QStringList books;
    for (const QString& name : kPreferredBooks) {
        if (m_pool.contains(name) && !m_pool.value(name).isEmpty()) {
            books.append(name);
        }
    }
    if (books.isEmpty()) {
        books = m_pool.keys();
    }
    if (books.isEmpty()) return {};

    const qint64 bookIndex = day % books.size();
    const QVector<Candidate>& candidates = m_pool.value(books.at(static_cast<int>(bookIndex)));
    if (candidates.isEmpty()) return {};

    const qint64 pick = (day / books.size()) % candidates.size();
    return passageFrom(candidates.at(static_cast<int>(pick)));
}

QStringList VerseService::search(const QString& query, int limit) const
{
    const_cast<VerseService*>(this)->ensureLoaded();
    if (!m_active || query.trimmed().isEmpty()) return {};

    const auto hits = m_search->search(query.toStdString(), *m_active);
    QStringList results;
    for (size_t i = 0; i < hits.size() && results.size() < static_cast<size_t>(limit); ++i) {
        results.append(QStringLiteral("%1 %2:%3  —  %4")
                           .arg(QString::fromStdString(hits[i].bookName))
                           .arg(hits[i].chapter)
                           .arg(hits[i].verse)
                           .arg(QString::fromStdString(hits[i].text)));
    }
    return results;
}

bool VerseService::isLoaded() const
{
    return m_loaded;
}

} // namespace arete::services
