#include "export/export.h"
#include "core/util.h"
#include "markdown/parser.h"

#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <QRegularExpression>
#include <QString>

namespace codex {

ExportManager::ExportManager(const std::filesystem::path &vaultRoot) : m_vaultRoot(vaultRoot) {}

std::string ExportManager::extensionFor(Format fmt) {
    switch (fmt) {
        case Html: return ".html";
        case Markdown: return ".md";
        case PlainText: return ".txt";
    }
    return ".html";
}

// ── Helpers (shared between export methods) ──

static std::string exportEscapeHtml(const std::string &s) {
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

static std::string exportInlineToHtml(const std::string &text) {
    auto result = text;
    result = std::regex_replace(result, std::regex(R"(`([^`]+)`)"), R"(<code>$1</code>)");
    result = std::regex_replace(result, std::regex(R"(\*\*(.+?)\*\*)"), R"(<strong>$1</strong>)");
    result = std::regex_replace(result, std::regex(R"(\*(.+?)\*)"), R"(<em>$1</em>)");
    result = std::regex_replace(result, std::regex(R"(~~(.+?)~~)"), R"(<s>$1</s>)");
    result = std::regex_replace(result, std::regex(R"(!\[([^\]]*)\]\(([^)]+)\))"), R"(<img src="$2" alt="$1">)");
    result = std::regex_replace(result, std::regex(R"(\[([^\]]+)\]\(([^)]+)\))"), R"(<a href="$2">$1</a>)");
    result = std::regex_replace(result, std::regex(R"(\[\[([^\]]+)\]\])"), R"(<a href="$1.html">$1</a>)");
    return result;
}

enum class ExpBlockType { None, Paragraph, Heading, CodeBlock, Blockquote,
                          UnorderedList, OrderedList, TaskList, HorizontalRule };

struct ExpLineInfo {
    ExpBlockType type = ExpBlockType::Paragraph;
    int headingLevel = 0;
    bool taskChecked = false;
    std::string content;
};

static ExpLineInfo classifyExportLine(const std::string &line) {
    ExpLineInfo info;
    info.content = line;

    if (line[0] == '#') {
        size_t level = 0;
        while (level < line.size() && line[level] == '#')
            ++level;
        if (level <= 6 && level < line.size() && line[level] == ' ') {
            info.type = ExpBlockType::Heading;
            info.headingLevel = static_cast<int>(level);
            info.content = line.substr(level + 1);
            return info;
        }
    }

    static const std::regex hrRe(R"(^\s*[-*_]{3,}\s*$)");
    if (std::regex_match(line, hrRe)) {
        info.type = ExpBlockType::HorizontalRule;
        info.content.clear();
        return info;
    }

    if (line.size() >= 2 && line[0] == '>' && line[1] == ' ') {
        info.type = ExpBlockType::Blockquote;
        info.content = line.substr(2);
        return info;
    }

    if (line.size() >= 2) {
        if (line.size() >= 6 && line[0] == '-' && line[1] == ' ' && line[2] == '[' &&
            (line[3] == ' ' || line[3] == 'x') && line[4] == ']' && line[5] == ' ') {
            info.type = ExpBlockType::TaskList;
            info.taskChecked = (line[3] == 'x');
            info.content = line.substr(6);
            return info;
        }
        if ((line[0] == '-' || line[0] == '*' || line[0] == '+') && line[1] == ' ') {
            info.type = ExpBlockType::UnorderedList;
            info.content = line.substr(2);
            return info;
        }
        static const std::regex olRe(R"(^\d+\.\s(.+)$)");
        std::smatch m;
        if (std::regex_match(line, m, olRe)) {
            info.type = ExpBlockType::OrderedList;
            info.content = m[1].str();
            return info;
        }
    }

    info.type = ExpBlockType::Paragraph;
    return info;
}

// ── Export Note ──

bool ExportManager::exportNote(const Note &note, const std::filesystem::path &outputDir, Format fmt) {
    std::filesystem::create_directories(outputDir);
    auto filename = sanitizeFilename(note.title) + extensionFor(fmt);

    std::ofstream f(outputDir / filename);
    if (!f) return false;

    switch (fmt) {
        case Html: {
            auto bodyHtml = markdownToHtml(note.content);
            f << renderHtml(note, bodyHtml);
            break;
        }
        case Markdown:
            f << note.content;
            break;
        case PlainText:
            f << stripMarkdown(note.content);
            break;
    }
    return bool(f);
}

bool ExportManager::exportMultiple(const std::vector<Note> &notes, const std::filesystem::path &outputDir, Format fmt) {
    std::filesystem::create_directories(outputDir);
    std::vector<std::pair<std::string, std::string>> entries;

    for (const auto &note : notes) {
        auto filename = sanitizeFilename(note.title) + extensionFor(fmt);
        if (!exportNote(note, outputDir, fmt))
            return false;
        entries.emplace_back(note.title, filename);
    }

    if (fmt == Html) {
        std::string indexBody = "<h1>Codex Export</h1>\n<ul>\n";
        for (const auto &[title, filename] : entries) {
            indexBody += "<li><a href=\"" + filename + "\">" + title + "</a></li>\n";
        }
        indexBody += "</ul>\n";
        Note dummy;
        dummy.title = "Codex Export";
        auto indexHtml = renderHtml(dummy, indexBody);
        std::ofstream f(outputDir / "index.html");
        if (!f) return false;
        f << indexHtml;
    } else {
        auto indexPath = outputDir / ("index" + extensionFor(fmt));
        std::ofstream f(indexPath);
        if (!f) return false;
        f << "# Codex Export\n\n";
        for (const auto &[title, filename] : entries) {
            f << "- [" << title << "](" << filename << ")\n";
        }
    }
    return true;
}

bool ExportManager::exportAll(const std::vector<Note> &allNotes, const std::filesystem::path &outputDir, Format fmt) {
    std::filesystem::create_directories(outputDir);
    std::vector<std::pair<std::string, std::filesystem::path>> entries;

    for (const auto &note : allNotes) {
        std::filesystem::path subPath;
        for (auto it = note.path.begin(); it != note.path.end(); ++it) {
            if (it == note.path.begin() && *it == "Notes")
                continue;
            subPath /= *it;
        }

        auto outPath = outputDir / subPath;
        outPath.replace_extension(extensionFor(fmt));
        std::filesystem::create_directories(outPath.parent_path());

        std::ofstream f(outPath);
        if (!f) return false;

        switch (fmt) {
            case Html: {
                auto bodyHtml = markdownToHtml(note.content);
                f << renderHtml(note, bodyHtml);
                break;
            }
            case Markdown:
                f << note.content;
                break;
            case PlainText:
                f << stripMarkdown(note.content);
                break;
        }
        entries.emplace_back(note.title, outPath);
    }

    if (fmt == Html) {
        std::string indexBody = "<h1>Codex Vault</h1>\n<ul>\n";
        for (const auto &[title, path] : entries) {
            auto rel = std::filesystem::relative(path, outputDir);
            indexBody += "<li><a href=\"" + rel.string() + "\">" + title + "</a></li>\n";
        }
        indexBody += "</ul>\n";
        Note dummy;
        dummy.title = "Codex Vault";
        auto indexHtml = renderHtml(dummy, indexBody);
        std::ofstream f(outputDir / "index.html");
        if (!f) return false;
        f << indexHtml;
    } else {
        auto indexPath = outputDir / ("index" + extensionFor(fmt));
        std::ofstream f(indexPath);
        if (!f) return false;
        f << "# Codex Vault\n\n";
        for (const auto &[title, path] : entries) {
            auto rel = std::filesystem::relative(path, outputDir);
            f << "- [" << title << "](" << rel.string() << ")\n";
        }
    }
    return true;
}

std::string ExportManager::stripMarkdown(const std::string &markdown) {
    QString text = QString::fromStdString(markdown);

    QRegularExpression fmRe(QStringLiteral("^---[\\s\\S]*?---\n*"));
    text.remove(fmRe);

    text.remove(QRegularExpression(QStringLiteral("```[\\s\\S]*?```")));

    text.remove(QRegularExpression(QStringLiteral("!\\[[^\\]]*\\]\\([^)]*\\)")));

    QRegularExpression linkRe(QStringLiteral("\\[([^\\]]+)\\]\\([^)]*\\)"));
    text.replace(linkRe, QStringLiteral("\\1"));

    QRegularExpression wikiRe(QStringLiteral("\\[\\[([^\\]|]+)(?:\\|[^\\]]+)?\\]\\]"));
    text.replace(wikiRe, QStringLiteral("\\1"));

    text.remove(QStringLiteral("**"));
    text.remove(QStringLiteral("__"));
    text.remove(QStringLiteral("*"));
    text.remove(QStringLiteral("_"));

    text.remove(QStringLiteral("~~"));

    text.remove(QRegularExpression(QStringLiteral("</?u>")));

    text.remove(QChar('`'));

    text.replace(QRegularExpression(QStringLiteral("^#+\\s")), QString());

    text.replace(QRegularExpression(QStringLiteral("^>\\s?")), QString());

    text.replace(QRegularExpression(QStringLiteral("- \\[[x ]?\\]")), QStringLiteral("-"));

    text.remove(QRegularExpression(QStringLiteral("<[^>]*>")));

    text.remove(QRegularExpression(QStringLiteral("^[-*_]{3,}\\s*$")));

    text.replace(QRegularExpression(QStringLiteral("\n{3,}")), QStringLiteral("\n\n"));

    return text.trimmed().toStdString();
}

// ── HTML rendering (block-level) ──

std::string ExportManager::markdownToHtml(const std::string &markdown) {
    std::istringstream stream(markdown);
    std::ostringstream html;
    std::string line;

    ExpBlockType currentType = ExpBlockType::None;
    std::vector<std::string> blockLines;
    int headingLevel = 0;

    auto emitBlock = [&]() {
        if (currentType == ExpBlockType::None || blockLines.empty()) return;

        switch (currentType) {
            case ExpBlockType::Heading: {
                for (const auto &l : blockLines)
                    html << "<h" << headingLevel << ">"
                         << exportInlineToHtml(exportEscapeHtml(l))
                         << "</h" << headingLevel << ">\n";
                break;
            }
            case ExpBlockType::CodeBlock: {
                html << "<pre><code>";
                for (const auto &l : blockLines)
                    html << exportEscapeHtml(l) << "\n";
                html << "</code></pre>\n";
                break;
            }
            case ExpBlockType::Blockquote: {
                html << "<blockquote>";
                for (const auto &l : blockLines)
                    html << "<p>" << exportInlineToHtml(exportEscapeHtml(l)) << "</p>\n";
                html << "</blockquote>\n";
                break;
            }
            case ExpBlockType::UnorderedList: {
                html << "<ul>\n";
                for (const auto &l : blockLines)
                    html << "<li>" << exportInlineToHtml(exportEscapeHtml(l)) << "</li>\n";
                html << "</ul>\n";
                break;
            }
            case ExpBlockType::OrderedList: {
                html << "<ol>\n";
                for (const auto &l : blockLines)
                    html << "<li>" << exportInlineToHtml(exportEscapeHtml(l)) << "</li>\n";
                html << "</ol>\n";
                break;
            }
            case ExpBlockType::TaskList: {
                html << "<ul class=\"contains-task-list\">\n";
                for (const auto &l : blockLines) {
                    bool checked = (!l.empty() && l[0] == '\x01');
                    auto content = checked ? l.substr(1) : l;
                    html << "<li class=\"task-list-item\">"
                         << "<input type=\"checkbox\" disabled"
                         << (checked ? " checked" : "")
                         << "> "
                         << exportInlineToHtml(exportEscapeHtml(content))
                         << "</li>\n";
                }
                html << "</ul>\n";
                break;
            }
            case ExpBlockType::HorizontalRule: {
                html << "<hr>\n";
                break;
            }
            case ExpBlockType::Paragraph:
            case ExpBlockType::None:
                break;
        }
    };

    auto emitParaBlock = [&]() {
        if (blockLines.empty()) return;
        html << "<p>";
        bool first = true;
        for (const auto &l : blockLines) {
            if (!first) html << "<br>\n";
            html << exportInlineToHtml(exportEscapeHtml(l));
            first = false;
        }
        html << "</p>\n";
    };

    bool inCodeBlock = false;

    while (std::getline(stream, line)) {
        if (line.size() >= 3 && line.substr(0, 3) == "```") {
            if (inCodeBlock) {
                emitBlock();
                blockLines.clear();
                inCodeBlock = false;
                continue;
            } else {
                if (currentType != ExpBlockType::None) {
                    if (currentType == ExpBlockType::Paragraph)
                        emitParaBlock();
                    else
                        emitBlock();
                    blockLines.clear();
                }
                inCodeBlock = true;
                currentType = ExpBlockType::CodeBlock;
                continue;
            }
        }

        if (inCodeBlock) {
            blockLines.push_back(line);
            continue;
        }

        if (line.empty()) {
            if (currentType == ExpBlockType::Paragraph)
                emitParaBlock();
            else if (currentType != ExpBlockType::None)
                emitBlock();
            blockLines.clear();
            currentType = ExpBlockType::None;
            continue;
        }

        auto info = classifyExportLine(line);

        if (currentType == info.type && info.type != ExpBlockType::HorizontalRule) {
            if (info.type == ExpBlockType::Heading) {
                emitBlock();
                blockLines.clear();
                headingLevel = info.headingLevel;
                blockLines.push_back(info.content);
                currentType = info.type;
            } else if (info.type == ExpBlockType::TaskList) {
                blockLines.push_back((info.taskChecked ? "\x01" : "") + info.content);
            } else {
                blockLines.push_back(info.content);
            }
        } else {
            if (currentType == ExpBlockType::Paragraph)
                emitParaBlock();
            else if (currentType != ExpBlockType::None)
                emitBlock();

            blockLines.clear();
            currentType = info.type;
            headingLevel = info.headingLevel;

            switch (info.type) {
                case ExpBlockType::HorizontalRule:
                    emitBlock();
                    currentType = ExpBlockType::None;
                    break;
                case ExpBlockType::TaskList:
                    blockLines.push_back((info.taskChecked ? "\x01" : "") + info.content);
                    break;
                default:
                    blockLines.push_back(info.content);
                    break;
            }
        }
    }

    if (inCodeBlock) {
        emitBlock();
    } else if (currentType == ExpBlockType::Paragraph) {
        emitParaBlock();
    } else if (currentType != ExpBlockType::None) {
        emitBlock();
    }

    return html.str();
}

std::string ExportManager::renderHtml(const Note &note, const std::string &bodyHtml) {
    auto tpl = htmlTemplate();
    auto pos = tpl.find("{{title}}");
    if (pos != std::string::npos)
        tpl.replace(pos, 9, note.title);
    pos = tpl.find("{{content}}");
    if (pos != std::string::npos)
        tpl.replace(pos, 11, bodyHtml);
    return tpl;
}

std::string ExportManager::htmlTemplate() {
    return R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>{{title}} - Codex</title>
<style>
  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    background: #1B1B1B;
    color: #D8D8D8;
    font-family: 'Segoe UI', -apple-system, Roboto, Helvetica, Arial, sans-serif;
    font-size: 16px;
    line-height: 1.7;
    max-width: 860px;
    margin: 0 auto;
    padding: 2.5em 1.5em;
  }
  a { color: #8A8A8A; text-decoration: none; }
  a:hover { text-decoration: underline; }
  h1, h2, h3, h4, h5, h6 { color: #8A8A8A; margin: 1.2em 0 0.5em; font-weight: 600; }
  h1 { font-size: 2em; border-bottom: 1px solid #333; padding-bottom: 0.3em; }
  h2 { font-size: 1.6em; }
  h3 { font-size: 1.3em; }
  p { margin: 0.8em 0; }
  code {
    background: #2B2B2B;
    color: #D8D8D8;
    padding: 0.2em 0.4em;
    border-radius: 3px;
    font-family: 'JetBrains Mono', 'Fira Code', monospace;
    font-size: 0.9em;
  }
  pre {
    background: #2B2B2B;
    padding: 1em;
    border-radius: 5px;
    overflow-x: auto;
    margin: 1em 0;
  }
  pre code { background: none; padding: 0; border-radius: 0; }
  ul, ol { margin: 0.8em 0; padding-left: 2em; }
  li { margin: 0.3em 0; }
  ul.contains-task-list { list-style: none; padding-left: 0; }
  li.task-list-item { list-style: none; }
  blockquote {
    border-left: 3px solid #8A8A8A;
    margin: 1em 0;
    padding: 0.5em 1em;
    background: #222;
    color: #B0B0B0;
  }
  hr { border: none; border-top: 1px solid #333; margin: 1.5em 0; }
  img { max-width: 100%; border-radius: 4px; }
  input[type=checkbox] { accent-color: #8A8A8A; margin-right: 0.5em; }
  s { color: #888; }
  u { text-decoration: underline; }
  .center, div[style*="text-align:center"] { text-align: center; }
  div[style*="text-align:left"] { text-align: left; }
  div[style*="text-align:right"] { text-align: right; }
  div[style*="text-align:justify"] { text-align: justify; }
  p[style*="text-align"] { text-align: inherit; }
</style>
</head>
<body>
{{content}}
</body>
</html>)";
}

std::string ExportManager::sanitizeFilename(const std::string &name) {
    return util::sanitizeFilename(name);
}

} // namespace codex
