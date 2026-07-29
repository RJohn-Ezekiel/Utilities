#include "HistoryStorage.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <algorithm>

HistoryStorage::HistoryStorage()
{
    load();
}

std::filesystem::path HistoryStorage::filePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return std::filesystem::path(dir.toStdString()) / "history.json";
}

void HistoryStorage::push(Reference ref)
{
    // Remove duplicate if exists
    auto it = std::ranges::find_if(history_, [&](const Reference& r) {
        return r.book == ref.book
            && r.chapter == ref.chapter
            && r.verseStart == ref.verseStart
            && r.verseEnd == ref.verseEnd;
    });
    if (it != history_.end())
        history_.erase(it);

    history_.insert(history_.begin(), std::move(ref));

    if (static_cast<int>(history_.size()) > maxHistory_)
        history_.resize(static_cast<size_t>(maxHistory_));

    save();
}

std::vector<Reference> HistoryStorage::recent(int count) const
{
    if (count <= 0)
        return {};
    auto n = std::min(static_cast<size_t>(count), history_.size());
    return {history_.begin(), history_.begin() + static_cast<ptrdiff_t>(n)};
}

void HistoryStorage::clear()
{
    history_.clear();
    save();
}

void HistoryStorage::save()
{
    QJsonArray arr;
    for (const auto& ref : history_) {
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

void HistoryStorage::load()
{
    history_.clear();

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
        history_.push_back(std::move(ref));
    }
}
