#include "markdown/parser.h"

#include <QRegularExpression>
#include <QString>
#include <QMap>

#include <algorithm>
#include <regex>
#include <sstream>
#include <unordered_set>

namespace codex {

Note Parser::parse(const std::string &content, const std::filesystem::path &relativePath) {
    Note note;
    note.path = relativePath;
    note.content = content;
    note.title = extractTitle(content);
    note.tags = extractTags(content);
    note.wikiLinks = extractWikiLinks(content);

    auto fm = extractFrontmatter(content);
    for (const auto &[key, val] : fm) {
        if (key == "journal" && val == "true") {
            note.isJournal = true;
        } else if (key == "template" && val == "true") {
            note.isTemplate = true;
        } else if (key == "deleted" && val == "true") {
            note.isDeleted = true;
        }
    }

    return note;
}

std::string Parser::extractTitle(const std::string &content) {
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.size() >= 2 && line[0] == '#' && line[1] == ' ') {
            auto title = line.substr(2);
            auto end = title.find_last_not_of(" \t\r\n");
            if (end != std::string::npos)
                title.erase(end + 1);
            return title;
        }
    }
    return {};
}

static bool isInCodeBlock(const std::string &content, size_t pos) {
    std::istringstream stream(content);
    std::string line;
    size_t currentPos = 0;
    bool inCode = false;
    while (std::getline(stream, line)) {
        size_t lineStart = currentPos;
        size_t lineEnd = currentPos + line.size() + 1;
        if (pos >= lineStart && pos < lineEnd)
            return inCode;
        if (line.size() >= 3 && line.substr(0, 3) == "```")
            inCode = !inCode;
        currentPos = lineEnd;
    }
    return false;
}

std::vector<std::string> Parser::extractTags(const std::string &content) {
    std::unordered_set<std::string> seen;
    std::vector<std::string> result;

    QString qcontent = QString::fromStdString(content);
    QRegularExpression re(QStringLiteral("(?<!\\w)#(\\w[\\w-]*)"));
    auto it = re.globalMatch(qcontent);

    while (it.hasNext()) {
        auto match = it.next();
        size_t pos = static_cast<size_t>(match.capturedStart());
        if (isInCodeBlock(content, pos))
            continue;

        std::string tag = match.captured(1).toStdString();
        if (seen.insert(tag).second)
            result.push_back(tag);
    }

    return result;
}

std::vector<std::string> Parser::extractWikiLinks(const std::string &content) {
    std::unordered_set<std::string> seen;
    std::vector<std::string> result;

    QString qcontent = QString::fromStdString(content);
    QRegularExpression re(QStringLiteral("\\[\\[([^\\]|]+)(?:\\|[^\\]]+)?\\]\\]"));
    auto it = re.globalMatch(qcontent);

    while (it.hasNext()) {
        auto match = it.next();
        size_t pos = static_cast<size_t>(match.capturedStart());
        if (isInCodeBlock(content, pos))
            continue;

        std::string link = match.captured(1).toStdString();
        if (seen.insert(link).second)
            result.push_back(link);
    }

    return result;
}

bool Parser::hasFrontmatter(const std::string &content) {
    if (content.size() < 4)
        return false;
    return content[0] == '-' && content[1] == '-' && content[2] == '-' && content[3] == '\n';
}

std::vector<std::pair<std::string, std::string>> Parser::extractFrontmatter(const std::string &content) {
    std::vector<std::pair<std::string, std::string>> result;
    if (!hasFrontmatter(content))
        return result;

    std::istringstream stream(content);
    std::string line;

    std::getline(stream, line);

    while (std::getline(stream, line)) {
        if (line.size() >= 3 && line.substr(0, 3) == "---")
            break;

        auto colonPos = line.find(':');
        if (colonPos == std::string::npos)
            continue;

        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);

        auto trim = [](std::string &s) {
            auto start = s.find_first_not_of(" \t\r\n");
            auto end = s.find_last_not_of(" \t\r\n");
            if (start == std::string::npos) {
                s.clear();
            } else {
                s = s.substr(start, end - start + 1);
            }
        };
        trim(key);
        trim(value);

        if (!key.empty())
            result.emplace_back(std::move(key), std::move(value));
    }

    return result;
}

// ── Block-level helpers for toHtml ──

enum class BlockType { None, Paragraph, Heading, CodeBlock, Blockquote,
                       UnorderedList, OrderedList, TaskList, HorizontalRule };

static std::string escapeHtml(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;"; break;
            case '<':  out += "&lt;"; break;
            case '>':  out += "&gt;"; break;
            case '"':  out += "&quot;"; break;
            default:   out += c; break;
        }
    }
    return out;
}

static std::string inlineToHtml(const std::string &text) {
    QString qt = QString::fromStdString(text);

    QRegularExpression codeRe(QStringLiteral("`([^`]+)`"));
    qt.replace(codeRe, QStringLiteral("<code>\\1</code>"));

    QRegularExpression boldRe(QStringLiteral("\\*\\*(.+?)\\*\\*"));
    qt.replace(boldRe, QStringLiteral("<strong>\\1</strong>"));

    QRegularExpression italicRe(QStringLiteral("(?<!\\*)\\*(?!\\*)(.+?)(?<!\\*)\\*(?!\\*)"));
    qt.replace(italicRe, QStringLiteral("<em>\\1</em>"));

    QRegularExpression strikeRe(QStringLiteral("~~(.+?)~~"));
    qt.replace(strikeRe, QStringLiteral("<s>\\1</s>"));

    QRegularExpression imageRe(QStringLiteral("!\\[([^\\]]*)\\]\\(([^)]+)\\)"));
    qt.replace(imageRe, QStringLiteral("<img src=\"\\2\" alt=\"\\1\">"));

    QRegularExpression wikiRe(QStringLiteral("\\[\\[([^\\]|]+)(?:\\|([^\\]]+))?\\]\\]"));
    {
        QString result;
        int lastEnd = 0;
        auto it = wikiRe.globalMatch(qt);
        while (it.hasNext()) {
            auto m = it.next();
            result += qt.mid(lastEnd, m.capturedStart() - lastEnd);
            auto title = m.captured(1);
            auto display = m.captured(2).isEmpty() ? title : m.captured(2);
            result += QStringLiteral("<a href=\"%1\">%2</a>").arg(title, display);
            lastEnd = m.capturedEnd();
        }
        result += qt.mid(lastEnd);
        qt = result;
    }

    QRegularExpression linkRe(QStringLiteral("\\[([^\\]]+)\\]\\(([^)]+)\\)"));
    qt.replace(linkRe, QStringLiteral("<a href=\"\\2\">\\1</a>"));

    return qt.toStdString();
}

// Detect line type for block-level parsing
struct LineInfo {
    BlockType type = BlockType::Paragraph;
    int headingLevel = 0;
    bool taskChecked = false;
    std::string content;
};

static LineInfo classifyLine(const std::string &line) {
    LineInfo info;
    info.content = line;

    // Heading
    if (line[0] == '#') {
        size_t level = 0;
        while (level < line.size() && line[level] == '#')
            ++level;
        if (level <= 6 && level < line.size() && line[level] == ' ') {
            info.type = BlockType::Heading;
            info.headingLevel = static_cast<int>(level);
            info.content = line.substr(level + 1);
            return info;
        }
    }

    // Horizontal rule
    static const std::regex hrRe(R"(^\s*[-*_]{3,}\s*$)");
    if (std::regex_match(line, hrRe)) {
        info.type = BlockType::HorizontalRule;
        info.content.clear();
        return info;
    }

    // Blockquote
    if (line.size() >= 2 && line[0] == '>' && line[1] == ' ') {
        info.type = BlockType::Blockquote;
        info.content = line.substr(2);
        return info;
    }

    // Check for list markers: - [ ] / - [x] (task list)
    // Also: - item, * item, + item (unordered), 1. item (ordered)
    if (line.size() >= 2) {
        // Task list: - [ ] or - [x]
        if (line.size() >= 6 && line[0] == '-' && line[1] == ' ' && line[2] == '[' &&
            (line[3] == ' ' || line[3] == 'x') && line[4] == ']' && line[5] == ' ') {
            info.type = BlockType::TaskList;
            info.taskChecked = (line[3] == 'x');
            info.content = line.substr(6);
            return info;
        }
        // Unordered list: -, *, +
        if ((line[0] == '-' || line[0] == '*' || line[0] == '+') && line[1] == ' ') {
            info.type = BlockType::UnorderedList;
            info.content = line.substr(2);
            return info;
        }
        // Ordered list: 1. 2. etc.
        static const std::regex olRe(R"(^\d+\.\s(.+)$)");
        std::smatch m;
        if (std::regex_match(line, m, olRe)) {
            info.type = BlockType::OrderedList;
            info.content = m[1].str();
            return info;
        }
    }

    info.type = BlockType::Paragraph;
    info.content = line;
    return info;
}

std::string Parser::toHtml(const std::string &content) {
    // Preserve HTML tags before any processing
    QString qContent = QString::fromStdString(content);
    QMap<QString, QString> preservedTags;
    int tagIdx = 0;
    auto preserve = [&](const QString &pattern) {
        QRegularExpression re(pattern, QRegularExpression::DotMatchesEverythingOption);
        auto it = re.globalMatch(qContent);
        while (it.hasNext()) {
            auto m = it.next();
            auto placeholder = QStringLiteral("\x01TAG%1\x02").arg(tagIdx++);
            preservedTags[placeholder] = m.captured(0);
            qContent.replace(m.capturedStart(), m.capturedLength(), placeholder);
        }
    };
    preserve(QStringLiteral("<video[^>]*>.*?</video>"));
    preserve(QStringLiteral("<audio[^>]*>.*?</audio>"));
    preserve(QStringLiteral("<source[^>]*/?>"));
    preserve(QStringLiteral("<img[^>]*/?>"));
    preserve(QStringLiteral("<center>.*?</center>"));
    preserve(QStringLiteral("<p[^>]*align\\s*=\\s*\"[^\"]*\"[^>]*>.*?</p>"));
    preserve(QStringLiteral("<div[^>]*align\\s*=\\s*\"[^\"]*\"[^>]*>.*?</div>"));

    std::istringstream stream(qContent.toStdString());
    std::ostringstream html;
    std::string line;

    // Block accumulator
    BlockType currentType = BlockType::None;
    std::vector<std::string> blockLines;
    int headingLevel = 0;

    auto emitBlock = [&]() {
        if (currentType == BlockType::None || blockLines.empty()) return;

        switch (currentType) {
            case BlockType::Heading: {
                for (const auto &l : blockLines)
                    html << "<h" << headingLevel << ">"
                         << inlineToHtml(escapeHtml(l))
                         << "</h" << headingLevel << ">\n";
                break;
            }
            case BlockType::CodeBlock: {
                html << "<pre><code>";
                for (const auto &l : blockLines)
                    html << escapeHtml(l) << "\n";
                html << "</code></pre>\n";
                break;
            }
            case BlockType::Blockquote: {
                html << "<blockquote>";
                for (const auto &l : blockLines)
                    html << "<p>" << inlineToHtml(escapeHtml(l)) << "</p>\n";
                html << "</blockquote>\n";
                break;
            }
            case BlockType::UnorderedList: {
                html << "<ul>\n";
                for (const auto &l : blockLines)
                    html << "<li>" << inlineToHtml(escapeHtml(l)) << "</li>\n";
                html << "</ul>\n";
                break;
            }
            case BlockType::OrderedList: {
                html << "<ol>\n";
                for (const auto &l : blockLines)
                    html << "<li>" << inlineToHtml(escapeHtml(l)) << "</li>\n";
                html << "</ol>\n";
                break;
            }
            case BlockType::TaskList: {
                html << "<ul class=\"contains-task-list\">\n";
                for (const auto &l : blockLines) {
                    bool checked = (!l.empty() && l[0] == '\x01');
                    auto content = checked ? l.substr(1) : l;
                    // Use Unicode checkbox symbols (work in Qt rich text)
                    auto checkbox = checked
                        ? "<span style=\"color:#6B8A9E;font-size:1.3em;\">\u2611</span>"
                        : "<span style=\"color:#6B8A9E;font-size:1.3em;\">\u2610</span>";
                    html << "<li class=\"task-list-item\" style=\"list-style:none;\">"
                         << checkbox << " "
                         << inlineToHtml(escapeHtml(content))
                         << "</li>\n";
                }
                html << "</ul>\n";
                break;
            }
            case BlockType::HorizontalRule: {
                html << "<hr>\n";
                break;
            }
            case BlockType::Paragraph:
            case BlockType::None:
                break;
        }
    };

    auto emitParaBlock = [&]() {
        if (blockLines.empty()) return;
        html << "<p>";
        bool first = true;
        for (const auto &l : blockLines) {
            if (!first) html << "<br>\n";
            html << inlineToHtml(escapeHtml(l));
            first = false;
        }
        html << "</p>\n";
    };

    bool inCodeBlock = false;
    bool frontmatterSkipped = false;

    while (std::getline(stream, line)) {
        // Skip frontmatter (only at start of content)
        if (!frontmatterSkipped && line.size() >= 3 &&
            line.substr(0, 3) == "---" && hasFrontmatter(content)) {
            frontmatterSkipped = true;
            bool inFm = true;
            while (std::getline(stream, line)) {
                if (line.size() >= 3 && line.substr(0, 3) == "---") {
                    inFm = false;
                    break;
                }
            }
            if (!inFm) continue;
        }
        frontmatterSkipped = true;

        // Code block handling
        if (line.size() >= 3 && line.substr(0, 3) == "```") {
            if (inCodeBlock) {
                emitBlock();
                blockLines.clear();
                inCodeBlock = false;
                continue;
            } else {
                if (currentType != BlockType::None) {
                    if (currentType == BlockType::Paragraph)
                        emitParaBlock();
                    else
                        emitBlock();
                    blockLines.clear();
                }
                inCodeBlock = true;
                currentType = BlockType::CodeBlock;
                continue;
            }
        }

        if (inCodeBlock) {
            blockLines.push_back(line);
            continue;
        }

        // Empty line flushes block
        if (line.empty()) {
            if (currentType == BlockType::Paragraph)
                emitParaBlock();
            else if (currentType != BlockType::None)
                emitBlock();
            blockLines.clear();
            currentType = BlockType::None;
            continue;
        }

        // Classify the line
        auto info = classifyLine(line);

        // If code block mode or continuing same block type
        if (currentType == info.type && info.type != BlockType::HorizontalRule) {
            // For lists, each line is a separate item
            // For heading, we only take first heading (rest are separate blocks)
            if (info.type == BlockType::Heading) {
                emitBlock();
                blockLines.clear();
                headingLevel = info.headingLevel;
                blockLines.push_back(info.content);
                currentType = info.type;
            } else if (info.type == BlockType::TaskList) {
                blockLines.push_back((info.taskChecked ? "\x01" : "") + info.content);
            } else {
                blockLines.push_back(info.content);
            }
        } else {
            // Flush previous block
            if (currentType == BlockType::Paragraph)
                emitParaBlock();
            else if (currentType != BlockType::None)
                emitBlock();

            blockLines.clear();
            currentType = info.type;
            headingLevel = info.headingLevel;

            switch (info.type) {
                case BlockType::HorizontalRule:
                    emitBlock();
                    currentType = BlockType::None;
                    break;
                case BlockType::TaskList:
                    blockLines.push_back((info.taskChecked ? "\x01" : "") + info.content);
                    break;
                default:
                    blockLines.push_back(info.content);
                    break;
            }
        }
    }

    // Flush remaining
    if (inCodeBlock) {
        emitBlock();
    } else if (currentType == BlockType::Paragraph) {
        emitParaBlock();
    } else if (currentType != BlockType::None) {
        emitBlock();
    }

    auto result = QString::fromStdString(html.str());
    for (auto it = preservedTags.begin(); it != preservedTags.end(); ++it)
        result.replace(it.key(), it.value());

    // Convert alignment tags to styled divs (reverse-order to preserve positions)
    result.replace(QStringLiteral("<center>"), QStringLiteral("<div style=\"text-align:center;\">"));
    result.replace(QStringLiteral("</center>"), QStringLiteral("</div>"));
    auto replaceReverse = [](QString &str, const QRegularExpression &re, const std::function<QString(const QStringList&)> &fn) {
        struct Match { int start; int length; QString replacement; };
        QVector<Match> matches;
        auto it = re.globalMatch(str);
        while (it.hasNext()) {
            auto m = it.next();
            QStringList caps;
            for (int i = 0; i <= m.lastCapturedIndex(); ++i)
                caps.append(m.captured(i));
            matches.append({static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedLength()), fn(caps)});
        }
        for (int i = matches.size() - 1; i >= 0; --i)
            str.replace(matches[i].start, matches[i].length, matches[i].replacement);
    };
    replaceReverse(result,
        QRegularExpression(QStringLiteral("<p[^>]*align\\s*=\\s*\"(\\w+)\"[^>]*>")),
        [](const QStringList &caps) { return QStringLiteral("<p style=\"text-align:%1;\">").arg(caps[1]); });
    replaceReverse(result,
        QRegularExpression(QStringLiteral("<div[^>]*align\\s*=\\s*\"(\\w+)\"[^>]*>")),
        [](const QStringList &caps) { return QStringLiteral("<div style=\"text-align:%1;\">").arg(caps[1]); });

    return result.toStdString();
}

} // namespace codex
