#include "index/search.h"
#include "markdown/parser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QString>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace codex {

SearchEngine::SearchEngine() = default;

void SearchEngine::rebuild(const std::filesystem::path &vaultRoot) {
    m_entries.clear();

    auto scanDir = [&](const std::filesystem::path &dir, bool isJournal, bool isTemplate) {
        auto dirPath = vaultRoot / dir;
        if (!std::filesystem::exists(dirPath))
            return;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(dirPath)) {
            if (!entry.is_regular_file())
                continue;
            auto ext = entry.path().extension().string();
            if (ext != ".md")
                continue;

            std::ifstream ifs(entry.path());
            if (!ifs)
                continue;
            std::stringstream buffer;
            buffer << ifs.rdbuf();
            std::string content = buffer.str();

            std::filesystem::path relative = std::filesystem::relative(entry.path(), vaultRoot);

            auto note = Parser::parse(content, relative);
            if (!note.title.empty() || !content.empty()) {
                note.isJournal = isJournal;
                note.isTemplate = isTemplate;
                IndexEntry ie;
                ie.path = note.path;
                ie.title = note.title;
                ie.content = note.content;
                ie.tags = note.tags;
                ie.wikiLinks = note.wikiLinks;
                m_entries[note.path.string()] = std::move(ie);
            }
        }
    };

    scanDir(vault_layout::DIR_NOTES, false, false);
    scanDir(vault_layout::DIR_JOURNAL, true, false);
    scanDir(vault_layout::DIR_TEMPLATES, false, true);
}

void SearchEngine::indexNote(const Note &note, const std::filesystem::path & /*vaultRoot*/) {
    IndexEntry ie;
    ie.path = note.path;
    ie.title = note.title;
    ie.content = note.content;
    ie.tags = note.tags;
    ie.wikiLinks = note.wikiLinks;
    m_entries[note.path.string()] = std::move(ie);
}

void SearchEngine::removeNote(const std::filesystem::path &relativePath) {
    m_entries.erase(relativePath.string());
}

static bool ciContains(const std::string &haystack, const std::string &needle) {
    if (needle.empty())
        return true;
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](unsigned char a, unsigned char b) {
            return std::tolower(a) == std::tolower(b);
        });
    return it != haystack.end();
}

float SearchEngine::calculateRelevance(const IndexEntry &entry, const std::string &term) const {
    float score = 0.0f;
    if (ciContains(entry.title, term))
        score += 10.0f;
    for (const auto &tag : entry.tags) {
        if (ciContains(tag, term)) {
            score += 5.0f;
            break;
        }
    }
    if (ciContains(entry.content, term))
        score += 1.0f;
    for (const auto &link : entry.wikiLinks) {
        if (ciContains(link, term)) {
            score += 1.0f;
            break;
        }
    }
    return score;
}

std::string SearchEngine::snippet(const std::string &content, const std::string &term, int contextChars) const {
    if (term.empty())
        return {};

    auto toLower = [](const std::string &s) -> std::string {
        std::string out;
        out.reserve(s.size());
        for (unsigned char c : s)
            out.push_back(static_cast<char>(std::tolower(c)));
        return out;
    };

    std::string lower = toLower(content);
    std::string lowerTerm = toLower(term);
    auto pos = lower.find(lowerTerm);
    if (pos == std::string::npos)
        return {};

    auto start = pos >= static_cast<size_t>(contextChars) ? pos - contextChars : 0;
    auto end = std::min(pos + term.size() + contextChars, content.size());

    std::string result;
    if (start > 0)
        result += "...";
    result += content.substr(start, end - start);
    if (end < content.size())
        result += "...";
    return result;
}

std::vector<SearchResult> SearchEngine::query(const std::string &searchTerm) const {
    std::vector<SearchResult> results;
    results.reserve(m_entries.size());

    for (const auto &[key, entry] : m_entries) {
        float relevance = calculateRelevance(entry, searchTerm);
        if (relevance > 0.0f) {
            SearchResult sr;
            sr.notePath = entry.path;
            sr.title = entry.title;
            sr.snippet = snippet(entry.content, searchTerm);
            sr.relevance = relevance;
            results.push_back(std::move(sr));
        }
    }

    std::sort(results.begin(), results.end(),
              [](const SearchResult &a, const SearchResult &b) {
                  return a.relevance > b.relevance;
              });

    return results;
}

bool SearchEngine::save(const std::filesystem::path &path) const {
    QJsonArray entriesArr;
    for (const auto &[key, entry] : m_entries) {
        QJsonObject obj;
        obj["path"] = QString::fromStdString(entry.path.string());
        obj["title"] = QString::fromStdString(entry.title);
        obj["content"] = QString::fromStdString(entry.content);
        QJsonArray tagsArr;
        for (const auto &t : entry.tags)
            tagsArr.append(QString::fromStdString(t));
        obj["tags"] = tagsArr;
        QJsonArray linksArr;
        for (const auto &l : entry.wikiLinks)
            linksArr.append(QString::fromStdString(l));
        obj["wikiLinks"] = linksArr;
        entriesArr.append(obj);
    }

    QJsonObject root;
    root["entries"] = entriesArr;

    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(root).toJson());
    return true;
}

bool SearchEngine::load(const std::filesystem::path &path) {
    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return false;

    m_entries.clear();
    QJsonObject root = doc.object();
    QJsonArray entriesArr = root["entries"].toArray();

    for (const auto &val : entriesArr) {
        QJsonObject obj = val.toObject();
        IndexEntry entry;
        entry.path = std::filesystem::path(obj["path"].toString().toStdString());
        entry.title = obj["title"].toString().toStdString();
        entry.content = obj["content"].toString().toStdString();

        for (const auto &t : obj["tags"].toArray())
            entry.tags.push_back(t.toString().toStdString());
        for (const auto &l : obj["wikiLinks"].toArray())
            entry.wikiLinks.push_back(l.toString().toStdString());

        m_entries[entry.path.string()] = std::move(entry);
    }

    return true;
}

} // namespace codex
