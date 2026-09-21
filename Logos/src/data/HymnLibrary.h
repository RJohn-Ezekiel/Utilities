#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>
#include <QFileSystemWatcher>
#include <functional>
#include <memory>

// A single hymn, editable in hymns.json without recompilation.
struct Hymn {
    QString id;
    int number = 0;
    QString title;
    QString latin;
    QString english;
    QString tune;
    QString author;
    QString composer;
    QString category;
    QString notes;

    [[nodiscard]] bool isValid() const { return !latin.isEmpty() || !english.isEmpty(); }
    [[nodiscard]] QString displayTitle() const
    {
        if (!title.isEmpty()) return title;
        return number > 0 ? QStringLiteral("Hymn %1").arg(number) : QStringLiteral("Untitled");
    }
    [[nodiscard]] QJsonObject toJson() const;
    static Hymn fromJson(const QJsonObject& obj);
};

class HymnLibrary {
public:
    HymnLibrary() = default;
    ~HymnLibrary() = default;

    // Load hymns from hymns.json. Searches several standard locations.
    bool load(const QString& baseDir);

    [[nodiscard]] const QVector<Hymn>& hymns() const { return hymns_; }
    [[nodiscard]] QString filePath() const { return filePath_; }
    [[nodiscard]] QString lastError() const { return lastError_; }

    // Search across title, latin, english, tune, author, composer, category.
    [[nodiscard]] QVector<Hymn> search(const QString& query) const;

    // Persist the in-memory collection back to hymns.json. Returns false
    // (and sets lastError_) if the file cannot be written.
    bool save();

    // Add, update or remove a hymn and persist immediately. `addHymn`
    // assigns a fresh id when `hymn.id` is empty. Returns false on
    // persistence failure (the in-memory change is still applied).
    bool addHymn(const Hymn& hymn);
    bool updateHymn(const Hymn& hymn);
    bool removeHymn(const QString& id);

    // Watch for external edits.
    void setReloadCallback(std::function<void()> callback);
    void pollForChanges();

    static QStringList defaultSearchPaths(const QString& baseDir);

private:
    bool loadFromFile(const QString& path);
    void startWatcher(const QString& path);

    QVector<Hymn> hymns_;
    QString filePath_;
    QString lastError_;
    qint64 lastModified_ = 0;
    std::function<void()> reloadCallback_;
    std::unique_ptr<class QFileSystemWatcher> watcher_;
};
