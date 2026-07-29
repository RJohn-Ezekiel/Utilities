#include "index/tags.h"
#include "core/types.h"
#include "markdown/parser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QString>

#include <algorithm>
#include <fstream>
#include <sstream>

namespace codex {

TagIndexer::TagIndexer() = default;

void TagIndexer::rebuild(const std::filesystem::path &vaultRoot) {
    m_tagToNotes.clear();
    m_noteToTags.clear();

    auto scanDir = [&](const std::filesystem::path &dir) {
        auto dirPath = vaultRoot / dir;
        if (!std::filesystem::exists(dirPath))
            return;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(dirPath)) {
            if (!entry.is_regular_file())
                continue;
            if (entry.path().extension() != ".md")
                continue;

            auto relative = std::filesystem::relative(entry.path(), vaultRoot);
            std::ifstream ifs(entry.path());
            if (!ifs)
                continue;
            std::stringstream buffer;
            buffer << ifs.rdbuf();
            auto tags = Parser::extractTags(buffer.str());

            indexNote(relative, tags);
        }
    };

    scanDir(vault_layout::DIR_NOTES);
    scanDir(vault_layout::DIR_JOURNAL);
    scanDir(vault_layout::DIR_TEMPLATES);
}

void TagIndexer::indexNote(const std::filesystem::path &notePath, const std::vector<std::string> &tags) {
    std::string key = notePath.string();

    auto oldTags = m_noteToTags.find(key);
    if (oldTags != m_noteToTags.end()) {
        for (const auto &tag : oldTags->second) {
            auto it = m_tagToNotes.find(tag);
            if (it != m_tagToNotes.end()) {
                it->second.erase(key);
                if (it->second.empty())
                    m_tagToNotes.erase(it);
            }
        }
        m_noteToTags.erase(oldTags);
    }

    for (const auto &tag : tags) {
        m_tagToNotes[tag].insert(key);
        m_noteToTags[key].insert(tag);
    }
}

void TagIndexer::removeNote(const std::filesystem::path &notePath) {
    std::string key = notePath.string();
    auto it = m_noteToTags.find(key);
    if (it == m_noteToTags.end())
        return;

    for (const auto &tag : it->second) {
        auto tagIt = m_tagToNotes.find(tag);
        if (tagIt != m_tagToNotes.end()) {
            tagIt->second.erase(key);
            if (tagIt->second.empty())
                m_tagToNotes.erase(tagIt);
        }
    }
    m_noteToTags.erase(it);
}

std::vector<std::filesystem::path> TagIndexer::notesWithTag(const std::string &tag) const {
    std::vector<std::filesystem::path> result;
    auto it = m_tagToNotes.find(tag);
    if (it != m_tagToNotes.end()) {
        result.reserve(it->second.size());
        for (const auto &p : it->second)
            result.emplace_back(p);
    }
    return result;
}

std::vector<std::string> TagIndexer::allTags() const {
    std::vector<std::string> result;
    result.reserve(m_tagToNotes.size());
    for (const auto &[tag, _] : m_tagToNotes)
        result.push_back(tag);
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<std::pair<std::string, int>> TagIndexer::tagCounts() const {
    std::vector<std::pair<std::string, int>> result;
    result.reserve(m_tagToNotes.size());
    for (const auto &[tag, notes] : m_tagToNotes)
        result.emplace_back(tag, static_cast<int>(notes.size()));
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
    return result;
}

bool TagIndexer::save(const std::filesystem::path &path) const {
    QJsonObject root;
    for (const auto &[tag, notes] : m_tagToNotes) {
        QJsonArray arr;
        for (const auto &n : notes)
            arr.append(QString::fromStdString(n));
        root[QString::fromStdString(tag)] = arr;
    }

    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(root).toJson());
    return true;
}

bool TagIndexer::load(const std::filesystem::path &path) {
    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return false;

    m_tagToNotes.clear();
    m_noteToTags.clear();

    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        std::string tag = it.key().toStdString();
        for (const auto &val : it.value().toArray()) {
            std::string notePath = val.toString().toStdString();
            m_tagToNotes[tag].insert(notePath);
            m_noteToTags[notePath].insert(tag);
        }
    }

    return true;
}

} // namespace codex
