#include "lyrics/LyricsParser.h"

#include <QRegularExpression>
#include <algorithm>

namespace phonio {

namespace {

// [mm:ss.xx] or [mm:ss.x] or [mm:ss]
bool parseTimeTag(const QString& token, qint64& outMs)
{
    QRegularExpressionMatch match =
        QRegularExpression(QStringLiteral(R"(^\[(\d{1,3}):(\d{1,2})(?:[.:](\d{1,3}))?\]$)"))
            .match(token);
    if (!match.hasMatch())
        return false;
    const qint64 minutes = match.captured(1).toLongLong();
    const qint64 seconds = match.captured(2).toLongLong();
    const QString fraction = match.captured(3);
    qint64 fractionMs = 0;
    if (!fraction.isEmpty()) {
        // 3+ digits: milliseconds; 2 digits: centiseconds; 1 digit: tenths.
        if (fraction.size() >= 3)
            fractionMs = fraction.left(3).toLongLong();
        else if (fraction.size() == 2)
            fractionMs = fraction.toLongLong() * 10;
        else
            fractionMs = fraction.toLongLong() * 100;
    }
    outMs = (minutes * 60 + seconds) * 1000 + fractionMs;
    return true;
}

QString trimLine(const QString& line)
{
    QString copy = line;
    int i = 0;
    while (i < copy.size() && (copy.at(i) == QLatin1Char('[') || copy.at(i) == QLatin1Char(' ')))
        ++i;
    return copy.mid(i).trimmed();
}

} // namespace

LyricsDocument LyricsParser::parse(const QString& content)
{
    LyricsDocument doc;
    QVector<LyricLine> raw;

    const auto lines = content.split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;

        // Metadata tags
        const QRegularExpression meta(QStringLiteral(R"(^\[(ti|ar|al|offset|by|re|ve):(.+)\]$)"),
                                      QRegularExpression::CaseInsensitiveOption);
        const auto metaMatch = meta.match(trimmed);
        if (metaMatch.hasMatch()) {
            const QString key = metaMatch.captured(1).toLower();
            const QString value = metaMatch.captured(2).trimmed();
            if (key == QLatin1String("ti"))
                doc.title = value;
            else if (key == QLatin1String("ar"))
                doc.artist = value;
            else if (key == QLatin1String("al"))
                doc.album = value;
            else if (key == QLatin1String("offset"))
                doc.offsetMs = value.toInt();
            continue;
        }

        // Gather all [time] tags on this line
        QVector<qint64> times;
        QString rest = trimmed;
        while (true) {
            const QRegularExpression tag(QStringLiteral(R"(^\[[^\]]*\]\s*)"));
            const auto tagMatch = tag.match(rest);
            if (!tagMatch.hasMatch())
                break;
            const QString token = tagMatch.captured(0).trimmed();
            qint64 ms = 0;
            if (parseTimeTag(token, ms))
                times.append(ms);
            rest = rest.mid(tagMatch.capturedLength());
        }
        if (times.isEmpty())
            continue; // plain line without timestamps (e.g. "Made by ...")

        const QString text = trimLine(rest);
        for (qint64 time : times)
            raw.append({time, text});
    }

    std::sort(raw.begin(), raw.end(), [](const LyricLine& a, const LyricLine& b) {
        return a.timeMs < b.timeMs;
    });

    if (doc.offsetMs != 0) {
        for (auto& line : raw)
            line.timeMs = qMax<qint64>(0, line.timeMs + doc.offsetMs);
        std::sort(raw.begin(), raw.end(), [](const LyricLine& a, const LyricLine& b) {
            return a.timeMs < b.timeMs;
        });
    }

    doc.lines = std::move(raw);
    return doc;
}

std::optional<LyricsDocument> LyricsParser::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return std::nullopt;
    return parse(QString::fromUtf8(file.readAll()));
}

int LyricsParser::activeLine(const LyricsDocument& doc, qint64 positionMs)
{
    int active = -1;
    for (int i = 0; i < doc.lines.size(); ++i) {
        if (doc.lines.at(i).timeMs <= positionMs)
            active = i;
        else
            break;
    }
    return active;
}

} // namespace phonio
