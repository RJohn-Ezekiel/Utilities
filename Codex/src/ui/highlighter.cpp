#include "ui/highlighter.h"
#include "ui/style.h"

#include <QTextDocument>

namespace codex {

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Headings
    m_h1Format.setForeground(QColor(style::ACCENT));
    m_h1Format.setFontWeight(QFont::Bold);
    m_h1Format.setFontPointSize(20);

    m_h2Format.setForeground(QColor(style::ACCENT));
    m_h2Format.setFontWeight(QFont::Bold);
    m_h2Format.setFontPointSize(17);

    m_h3Format.setForeground(QColor(style::ACCENT));
    m_h3Format.setFontWeight(QFont::Bold);
    m_h3Format.setFontPointSize(15);

    // Inline styles
    m_boldFormat.setFontWeight(QFont::Bold);

    m_italicFormat.setFontItalic(true);

    m_strikethroughFormat.setFontStrikeOut(true);
    m_strikethroughFormat.setForeground(QColor("#888888"));

    m_underlineFormat.setFontUnderline(true);

    m_codeFormat.setBackground(QColor("#2D2D2D"));
    m_codeFormat.setForeground(QColor("#D8D8D8"));

    m_codeBlockFormat.setForeground(QColor("#D8D8D8"));
    m_codeBlockFormat.setFontFamilies({QStringLiteral("JetBrains Mono")});

    m_linkFormat.setForeground(QColor(style::ACCENT));
    m_linkFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);

    m_wikiLinkFormat.setForeground(QColor(style::ACCENT));
    m_wikiLinkFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    m_wikiLinkFormat.setFontWeight(QFont::Bold);

    m_imageFormat.setForeground(QColor("#888888"));
    m_imageFormat.setFontItalic(true);

    m_blockquoteFormat.setForeground(QColor("#888888"));
    m_blockquoteFormat.setFontItalic(true);

    m_checkboxFormat.setForeground(QColor(style::ACCENT));

    m_hrFormat.setForeground(QColor(style::ACCENT));
    m_hrFormat.setFontStrikeOut(true);

    m_tagFormat.setForeground(QColor(style::ACCENT));
    m_tagFormat.setFontWeight(QFont::Bold);

    QTextCharFormat alignFormat;
    alignFormat.setForeground(QColor("#888888"));
    alignFormat.setFontItalic(true);

    // Build rules (applied in order, last wins for overlapping)
    // Horizontal rule
    m_rules.append({QRegularExpression(QStringLiteral("^\\s*[-*_]{3,}\\s*$")), m_hrFormat});

    // Headings (must be at start of line)
    m_rules.append({QRegularExpression(QStringLiteral("^#{1}\\s+.*$")), m_h1Format});
    m_rules.append({QRegularExpression(QStringLiteral("^#{2}\\s+.*$")), m_h2Format});
    m_rules.append({QRegularExpression(QStringLiteral("^#{3}\\s+.*$")), m_h3Format});

    // Blockquotes
    m_rules.append({QRegularExpression(QStringLiteral("^>\\s.*$")), m_blockquoteFormat});

    // Code blocks
    m_rules.append({QRegularExpression(QStringLiteral("^```.*$")), m_codeBlockFormat});

    // Checkboxes
    m_rules.append({QRegularExpression(QStringLiteral("- \\[[x ]?\\]")), m_checkboxFormat});

    // Images
    m_rules.append({QRegularExpression(QStringLiteral("!\\[[^\\]]*\\]\\([^)]*\\)")), m_imageFormat});

    // Wiki links [[Title]]
    m_rules.append({QRegularExpression(QStringLiteral("\\[\\[[^\\]]+\\]\\]")), m_wikiLinkFormat});

    // Markdown links [text](url)
    m_rules.append({QRegularExpression(QStringLiteral("\\[([^\\]]+)\\]\\([^)]+\\)")), m_linkFormat});

    // Tags #tag
    m_rules.append({QRegularExpression(QStringLiteral("#[\\w/-]+")), m_tagFormat});

    // Inline code
    m_rules.append({QRegularExpression(QStringLiteral("`[^`]+`")), m_codeFormat});

    // Bold **text**
    m_rules.append({QRegularExpression(QStringLiteral("\\*\\*[^*]+\\*\\*")), m_boldFormat});
    m_rules.append({QRegularExpression(QStringLiteral("__[^_]+__")), m_boldFormat});

    // Italic *text*
    m_rules.append({QRegularExpression(QStringLiteral("\\*[^*]+\\*")), m_italicFormat});
    m_rules.append({QRegularExpression(QStringLiteral("_[^_]+_")), m_italicFormat});

    // Strikethrough ~~text~~
    m_rules.append({QRegularExpression(QStringLiteral("~~[^~]+~~")), m_strikethroughFormat});

    // Underline <u>text</u>
    m_rules.append({QRegularExpression(QStringLiteral("<u>[^<]+</u>")), m_underlineFormat});

    // Alignment tags
    m_rules.append({QRegularExpression(QStringLiteral("</?center>")), alignFormat});
    m_rules.append({QRegularExpression(QStringLiteral("<p align\\s*=\\s*\"\\w+\">")), alignFormat});
    m_rules.append({QRegularExpression(QStringLiteral("</?p>")), alignFormat});

    // Unordered list markers
    QTextCharFormat listMarkerFormat;
    listMarkerFormat.setForeground(QColor(style::ACCENT));
    listMarkerFormat.setFontWeight(QFont::Bold);
    m_rules.append({QRegularExpression(QStringLiteral("^\\s*[-*+]\\s(?=\\[)")), listMarkerFormat});
    m_rules.append({QRegularExpression(QStringLiteral("^\\s*[-*+]\\s")), listMarkerFormat});

    // Ordered list markers
    m_rules.append({QRegularExpression(QStringLiteral("^\\s*\\d+\\.\\s")), listMarkerFormat});

    // Blockquote marker
    QTextCharFormat bqMarkerFormat;
    bqMarkerFormat.setForeground(QColor(style::ACCENT));
    bqMarkerFormat.setFontWeight(QFont::Bold);
    m_rules.append({QRegularExpression(QStringLiteral("^>\\s")), bqMarkerFormat});
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    // Handle fenced code blocks using Qt's block state mechanism
    bool inCode = (previousBlockState() == CODE_BLOCK_STATE);
    bool isFence = text.trimmed().startsWith(QStringLiteral("```"));

    if (isFence) {
        // Toggle code block state
        setCurrentBlockState(inCode ? 0 : CODE_BLOCK_STATE);
        setFormat(0, text.length(), m_codeBlockFormat);
        return;
    }

    if (inCode) {
        setCurrentBlockState(CODE_BLOCK_STATE);
        setFormat(0, text.length(), m_codeBlockFormat);
        return;
    }

    setCurrentBlockState(0);

    // Apply all formatting rules
    for (const auto &rule : m_rules) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto match = it.next();
            int start = match.capturedStart();
            int length = match.capturedLength();
            setFormat(start, length, rule.format);
        }
    }
}

} // namespace codex
