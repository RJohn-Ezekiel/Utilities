#include "NoteStorage.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

NoteStorage::NoteStorage()
{
    load();
}

std::filesystem::path NoteStorage::filePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return std::filesystem::path(dir.toStdString()) / "notes.json";
}

void NoteStorage::setNote(std::string refKey, std::string text)
{
    notes_[std::move(refKey)] = std::move(text);
    save();
}

std::string NoteStorage::note(std::string_view refKey) const
{
    auto it = notes_.find(refKey);
    if (it != notes_.end())
        return it->second;
    return {};
}

void NoteStorage::deleteNote(std::string_view refKey)
{
    notes_.erase(std::string(refKey));
    save();
}

bool NoteStorage::hasNote(std::string_view refKey) const
{
    return notes_.contains(refKey);
}

std::vector<std::pair<std::string, std::string>> NoteStorage::allNotes() const
{
    std::vector<std::pair<std::string, std::string>> result;
    result.reserve(notes_.size());
    for (const auto& [key, text] : notes_)
        result.emplace_back(key, text);
    return result;
}

void NoteStorage::save()
{
    QJsonObject obj;
    for (const auto& [key, text] : notes_)
        obj[QString::fromStdString(key)] = QString::fromStdString(text);

    QJsonDocument doc(obj);
    QFile file(QString::fromStdString(filePath().string()));
    if (file.open(QIODevice::WriteOnly))
        file.write(doc.toJson(QJsonDocument::Indented));
}

void NoteStorage::load()
{
    notes_.clear();

    QFile file(QString::fromStdString(filePath().string()));
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject())
        return;

    for (auto it = doc.object().begin(); it != doc.object().end(); ++it)
        notes_[it.key().toStdString()] = it.value().toString().toStdString();
}
