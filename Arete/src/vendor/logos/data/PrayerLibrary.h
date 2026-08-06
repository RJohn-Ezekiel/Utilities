#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFileSystemWatcher>
#include <functional>
#include <memory>

// A single prayer, editable in prayer.json without recompilation.
struct Prayer {
    QString id;
    QString title;
    QString category;
    QString latin;
    QString english;
    QStringList tags;
    QString author;
    QString source;
    QString notes;

    [[nodiscard]] bool isValid() const { return !latin.isEmpty() || !english.isEmpty(); }
    [[nodiscard]] QJsonObject toJson() const;
    static Prayer fromJson(const QJsonObject& obj);
};

class PrayerLibrary {
public:
    PrayerLibrary() = default;
    ~PrayerLibrary() = default;

    // Load prayers from prayer.json. Searches several standard locations.
    bool load(const QString& baseDir);

    [[nodiscard]] const QVector<Prayer>& prayers() const { return prayers_; }
    [[nodiscard]] QString filePath() const { return filePath_; }
    [[nodiscard]] QString lastError() const { return lastError_; }

    // Search across title, latin, english, tags, category, author, source.
    [[nodiscard]] QVector<Prayer> search(const QString& query) const;
    [[nodiscard]] QVector<Prayer> byCategory(const QString& category) const;
    [[nodiscard]] QStringList categories() const;

    // Persist the in-memory collection back to prayer.json. Returns false
    // (and sets lastError_) if the file cannot be written.
    bool save();

    // Add, update or remove a prayer and persist immediately. `addPrayer`
    // assigns a fresh id when `prayer.id` is empty. Returns false on
    // persistence failure (the in-memory change is still applied).
    bool addPrayer(const Prayer& prayer);
    bool updatePrayer(const Prayer& prayer);
    bool removePrayer(const QString& id);

    // Watch for external edits.
    void setReloadCallback(std::function<void()> callback);
    void pollForChanges();

    static QStringList defaultSearchPaths(const QString& baseDir);

private:
    bool loadFromFile(const QString& path);
    void startWatcher(const QString& path);

    QVector<Prayer> prayers_;
    QString filePath_;
    QString lastError_;
    qint64 lastModified_ = 0;
    std::function<void()> reloadCallback_;
    std::unique_ptr<class QFileSystemWatcher> watcher_;
};
