#include "library/LibraryScanner.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>

namespace phonio {

namespace {
constexpr int kBatchSize = 50;
}

LibraryScanner::LibraryScanner(QObject* parent)
    : QThread(parent)
{
}

void LibraryScanner::setFolders(const QStringList& folders)
{
    m_folders = folders;
}

void LibraryScanner::cancel()
{
    m_cancelled = true;
}

void LibraryScanner::run()
{
    m_cancelled = false;
    m_filesScanned = 0;
    m_batch.clear();
    for (const QString& folder : m_folders) {
        if (m_cancelled)
            break;
        scanDirectory(folder);
    }
    flush();
    if (!isInterruptionRequested())
        emit progress(m_filesScanned);
}

void LibraryScanner::scanDirectory(const QString& path)
{
    const QDir dir(path);
    const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                                                    QDir::DirsFirst | QDir::Name);
    for (const QFileInfo& info : entries) {
        if (m_cancelled)
            return;
        if (info.isDir()) {
            if (!info.isSymLink())
                scanDirectory(info.absoluteFilePath());
        } else if (m_metadata.isSupportedFile(info.absoluteFilePath())) {
            const auto result = m_metadata.read(info.absoluteFilePath());
            if (result.ok) {
                m_batch.append(result.track);
                if (m_batch.size() >= kBatchSize)
                    flush();
            }
        }
        ++m_filesScanned;
        if (m_filesScanned % 100 == 0)
            emit progress(m_filesScanned);
    }
}

void LibraryScanner::flush()
{
    if (!m_batch.isEmpty()) {
        emit scannedTracks(m_batch);
        m_batch.clear();
    }
}

} // namespace phonio
