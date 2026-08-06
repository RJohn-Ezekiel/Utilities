#include "HymnLibrary.h"

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

QJsonObject Hymn::toJson() const
{
    QJsonObject obj;
    obj["number"] = number;
    obj["title"] = title;
    obj["latin"] = latin;
    obj["english"] = english;
    obj["tune"] = tune;
    obj["author"] = author;
    obj["composer"] = composer;
    obj["category"] = category;
    obj["notes"] = notes;
    return obj;
}

Hymn Hymn::fromJson(const QJsonObject& obj)
{
    Hymn h;
    h.id = QString::number(qHash(obj["title"].toString() + obj["latin"].toString()), 16);
    h.number = obj["number"].toInt(0);
    h.title = obj["title"].toString();
    h.latin = obj["latin"].toString();
    h.english = obj["english"].toString();
    h.tune = obj["tune"].toString();
    h.author = obj["author"].toString();
    h.composer = obj["composer"].toString();
    h.category = obj["category"].toString();
    h.notes = obj["notes"].toString();
    return h;
}

QStringList HymnLibrary::defaultSearchPaths(const QString& baseDir)
{
    QStringList paths;
    paths.append(baseDir + "/hymns.json");
    paths.append(QCoreApplication::applicationDirPath() + "/hymns.json");
    paths.append(QCoreApplication::applicationDirPath() + "/../hymns.json");
    paths.append(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/hymns.json");
    paths.append(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/hymns.json");
    paths.append(QDir::homePath() + "/.config/arete/logos/hymns.json");
    return paths;
}

bool HymnLibrary::load(const QString& baseDir)
{
    hymns_.clear();
    filePath_.clear();
    lastError_.clear();

    for (const QString& path : defaultSearchPaths(baseDir)) {
        if (QFileInfo::exists(path)) {
            return loadFromFile(path);
        }
    }

    lastError_ = QStringLiteral("hymns.json not found in any standard location");
    arete::logging::Logger::instance().warning(lastError_, QStringLiteral("logos"));
    return false;
}

bool HymnLibrary::loadFromFile(const QString& path)
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
    } else if (doc.isObject() && doc.object().contains("hymns")) {
        array = doc.object()["hymns"].toArray();
    } else {
        lastError_ = QStringLiteral("hymns.json must be an array of hymn objects");
        arete::logging::Logger::instance().warning(lastError_, QStringLiteral("logos"));
        return false;
    }

    int skipped = 0;
    for (const QJsonValue& value : array) {
        if (!value.isObject()) { ++skipped; continue; }
        Hymn h = Hymn::fromJson(value.toObject());
        if (h.isValid()) hymns_.append(h);
        else ++skipped;
    }

    filePath_ = path;
    lastModified_ = QFileInfo(path).lastModified().toSecsSinceEpoch();
    startWatcher(path);
    arete::logging::Logger::instance().info(
        QStringLiteral("Loaded %1 hymns from %2").arg(hymns_.size()).arg(path), QStringLiteral("logos"));
    return true;
}

void HymnLibrary::startWatcher(const QString& path)
{
    watcher_ = std::make_unique<QFileSystemWatcher>();
    watcher_->addPath(path);
    QObject::connect(watcher_.get(), &QFileSystemWatcher::fileChanged, [this, path]() {
        watcher_->addPath(path);
        pollForChanges();
    });
}

void HymnLibrary::pollForChanges()
{
    if (filePath_.isEmpty()) return;
    const qint64 mtime = QFileInfo(filePath_).lastModified().toSecsSinceEpoch();
    if (mtime != lastModified_ && QFileInfo::exists(filePath_)) {
        loadFromFile(filePath_);
        if (reloadCallback_) reloadCallback_();
    }
}

void HymnLibrary::setReloadCallback(std::function<void()> callback)
{
    reloadCallback_ = std::move(callback);
}

QVector<Hymn> HymnLibrary::search(const QString& query) const
{
    if (query.trimmed().isEmpty()) return hymns_;
    const QString q = query.trimmed().toLower();

    QVector<Hymn> results;
    for (const Hymn& h : hymns_) {
        if (h.title.toLower().contains(q) ||
            h.latin.toLower().contains(q) ||
            h.english.toLower().contains(q) ||
            h.tune.toLower().contains(q) ||
            h.author.toLower().contains(q) ||
            h.composer.toLower().contains(q) ||
            h.category.toLower().contains(q) ||
            h.notes.toLower().contains(q)) {
            results.append(h);
        }
    }
    return results;
}

bool HymnLibrary::save()
{
    if (filePath_.isEmpty()) {
        lastError_ = QStringLiteral("no hymns.json path loaded yet");
        arete::logging::Logger::instance().warning(lastError_, QStringLiteral("logos"));
        return false;
    }

    QJsonArray array;
    for (const Hymn& h : hymns_) {
        array.append(h.toJson());
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

bool HymnLibrary::addHymn(const Hymn& hymn)
{
    Hymn h = hymn;
    if (h.id.isEmpty()) {
        h.id = QString::number(qHash(h.title + h.latin + h.english
                                     + QString::number(QDateTime::currentMSecsSinceEpoch())), 16);
    }
    hymns_.append(h);
    return save();
}

bool HymnLibrary::updateHymn(const Hymn& hymn)
{
    if (hymn.id.isEmpty()) return addHymn(hymn);
    for (Hymn& h : hymns_) {
        if (h.id == hymn.id) {
            h = hymn;
            return save();
        }
    }
    hymns_.push_back(hymn);
    return save();
}

bool HymnLibrary::removeHymn(const QString& id)
{
    if (id.isEmpty()) return false;
    bool removed = false;
    for (int i = 0; i < hymns_.size(); ++i) {
        if (hymns_.at(i).id == id) {
            hymns_.erase(hymns_.begin() + i);
            removed = true;
            break;
        }
    }
    return removed && save();
}
