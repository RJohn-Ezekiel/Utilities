#include "arete/filewatch/FileWatcher.h"
#include <QTimer>
#include <QDir>
#include <QHash>
#include <unordered_map>

namespace arete::filewatch {

class FileWatcher::Private
{
public:
    QFileSystemWatcher watcher;
    std::unordered_map<QString, FileWatcher::Callback> fileCallbacks;
    std::unordered_map<QString, FileWatcher::Callback> dirCallbacks;
    QHash<QString, QTimer*> pendingTimers;
    int debounceMs = 100;
    QHash<QString, qint64> fileSizes;
};


FileWatcher::~FileWatcher() = default;

FileWatcher::FileWatcher(QObject* parent)
    : QObject(parent), d(std::make_unique<Private>())
{
    connect(&d->watcher, &QFileSystemWatcher::fileChanged, this,
            [this](const QString& path) {
        // Re-add (files get unwatched after change)
        if (QFileInfo::exists(path)) {
            d->watcher.addPath(path);
        }
        emit fileChanged(path);
        if (auto it = d->fileCallbacks.find(path); it != d->fileCallbacks.end()) {
            // Debounce
            auto timerIt = d->pendingTimers.constFind(path);
            if (timerIt != d->pendingTimers.constEnd()) {
                timerIt.value()->start();
            } else {
                auto* timer = new QTimer(this);
                timer->setSingleShot(true);
                timer->setInterval(d->debounceMs);
                connect(timer, &QTimer::timeout, this, [this, path, cb = it->second]() {
                    d->pendingTimers.remove(path);
                    if (cb) cb(path);
                });
                d->pendingTimers.insert(path, timer);
                timer->start();
            }
        }
    });

    connect(&d->watcher, &QFileSystemWatcher::directoryChanged, this,
            [this](const QString& path) {
        emit directoryChanged(path);
        if (auto it = d->dirCallbacks.find(path); it != d->dirCallbacks.end()) {
            auto timerIt = d->pendingTimers.constFind(path);
            if (timerIt != d->pendingTimers.constEnd()) {
                timerIt.value()->start();
            } else {
                auto* timer = new QTimer(this);
                timer->setSingleShot(true);
                timer->setInterval(d->debounceMs);
                connect(timer, &QTimer::timeout, this, [this, path, cb = it->second]() {
                    d->pendingTimers.remove(path);
                    if (cb) cb(path);
                });
                d->pendingTimers.insert(path, timer);
                timer->start();
            }
        }
    });
}

bool FileWatcher::addFile(const QString& filePath, Callback callback)
{
    if (d->fileCallbacks.find(filePath) != d->fileCallbacks.end()) {
        return false;
    }
    if (!QFileInfo::exists(filePath)) {
        return false;
    }
    d->fileCallbacks[filePath] = std::move(callback);
    const bool added = d->watcher.addPath(filePath);
    if (added) {
        d->fileSizes.insert(filePath, QFileInfo(filePath).size());
    } else {
        d->fileCallbacks.erase(filePath);
    }
    return added;
}

bool FileWatcher::addDirectory(const QString& dirPath, Callback callback)
{
    if (d->dirCallbacks.find(dirPath) != d->dirCallbacks.end()) {
        return false;
    }
    if (!QDir(dirPath).exists()) {
        return false;
    }
    d->dirCallbacks[dirPath] = std::move(callback);
    const bool added = d->watcher.addPath(dirPath);
    if (!added) {
        d->dirCallbacks.erase(dirPath);
    }
    return added;
}

void FileWatcher::removeDirectory(const QString& dirPath)
{
    d->watcher.removePath(dirPath);
    d->dirCallbacks.erase(dirPath);
    if (auto it = d->pendingTimers.find(dirPath); it != d->pendingTimers.end()) {
        it.value()->deleteLater();
        d->pendingTimers.erase(it);
    }
}

void FileWatcher::removePath(const QString& path)
{
    d->watcher.removePath(path);
    d->fileCallbacks.erase(path);
    d->dirCallbacks.erase(path);
    if (auto it = d->pendingTimers.find(path); it != d->pendingTimers.end()) {
        it.value()->deleteLater();
        d->pendingTimers.erase(it);
    }
}

void FileWatcher::clear()
{
    d->watcher.removePaths(d->watcher.files());
    d->watcher.removePaths(d->watcher.directories());
    d->fileCallbacks.clear();
    d->dirCallbacks.clear();
    for (auto it = d->pendingTimers.begin(); it != d->pendingTimers.end(); ++it) {
        it.value()->deleteLater();
    }
    d->pendingTimers.clear();
    d->fileSizes.clear();
}

QStringList FileWatcher::watchedFiles() const
{
    return d->watcher.files();
}

QStringList FileWatcher::watchedDirectories() const
{
    return d->watcher.directories();
}

void FileWatcher::setDebounceInterval(int ms)
{
    d->debounceMs = std::max(0, ms);
}

int FileWatcher::debounceInterval() const
{
    return d->debounceMs;
}

} // namespace arete::filewatch