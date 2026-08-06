#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <functional>
#include <memory>

namespace arete::filewatch {

// Directory or file watcher with debounce support
class FileWatcher : public QObject
{
    Q_OBJECT

public:
    using Callback = std::function<void(const QString& path)>;

    explicit FileWatcher(QObject* parent = nullptr);
    ~FileWatcher() override;

    // Watch a file (recreates on replacement)
    bool addFile(const QString& filePath, Callback callback);

    // Watch a directory recursively (create, change, delete)
    bool addDirectory(const QString& dirPath, Callback callback);
    void removeDirectory(const QString& dirPath);

    void removePath(const QString& path);
    void clear();

    [[nodiscard]] QStringList watchedFiles() const;
    [[nodiscard]] QStringList watchedDirectories() const;

    // Debounce interval in ms (default 100)
    void setDebounceInterval(int ms);
    [[nodiscard]] int debounceInterval() const;

signals:
    void fileChanged(const QString& path);
    void directoryChanged(const QString& path);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace arete::filewatch