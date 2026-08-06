#pragma once

#include "metadata/MetadataManager.h"

#include <QObject>
#include <QThread>
#include <QVector>
#include <QStringList>

namespace phonio {

// Scans configured folders in a worker thread and reports found tracks.
// Emits scannedTracks() with batches and finished() when done.
class LibraryScanner : public QThread
{
    Q_OBJECT

public:
    explicit LibraryScanner(QObject* parent = nullptr);

    void setFolders(const QStringList& folders);
    void cancel();

signals:
    void scannedTracks(QVector<Track> tracks);
    void progress(int filesScanned);

protected:
    void run() override;

private:
    void scanDirectory(const QString& path);
    void flush();

    MetadataManager m_metadata;
    QStringList m_folders;
    QVector<Track> m_batch;
    volatile bool m_cancelled = false;
    int m_filesScanned = 0;
};

} // namespace phonio
