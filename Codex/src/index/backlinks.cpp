#include "index/backlinks.h"
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

BacklinkManager::BacklinkManager() = default;

void BacklinkManager::rebuild(const std::filesystem::path &vaultRoot) {
    m_graph.clear();

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
            auto links = Parser::extractWikiLinks(buffer.str());

            for (const auto &link : links) {
                m_graph[link].push_back(relative.string());
            }
        }
    };

    scanDir(vault_layout::DIR_NOTES);
    scanDir(vault_layout::DIR_JOURNAL);
}

void BacklinkManager::updateNote(const std::filesystem::path &notePath,
                                  const std::vector<std::string> &wikiLinks) {
    std::string key = notePath.string();

    for (auto &[target, sources] : m_graph) {
        auto it = std::remove(sources.begin(), sources.end(), key);
        sources.erase(it, sources.end());
    }

    for (const auto &target : wikiLinks) {
        auto &sources = m_graph[target];
        if (std::find(sources.begin(), sources.end(), key) == sources.end())
            sources.push_back(key);
    }
}

void BacklinkManager::removeNote(const std::filesystem::path &notePath) {
    std::string key = notePath.string();
    for (auto &[target, sources] : m_graph) {
        auto it = std::remove(sources.begin(), sources.end(), key);
        sources.erase(it, sources.end());
    }
}

std::vector<std::filesystem::path> BacklinkManager::backlinksTo(const std::filesystem::path &notePath) const {
    std::vector<std::filesystem::path> result;
    // Wiki links use the note title (filename without extension), not the full path
    std::string title = notePath.stem().string();

    for (const auto &[target, sources] : m_graph) {
        if (target == title) {
            result.reserve(sources.size());
            for (const auto &s : sources)
                result.emplace_back(s);
            break;
        }
    }

    return result;
}

std::vector<std::filesystem::path> BacklinkManager::forwardLinksFrom(const std::filesystem::path &notePath) const {
    std::vector<std::filesystem::path> result;
    std::string key = notePath.string();

    for (const auto &[target, sources] : m_graph) {
        for (const auto &s : sources) {
            if (s == key)
                result.emplace_back(target);
        }
    }

    return result;
}

bool BacklinkManager::save(const std::filesystem::path &path) const {
    QJsonObject root;
    for (const auto &[target, sources] : m_graph) {
        QJsonArray arr;
        for (const auto &s : sources)
            arr.append(QString::fromStdString(s));
        root[QString::fromStdString(target)] = arr;
    }

    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(root).toJson());
    return true;
}

bool BacklinkManager::load(const std::filesystem::path &path) {
    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return false;

    m_graph.clear();
    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        std::string target = it.key().toStdString();
        std::vector<std::string> sources;
        for (const auto &val : it.value().toArray())
            sources.push_back(val.toString().toStdString());
        m_graph[target] = std::move(sources);
    }

    return true;
}

std::vector<std::string> BacklinkManager::scanWikiLinks(const std::filesystem::path &filePath) {
    std::ifstream ifs(filePath);
    if (!ifs)
        return {};
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return Parser::extractWikiLinks(buffer.str());
}

} // namespace codex
