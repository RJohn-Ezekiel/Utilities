#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QList>
#include <QRegularExpression>
#include <QFileInfo>
#include <functional>
#include <optional>

namespace arete::search {

// ============================================================================
// Basic utilities
// ============================================================================

// Normalize text for search: lowercase, collapse whitespace, trim
[[nodiscard]] QString normalize(const QString& text);

// Case-insensitive substring match with diacritic folding
[[nodiscard]] bool contains(const QString& text, const QString& query);

// Number of occurrences of query in text
[[nodiscard]] int occurrences(const QString& text, const QString& query);

// Tokenize text into words
[[nodiscard]] QStringList tokens(const QString& text);

// ============================================================================
// Scored search results
// ============================================================================

struct SearchResult
{
    QString title;
    QString snippet;
    QString subtitle;    // e.g. path or category
    QString target;      // opaque id or path to jump to
    double score = 0.0;
    int position = -1;   // position of match in text (for highlighting)

    bool operator<(const SearchResult& other) const { return score < other.score; }
};

// ============================================================================
// Search scoring
// ============================================================================

// Score a single item by matching query against fields.
// Returns nullopt if no match.
struct FieldWeight
{
    QString name;    // field value to search in
    double weight = 1.0;
};

[[nodiscard]] std::optional<SearchResult> scoreItem(
    const QString& title,
    const QString& query,
    const QVector<FieldWeight>& fields,
    const QString& subtitle = QString(),
    const QString& target = QString(),
    int maxSnippetLength = 120);

// Filter + rank a list of items.
// item() must return title, fields, subtitle, target for a given index.
template <typename Item, typename Accessor>
[[nodiscard]] QVector<SearchResult> search(const QVector<Item>& items,
                                           const QString& query,
                                           const Accessor& accessor,
                                           int maxResults = 50)
{
    QVector<SearchResult> results;
    if (query.trimmed().isEmpty()) return results;

    const QString norm = normalize(query);
    for (const Item& item : items) {
        auto fields = accessor(item); // QVector<FieldWeight>
        auto result = scoreItem(
            fields.isEmpty() ? QString() : fields.first().name, norm, fields);
        if (!result) continue;
        if (result->score <= 0.0) continue;
        results.append(*result);
    }

    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) { return a.score > b.score; });
    if (results.size() > maxResults) {
        results.resize(maxResults);
    }
    return results;
}

// ============================================================================
// Fuzzy matching
// ============================================================================

// Damerau-Levenshtein distance (edit distance) with transpositions
[[nodiscard]] int editDistance(const QString& a, const QString& b,
                               int maxDistance = INT_MAX);

// Fuzzy match: subsequence matching where characters can be skipped.
// Returns match score (higher = better) or nullopt if no match.
[[nodiscard]] std::optional<double> fuzzyScore(const QString& text, const QString& query);

// ============================================================================
// Highlighting
// ============================================================================

// Build an HTML snippet with <b> around query matches
[[nodiscard]] QString highlight(const QString& text, const QString& query,
                                int maxLength = 120,
                                const QString& openTag = QStringLiteral("<b>"),
                                const QString& closeTag = QStringLiteral("</b>"));

// Positions of matches (for manual highlighting in rich text)
[[nodiscard]] QVector<int> matchPositions(const QString& text, const QString& query);

// ============================================================================
// Prefix / suffix utilities
// ============================================================================

// Suggest completions for a given prefix
[[nodiscard]] QStringList suggestCompletions(const QStringList& candidates,
                                             const QString& prefix,
                                             int maxSuggestions = 10);

// ============================================================================
// Filesystem search (async-friendly; call off the UI thread)
// ============================================================================

struct FileSearchOptions
{
    bool recursive = true;
    bool caseInsensitive = true;
    QStringList namePatterns;    // e.g. {"*.md", "*.txt"}
    QStringList contentTerms;   // if set, files must contain these terms
    int maxResults = 100;
    bool followSymlinks = false;
    std::function<bool(const QFileInfo&)> exclude;
};

// Search a directory tree. Returns results sorted by relevance.
// (Pure function; no Qt event loop dependency.)
[[nodiscard]] QVector<SearchResult> searchDirectory(
    const QString& directory,
    const QString& query,
    const FileSearchOptions& options);

} // namespace arete::search