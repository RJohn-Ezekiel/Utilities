#pragma once

#include "core/Types.h"

#include <QVector>
#include <QString>
#include <QFile>
#include <optional>

namespace phonio {

struct LyricsDocument
{
    QString title;
    QString artist;
    QString album;
    int offsetMs = 0;               // positive offset shifts lyrics later
    QVector<LyricLine> lines;       // sorted by time, deduplicated timestamps expanded
    bool isEmpty() const { return lines.isEmpty(); }
};

// Pure parser for the LRC format. No Qt widget dependencies, unit-testable.
class LyricsParser
{
public:
    // Parses the content of an .lrc file.
    static LyricsDocument parse(const QString& content);

    // Loads + parses a file; returns std::nullopt on any error.
    static std::optional<LyricsDocument> loadFromFile(const QString& filePath);

    // Index of the last line with timeMs <= positionMs, -1 if none.
    static int activeLine(const LyricsDocument& doc, qint64 positionMs);
};

} // namespace phonio
