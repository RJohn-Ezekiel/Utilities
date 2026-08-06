#include "lyrics/LyricsManager.h"

#include "library/LibraryManager.h"

#include <QFileInfo>
#include <QFile>
#include <QDir>

namespace phonio {

LyricsManager::LyricsManager(LibraryManager* library, QObject* parent)
    : QObject(parent)
    , m_library(library)
{
}

QString LyricsManager::autoLyricsPath(const QString& audioFilePath)
{
    const QFileInfo info(audioFilePath);
    return info.dir().filePath(info.completeBaseName() + QStringLiteral(".lrc"));
}

void LyricsManager::loadLyricsFor(const Track& track)
{
    if (m_loading) {
        m_loadedForTrackId = track.id; // re-request when the pending load finishes
        return;
    }
    m_document = {};
    m_source = Source::None;
    m_sourcePath.clear();

    QString path = track.lyricsPath;
    m_source = Source::Attached;
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        path = autoLyricsPath(track.filePath);
        m_source = Source::BesideFile;
    }
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        m_source = Source::None;
        emit lyricsLoaded();
        return;
    }
    m_sourcePath = path;
    m_loading = true;
    m_loadedForTrackId = track.id;

    const auto doc = LyricsParser::loadFromFile(path);
    m_loading = false;
    if (doc) {
        m_document = *doc;
    } else {
        m_document = {};
        m_source = Source::None;
    }
    emit lyricsLoaded();
}

void LyricsManager::clear()
{
    m_document = {};
    m_source = Source::None;
    m_sourcePath.clear();
    m_loadedForTrackId = -1;
}

QString LyricsManager::attachLyrics(const Track& track, const QString& lrcPath, bool copyBeside)
{
    const QFileInfo info(lrcPath);
    if (!info.exists() || info.suffix().compare(QLatin1String("lrc"), Qt::CaseInsensitive) != 0)
        return {};

    if (copyBeside) {
        const QString target = autoLyricsPath(track.filePath);
        if (QFileInfo(target).exists()) {
            if (!QFile::remove(target))
                return {};
        }
        if (!QFile::copy(lrcPath, target))
            return {};
        m_library->setLyricsPath(track.id, QString());
        return target;
    }

    m_library->setLyricsPath(track.id, lrcPath);
    return lrcPath;
}

void LyricsManager::detachLyrics(const Track& track)
{
    m_library->setLyricsPath(track.id, QString());
}

} // namespace phonio
