#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

namespace codex {

class MarkdownHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit MarkdownHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<Rule> m_rules;
    QTextCharFormat m_h1Format;
    QTextCharFormat m_h2Format;
    QTextCharFormat m_h3Format;
    QTextCharFormat m_boldFormat;
    QTextCharFormat m_italicFormat;
    QTextCharFormat m_strikethroughFormat;
    QTextCharFormat m_underlineFormat;
    QTextCharFormat m_codeFormat;
    QTextCharFormat m_codeBlockFormat;
    QTextCharFormat m_linkFormat;
    QTextCharFormat m_wikiLinkFormat;
    QTextCharFormat m_imageFormat;
    QTextCharFormat m_blockquoteFormat;
    QTextCharFormat m_checkboxFormat;
    QTextCharFormat m_hrFormat;
    QTextCharFormat m_tagFormat;

    static constexpr int CODE_BLOCK_STATE = 1;
};

} // namespace codex
