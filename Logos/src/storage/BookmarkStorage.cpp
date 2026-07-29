#include "BookmarkStorage.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <algorithm>

BookmarkStorage::BookmarkStorage()
{
    load();
}

std::filesystem::path BookmarkStorage::filePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return std::filesystem::path(dir.toStdString()) / "bookmarks.json";
}

void BookmarkStorage::add(Reference ref)
{
    if (!contains(ref)) {
        bookmarks_.push_back(std::move(ref));
        save();
    }
}

void BookmarkStorage::remove(const Reference& ref)
{
    auto it = std::ranges::find_if(bookmarks_, [&](const Reference& r) {
        return r.book == ref.book
            && r.chapter == ref.chapter
            && r.verseStart == ref.verseStart
            && r.verseEnd == ref.verseEnd;
    });
    if (it != bookmarks_.end()) {
        bookmarks_.erase(it);
        save();
    }
}

bool BookmarkStorage::contains(const Reference& ref) const
{
    return std::ranges::any_of(bookmarks_, [&](const Reference& r) {
        return r.book == ref.book
            && r.chapter == ref.chapter
            && r.verseStart == ref.verseStart
            && r.verseEnd == ref.verseEnd;
    });
}

std::vector<Reference> BookmarkStorage::all() const
{
    return bookmarks_;
}

void BookmarkStorage::save()
{
    QJsonArray arr;
    for (const auto& ref : bookmarks_) {
        QJsonObject obj;
        obj["book"] = QString::fromStdString(ref.book);
        obj["chapter"] = ref.chapter;
        if (ref.verseStart.has_value())
            obj["verseStart"] = *ref.verseStart;
        if (ref.verseEnd.has_value())
            obj["verseEnd"] = *ref.verseEnd;
        arr.append(obj);
    }

    QJsonDocument doc(arr);
    QFile file(QString::fromStdString(filePath().string()));
    if (file.open(QIODevice::WriteOnly))
        file.write(doc.toJson(QJsonDocument::Indented));
}

void BookmarkStorage::load()
{
    bookmarks_.clear();

    QFile file(QString::fromStdString(filePath().string()));
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray())
        return;

    for (const QJsonValue& val : doc.array()) {
        const QJsonObject obj = val.toObject();
        Reference ref;
        ref.book = obj["book"].toString().toStdString();
        ref.chapter = obj["chapter"].toInt();
        if (obj.contains("verseStart"))
            ref.verseStart = obj["verseStart"].toInt();
        if (obj.contains("verseEnd"))
            ref.verseEnd = obj["verseEnd"].toInt();
        bookmarks_.push_back(std::move(ref));
    }
}
