#include "arete/search/SearchUtils.h"
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QSet>
#include <algorithm>
#include <climits>

namespace arete::search {

QString normalize(const QString& text)
{
    QString result = text;
    result = result.toLower();
    result = result.normalized(QString::NormalizationForm_KD);
    QString stripped;
    stripped.reserve(result.size());
    for (const QChar& c : result) {
        if (c.category() == QChar::Mark_NonSpacing || c.category() == QChar::Mark_SpacingCombining) {
            continue; // strip diacritics
        }
        stripped.append(c);
    }
    return stripped.trimmed().simplified();
}

bool contains(const QString& text, const QString& query)
{
    return normalize(text).contains(normalize(query));
}

int occurrences(const QString& text, const QString& query)
{
    const QString t = normalize(text);
    const QString q = normalize(query);
    if (q.isEmpty()) return 0;
    int count = 0;
    int pos = 0;
    while ((pos = t.indexOf(q, pos)) != -1) {
        ++count;
        pos += q.size();
    }
    return count;
}

QStringList tokens(const QString& text)
{
    QStringList result;
    QString current;
    for (const QChar& c : text) {
        if (c.isLetterOrNumber() || c.isMark()) {
            current.append(c.toLower());
        } else if (!current.isEmpty()) {
            result.append(current);
            current.clear();
        }
    }
    if (!current.isEmpty()) result.append(current);
    return result;
}

std::optional<SearchResult> scoreItem(const QString& title, const QString& query,
                                      const QVector<FieldWeight>& fields,
                                      const QString& subtitle, const QString& target,
                                      int maxSnippetLength)
{
    const QString normQuery = normalize(query);
    if (normQuery.isEmpty()) return std::nullopt;

    double bestScore = 0.0;
    QString bestSnippet;
    int bestPosition = -1;

    // Score title extra
    {
        const QString t = normalize(title);
        const int pos = t.indexOf(normQuery);
        if (pos != -1) {
            const double boost = 3.0 * (pos == 0 ? 1.5 : 1.0);
            bestScore = std::max(bestScore, boost + static_cast<double>(normQuery.size()) / std::max(1, static_cast<int>(t.size())));
            bestPosition = pos;
            bestSnippet = title;
        } else if (auto fuzzy = fuzzyScore(t, normQuery)) {
            bestScore = std::max(bestScore, *fuzzy * 2.0);
            bestPosition = t.indexOf(tokens(normQuery).first());
        }
    }

    for (const auto& field : fields) {
        const QString t = normalize(field.name);
        const int pos = t.indexOf(normQuery);
        if (pos != -1) {
            const double score = field.weight + static_cast<double>(normQuery.size()) / std::max(1, static_cast<int>(t.size()));
            if (score > bestScore) {
                bestScore = score;
                bestPosition = pos;
                bestSnippet = field.name;
            }
        } else if (auto fuzzy = fuzzyScore(t, normQuery)) {
            const double score = field.weight * 0.6 * *fuzzy;
            if (score > bestScore) {
                bestScore = score;
                bestSnippet = field.name;
            }
        }
    }

    if (bestScore <= 0.0) return std::nullopt;

    SearchResult result;
    result.title = title;
    result.snippet = bestSnippet.isEmpty() ? title : bestSnippet;
    result.subtitle = subtitle;
    result.target = target;
    result.score = bestScore;
    result.position = bestPosition;
    if (result.snippet.size() > maxSnippetLength) {
        result.snippet = result.snippet.left(maxSnippetLength) + QStringLiteral("…");
    }
    return result;
}

int editDistance(const QString& a, const QString& b, int maxDistance)
{
    const int m = a.size();
    const int n = b.size();
    if (m == 0) return n;
    if (n == 0) return m;
    if (std::abs(m - n) > maxDistance) return maxDistance + 1;

    QVector<int> prev(n + 1);
    QVector<int> curr(n + 1);
    for (int j = 0; j <= n; ++j) prev[j] = j;

    for (int i = 1; i <= m; ++i) {
        curr[0] = i;
        int best = curr[0];
        for (int j = 1; j <= n; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            int min = std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
            // Transposition
            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1]) {
                min = std::min(min, prev[n > 0 ? j - 2 : 0] + cost);
            }
            curr[j] = min;
            best = std::min(best, min);
        }
        if (best > maxDistance) return maxDistance + 1;
        prev.swap(curr);
    }
    return prev[n];
}

std::optional<double> fuzzyScore(const QString& text, const QString& query)
{
    if (query.isEmpty()) return 0.0;
    if (text.isEmpty()) return std::nullopt;

    int t = 0;
    int q = 0;
    int firstMatch = -1;
    int consecutive = 0;
    int bestConsecutive = 0;
    double score = 0.0;

    while (q < query.size() && t < text.size()) {
        if (text[t] == query[q]) {
            if (firstMatch == -1) firstMatch = t;
            ++q;
            ++consecutive;
            bestConsecutive = std::max(bestConsecutive, consecutive);
            // Bonus for matching at word boundary
            if (t == 0 || text[t - 1].isSpace() || text[t - 1] == QLatin1Char('_') || text[t - 1] == QLatin1Char('-')) {
                score += 0.5;
            }
        } else {
            consecutive = 0;
        }
        ++t;
    }

    if (q < query.size()) return std::nullopt; // Not all chars matched

    score += bestConsecutive * 0.8;
    score += (query.size() == text.size()) ? 1.0 : 0.0;
    score += (firstMatch == 0) ? 0.3 : 0.0;
    return score;
}

QString highlight(const QString& text, const QString& query, int maxLength,
                  const QString& openTag, const QString& closeTag)
{
    const QString normQuery = normalize(query);
    const QString normText = normalize(text);
    if (normQuery.isEmpty() || normText.isEmpty()) {
        QString out = text;
        if (out.size() > maxLength) out = out.left(maxLength) + QStringLiteral("…");
        return out.toHtmlEscaped();
    }

    // Find matches in normalized text
    QVector<int> positions;
    int pos = 0;
    while ((pos = normText.indexOf(normQuery, pos)) != -1) {
        positions.append(pos);
        pos += normQuery.size();
    }

    QString result;
    QString segment = text;
    int offset = 0;
    int currentPos = 0;
    if (positions.isEmpty()) {
        if (segment.size() > maxLength) segment = segment.left(maxLength) + QStringLiteral("…");
        return segment.toHtmlEscaped();
    }

    // Build a merged range map of match positions
    QVector<QPair<int, int>> ranges;
    for (int p : positions) {
        if (!ranges.isEmpty() && p <= ranges.last().second) {
            ranges.last().second = std::max(ranges.last().second, p + static_cast<int>(normQuery.size()));
        } else {
            ranges.append({p, p + static_cast<int>(normQuery.size())});
        }
    }

    // Convert normalized indices back to original indices is lossy for
    // diacritics; approximate by iterating original chars and tracking norm index.
    QString normBuilder;
    QVector<int> originalIndex;
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text[i];
        if (c.category() == QChar::Mark_NonSpacing || c.category() == QChar::Mark_SpacingCombining) {
            continue;
        }
        normBuilder.append(c.toLower());
        originalIndex.append(i);
    }

    auto mapToOriginal = [&](int normIdx) -> int {
        if (normIdx >= 0 && normIdx < originalIndex.size()) return originalIndex[normIdx];
        return -1;
    };

    int lastEnd = 0;
    for (const auto& range : ranges) {
        const int startOrig = mapToOriginal(range.first);
        int endOrig = range.second - 1 < originalIndex.size() ? originalIndex[range.second - 1] : text.size();
        if (startOrig < lastEnd || startOrig < 0) continue;
        result += text.mid(lastEnd, startOrig - lastEnd).toHtmlEscaped();
        result += openTag;
        result += text.mid(startOrig, endOrig - startOrig + 1).toHtmlEscaped();
        result += closeTag;
        lastEnd = endOrig + 1;
    }
    result += text.mid(lastEnd).toHtmlEscaped();

    // Trim to maxLength from a sensible center
    if (result.size() > maxLength) {
        // Keep tail; truncate front
        result = QStringLiteral("…") + result.right(maxLength - 1);
    }
    return result;
}

QVector<int> matchPositions(const QString& text, const QString& query)
{
    QVector<int> positions;
    const QString t = normalize(text);
    const QString q = normalize(query);
    if (q.isEmpty()) return positions;
    int pos = 0;
    while ((pos = t.indexOf(q, pos)) != -1) {
        positions.append(pos);
        pos += q.size();
    }
    return positions;
}

QStringList suggestCompletions(const QStringList& candidates, const QString& prefix, int maxSuggestions)
{
    QStringList result;
    const QString normPrefix = normalize(prefix);
    if (normPrefix.isEmpty()) return result;

    // Exact prefix first
    for (const QString& c : candidates) {
        if (normalize(c).startsWith(normPrefix)) {
            result.append(c);
        }
    }
    // Then fuzzy
    for (const QString& c : candidates) {
        if (result.contains(c)) continue;
        if (fuzzyScore(normalize(c), normPrefix).has_value()) {
            result.append(c);
        }
        if (result.size() >= maxSuggestions) break;
    }
    if (result.size() > maxSuggestions) result.resize(maxSuggestions);
    return result;
}

QVector<SearchResult> searchDirectory(const QString& directory, const QString& query,
                                      const FileSearchOptions& options)
{
    QVector<SearchResult> results;
    const QString normQuery = normalize(query);
    if (normQuery.isEmpty() || directory.isEmpty()) return results;

    QDir root(directory);
    if (!root.exists()) return results;

    QRegularExpression nameFilter;
    QStringList namePatterns = options.namePatterns;
    if (!namePatterns.isEmpty()) {
        // Convert wildcard patterns to regex
        QStringList patternParts;
        for (const QString& p : namePatterns) {
            QString rx = QRegularExpression::escape(p);
            rx.replace(QStringLiteral("\\*"), QStringLiteral(".*"));
            rx.replace(QStringLiteral("\\?"), QStringLiteral("."));
            patternParts.append(rx);
        }
        nameFilter = QRegularExpression(QStringLiteral("^(?:%1)$").arg(patternParts.join(QLatin1Char('|'))),
                                        options.caseInsensitive
                                            ? QRegularExpression::CaseInsensitiveOption
                                            : QRegularExpression::NoPatternOption);
    }

    QVector<QString> pending;
    pending.append(directory);
    QSet<QString> visited;

    while (!pending.isEmpty() && results.size() < options.maxResults) {
        const QString dir = pending.takeFirst();
        visited.insert(QFileInfo(dir).canonicalFilePath());

        QDir d(dir);
        const QFileInfoList entries = d.entryInfoList(
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable |
            (options.followSymlinks ? QDir::NoSymLinks : QDir::Dirs));

        for (const QFileInfo& info : entries) {
            if (options.exclude && options.exclude(info)) continue;

            if (info.isDir()) {
                if (options.recursive) {
                    const QString canonical = info.canonicalFilePath();
                    if (!visited.contains(canonical)) {
                        pending.append(info.absoluteFilePath());
                    }
                }
                continue;
            }

            if (!info.isFile()) continue;

            if (!namePatterns.isEmpty() && !nameFilter.match(info.fileName()).hasMatch()) continue;

            // Check file name for match
            const QString fileName = info.fileName();
            double bestScore = 0.0;
            QString snippet;

            if (auto fuzzy = fuzzyScore(normalize(fileName), normQuery)) {
                bestScore = *fuzzy * 2.0;
            }

            // Check content terms
            if (!options.contentTerms.isEmpty() && bestScore <= 0.0) {
                // Quick check: filename doesn't match; may still match content
                QFile file(info.absoluteFilePath());
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    const QByteArray data = file.read(512 * 1024); // Read first 512KB
                    const QString text = options.caseInsensitive
                        ? QString::fromUtf8(data).toLower()
                        : QString::fromUtf8(data);
                    bool allFound = true;
                    for (const QString& term : options.contentTerms) {
                        const QString t = options.caseInsensitive ? term.toLower() : term;
                        if (!text.contains(t)) {
                            allFound = false;
                            break;
                        }
                    }
                    if (allFound) {
                        bestScore = 0.5;
                        snippet = fileName;
                    }
                }
            }

            if (bestScore > 0.0) {
                SearchResult result;
                result.title = fileName;
                result.snippet = snippet.isEmpty() ? fileName : snippet;
                result.subtitle = info.absoluteFilePath();
                result.target = info.absoluteFilePath();
                result.score = bestScore;
                results.append(result);
            }
        }
    }

    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) { return a.score > b.score; });
    if (results.size() > options.maxResults) {
        results.resize(options.maxResults);
    }
    return results;
}

} // namespace arete::search