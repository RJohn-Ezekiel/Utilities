#pragma once

#include "core/Types.h"
#include "lyrics/LyricsParser.h"

#include <QObject>
#include <optional>

namespace phonio {

class LibraryManager;

// Resolves and caches lyrics for tracks.
// Resolution order: attached path (DB) -> .lrc beside the audio file.
class LyricsManager : public QObject
{
    Q_OBJECT

public:
    enum class Source { None, BesideFile, Attached };

    explicit LyricsManager(LibraryManager* library, QObject* parent = nullptr);

    // Loads lyrics for a track (cached). Emits lyricsLoaded.
    void loadLyricsFor(const Track& track);
    void clear();

    const LyricsDocument& document() const { return m_document; }
    Source source() const { return m_source; }
    QString sourcePath() const { return m_sourcePath; }
    bool hasLyrics() const { return !m_document.isEmpty(); }
    bool isLoading() const { return m_loading; }

    // Attaches an .lrc file to a track.
    // copyBeside: copy the file next to the audio file; otherwise store the path in the DB.
    // Returns the final path on success, empty string on failure.
    QString attachLyrics(const Track& track, const QString& lrcPath, bool copyBeside);

    // Detaches (clears) attached lyrics of a track.
    void detachLyrics(const Track& track);

    static QString autoLyricsPath(const QString& audioFilePath);

signals:
    void lyricsLoaded();

private:
    LibraryManager* m_library;
    LyricsDocument m_document;
    Source m_source = Source::None;
    QString m_sourcePath;
    bool m_loading = false;
    qint64 m_loadedForTrackId = -1;
};

} // namespace phonio
