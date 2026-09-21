#include "PrayerLibrary.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonValue>
#include <QFileSystemWatcher>
#include <QCoreApplication>
#include <QStandardPaths>

#include "arete/json/JsonUtils.h"
#include "arete/logging/Logger.h"

QJsonObject Prayer::toJson() const
{
    QJsonObject obj;
    obj["title"] = title;
    obj["category"] = category;
    obj["latin"] = latin;
    obj["english"] = english;
    obj["tags"] = QJsonArray::fromStringList(tags);
    obj["author"] = author;
    obj["source"] = source;
    obj["notes"] = notes;
    return obj;
}

Prayer Prayer::fromJson(const QJsonObject& obj)
{
    Prayer p;
    p.id = QString::number(qHash(obj["title"].toString() + obj["latin"].toString()), 16);
    p.title = obj["title"].toString();
    p.category = obj["category"].toString();
    p.latin = obj["latin"].toString();
    p.english = obj["english"].toString();
    const auto tagArr = obj["tags"].toArray();
    for (const auto& v : tagArr) p.tags.append(v.toString());
    p.author = obj["author"].toString();
    p.source = obj["source"].toString();
    p.notes = obj["notes"].toString();
    return p;
}

QStringList PrayerLibrary::defaultSearchPaths(const QString& baseDir)
{
    QStringList paths;
    paths.append(baseDir + "/prayer.json");
    paths.append(QCoreApplication::applicationDirPath() + "/prayer.json");
    paths.append(QCoreApplication::applicationDirPath() + "/../prayer.json");
    paths.append(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/prayer.json");
    paths.append(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/prayer.json");
    paths.append(QDir::homePath() + "/.config/arete/logos/prayer.json");
    return paths;
}

bool PrayerLibrary::load(const QString& baseDir)
{
    prayers_.clear();
    filePath_.clear();
    lastError_.clear();

    for (const QString& path : defaultSearchPaths(baseDir)) {
        if (QFileInfo::exists(path)) {
            return loadFromFile(path);
        }
    }

    lastError_ = QStringLiteral("prayer.json not found in any standard location");
    arete::logging::Logger::instance().warning(lastError_, QStringLiteral("logos"));
    return false;
}

bool PrayerLibrary::loadFromFile(const QString& path)
{
    auto result = arete::json::loadFile(path);
    if (result.hasError()) {
        lastError_ = result.error().toString();
        arete::logging::Logger::instance().warning(
            QStringLiteral("Failed to load %1: %2").arg(path, lastError_), QStringLiteral("logos"));
        return false;
    }

    const QJsonDocument doc = result.value();
    QJsonArray array;
    if (doc.isArray()) {
        array = doc.array();
    } else if (doc.isObject() && doc.object().contains("prayers")) {
        array = doc.object()["prayers"].toArray();
    } else {
        lastError_ = QStringLiteral("prayer.json must be an array of prayer objects");
        arete::logging::Logger::instance().warning(lastError_, QStringLiteral("logos"));
        return false;
    }

    int skipped = 0;
    for (const QJsonValue& value : array) {
        if (!value.isObject()) { ++skipped; continue; }
        Prayer p = Prayer::fromJson(value.toObject());
        if (p.isValid()) prayers_.append(p);
        else ++skipped;
    }

    filePath_ = path;
    lastModified_ = QFileInfo(path).lastModified().toSecsSinceEpoch();
    startWatcher(path);
    arete::logging::Logger::instance().info(
        QStringLiteral("Loaded %1 prayers from %2").arg(prayers_.size()).arg(path), QStringLiteral("logos"));
    return true;
}

void PrayerLibrary::startWatcher(const QString& path)
{
    watcher_ = std::make_unique<QFileSystemWatcher>();
    watcher_->addPath(path);
    QObject::connect(watcher_.get(), &QFileSystemWatcher::fileChanged, [this, path]() {
        // Re-arm (Qt drops watches after a change) then reload if modified.
        watcher_->addPath(path);
        pollForChanges();
    });
}

void PrayerLibrary::pollForChanges()
{
    if (filePath_.isEmpty()) return;
    const qint64 mtime = QFileInfo(filePath_).lastModified().toSecsSinceEpoch();
    if (mtime != lastModified_ && QFileInfo::exists(filePath_)) {
        const auto previous = prayers_;
        loadFromFile(filePath_);
        if (reloadCallback_) reloadCallback_();
    }
}

void PrayerLibrary::setReloadCallback(std::function<void()> callback)
{
    reloadCallback_ = std::move(callback);
}

QVector<Prayer> PrayerLibrary::search(const QString& query) const
{
    if (query.trimmed().isEmpty()) return prayers_;
    const QString q = query.trimmed().toLower();

    QVector<Prayer> results;
    for (const Prayer& p : prayers_) {
        if (p.title.toLower().contains(q) ||
            p.latin.toLower().contains(q) ||
            p.english.toLower().contains(q) ||
            p.category.toLower().contains(q) ||
            p.author.toLower().contains(q) ||
            p.source.toLower().contains(q) ||
            p.notes.toLower().contains(q)) {
            for (const QString& t : p.tags) {
                if (t.toLower().contains(q)) { results.append(p); break; }
            }
        }
    }
    return results;
}

QVector<Prayer> PrayerLibrary::byCategory(const QString& category) const
{
    QVector<Prayer> results;
    for (const Prayer& p : prayers_) {
        if (p.category.compare(category, Qt::CaseInsensitive) == 0) results.append(p);
    }
    return results;
}

QStringList PrayerLibrary::categories() const
{
    QStringList list;
    for (const Prayer& p : prayers_) {
        if (!p.category.isEmpty() && !list.contains(p.category)) list.append(p.category);
    }
    return list;
}

bool PrayerLibrary::save()
{
    if (filePath_.isEmpty()) {
        lastError_ = QStringLiteral("no prayer.json path loaded yet");
        arete::logging::Logger::instance().warning(lastError_, QStringLiteral("logos"));
        return false;
    }

    QJsonArray array;
    for (const Prayer& p : prayers_) {
        array.append(p.toJson());
    }
    auto result = arete::json::saveArray(filePath_, array, /*createBackup=*/true, /*prettyPrint=*/true);
    if (result.hasError()) {
        lastError_ = result.error().toString();
        arete::logging::Logger::instance().warning(
            QStringLiteral("Failed to save %1: %2").arg(filePath_, lastError_), QStringLiteral("logos"));
        return false;
    }
    lastModified_ = QFileInfo(filePath_).lastModified().toSecsSinceEpoch();
    return true;
}

bool PrayerLibrary::addPrayer(const Prayer& prayer)
{
    Prayer p = prayer;
    if (p.id.isEmpty()) {
        p.id = QString::number(qHash(p.title + p.latin + p.english
                                     + QString::number(QDateTime::currentMSecsSinceEpoch())), 16);
    }
    prayers_.append(p);
    return save();
}

bool PrayerLibrary::updatePrayer(const Prayer& prayer)
{
    if (prayer.id.isEmpty()) return addPrayer(prayer);
    for (Prayer& p : prayers_) {
        if (p.id == prayer.id) {
            p = prayer;
            return save();
        }
    }
    prayers_.append(prayer);
    return save();
}

bool PrayerLibrary::removePrayer(const QString& id)
{
    if (id.isEmpty()) return false;
    bool removed = false;
    for (int i = 0; i < prayers_.size(); ++i) {
        if (prayers_.at(i).id == id) {
            prayers_.removeAt(i);
            removed = true;
            break;
        }
    }
    return removed && save();
}
