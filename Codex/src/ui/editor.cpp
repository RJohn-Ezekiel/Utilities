#include "ui/editor.h"
#include "ui/style.h"
#include "ui/highlighter.h"
#include "markdown/parser.h"

#include <QFile>
#include <QTextStream>
#include <QKeyEvent>
#include <QFont>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QUrl>

namespace codex {

Editor::Editor(QWidget *parent)
    : QStackedWidget(parent)
{
    setMinimumHeight(100);

    m_sourceEdit = new QPlainTextEdit;
    m_sourceEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_sourceEdit->setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
    m_sourceEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    QFont editorFont(QStringLiteral("JetBrains Mono"));
    editorFont.setStyleHint(QFont::Monospace);
    editorFont.setPointSize(13);
    m_sourceEdit->setFont(editorFont);
    updateCodeBlockBackgrounds();

    m_highlighter = new MarkdownHighlighter(m_sourceEdit->document());

    m_preview = new QTextBrowser;
    m_preview->setOpenExternalLinks(false);
    m_preview->setOpenLinks(false);

    addWidget(m_sourceEdit);
    addWidget(m_preview);
    setCurrentIndex(0);

    m_sourceEdit->viewport()->installEventFilter(this);

    m_autosaveTimer = new QTimer(this);
    m_autosaveTimer->setInterval(3000);
    m_autosaveTimer->setSingleShot(true);
    connect(m_autosaveTimer, &QTimer::timeout, this, &Editor::autosave);

    connect(m_sourceEdit, &QPlainTextEdit::textChanged, this, &Editor::onTextChanged);
    connect(m_sourceEdit, &QPlainTextEdit::cursorPositionChanged, this, &Editor::onCursorPosChanged);
    connect(m_preview, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        setMode(Source);
        Q_EMIT wikiLinkClicked(url.path());
    });

}

void Editor::loadFile(const std::filesystem::path &path)
{
    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    m_savedContent = in.readAll();
    m_sourceEdit->setPlainText(m_savedContent);
    m_currentFile = path;
    m_modified = false;
    m_sourceEdit->document()->setModified(false);
    updateCodeBlockBackgrounds();
    applyRendering();
}

void Editor::saveFile()
{
    if (m_currentFile.empty())
        return;

    auto content = m_sourceEdit->toPlainText();
    QFile file(QString::fromStdString(m_currentFile.string()));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << content;
    file.close();

    m_savedContent = content;
    m_modified = false;
    m_sourceEdit->document()->setModified(false);
    emit fileSaved(m_currentFile);
}

bool Editor::isModified() const noexcept
{
    return m_modified || m_sourceEdit->document()->isModified();
}

void Editor::clear()
{
    m_sourceEdit->clear();
    m_preview->clear();
    m_currentFile.clear();
    m_modified = false;
    m_savedContent.clear();
    m_sourceEdit->document()->setModified(false);
    updateCodeBlockBackgrounds();
}

void Editor::setVaultRoot(const std::filesystem::path &root)
{
    m_vaultRoot = root;
}

void Editor::setMode(Mode mode)
{
    if (mode == m_mode) return;
    m_mode = mode;
    setCurrentIndex(mode == Preview ? 1 : 0);

    if (mode == Preview) {
        // Sync preview with latest source
        applyRendering();
    } else {
        m_sourceEdit->setFocus();
    }
    emit modeChanged(mode);
}

void Editor::toggleMode()
{
    setMode(m_mode == Source ? Preview : Source);
}

QString Editor::rawContent() const
{
    return m_sourceEdit->toPlainText();
}

QTextCursor Editor::textCursor() const
{
    return m_sourceEdit->textCursor();
}

void Editor::setTextCursor(const QTextCursor &cursor)
{
    m_sourceEdit->setTextCursor(cursor);
}

void Editor::setFocus()
{
    m_sourceEdit->setFocus();
}

void Editor::applyRendering()
{
    auto md = m_sourceEdit->toPlainText().toStdString();
    auto html = Parser::toHtml(md);

    // Resolve relative media paths to absolute file:// URLs
    QString htmlQ = QString::fromStdString(html);
    if (!m_vaultRoot.empty()) {
        auto vaultRootStr = QString::fromStdString(m_vaultRoot.string());
        QRegularExpression srcRe(QStringLiteral("(src\\s*=\\s*\")([^\"]+)(\")"),
                                 QRegularExpression::CaseInsensitiveOption);
        auto it = srcRe.globalMatch(htmlQ);
        QList<QPair<int, int>> replacements;
        while (it.hasNext()) {
            auto m = it.next();
            auto url = m.captured(2);
            if (!url.startsWith(QStringLiteral("file://")) &&
                !url.startsWith(QStringLiteral("http://")) &&
                !url.startsWith(QStringLiteral("https://")) &&
                !url.startsWith(QChar('/'))) {
                auto absPath = QUrl::fromLocalFile(vaultRootStr + QChar('/') + url).toString();
                replacements.append({m.capturedStart(2), m.capturedLength(2)});
            }
        }
        for (int i = replacements.size() - 1; i >= 0; --i) {
            auto [pos, len] = replacements[i];
            auto url = htmlQ.mid(pos, len);
            if (!url.startsWith(QStringLiteral("file://")) &&
                !url.startsWith(QStringLiteral("http://")) &&
                !url.startsWith(QStringLiteral("https://")) &&
                !url.startsWith(QChar('/'))) {
                auto absPath = QUrl::fromLocalFile(vaultRootStr + QChar('/') + url).toString();
                htmlQ.replace(pos, len, absPath);
            }
        }
    }

    auto styled = QStringLiteral(
        "<html><head><style>"
        "body { background: %1; color: %2; font-family: 'JetBrains Mono', monospace; font-size: 13px; "
        "padding: 1em; line-height: 1.6; }"
        "a { color: %3; }"
        "h1, h2, h3, h4, h5, h6 { color: %3; margin: 0.8em 0 0.3em; }"
        "h1 { font-size: 1.6em; } h2 { font-size: 1.4em; } h3 { font-size: 1.2em; }"
        "code { background: #2D2D2D; padding: 0.2em 0.4em; border-radius: 3px; }"
        "pre { background: #2D2D2D; padding: 0.8em; border-radius: 4px; }"
        "pre code { background: none; padding: 0; }"
        "blockquote { border-left: 3px solid %3; margin: 0.5em 0; padding: 0.3em 0.8em; background: #252526; }"
        "img { max-width: 100%%; border-radius: 4px; }"
        "video { max-width: 100%%; border-radius: 4px; }"
        "ul, ol { margin: 0.8em 0; padding-left: 2em; }"
        "ul.contains-task-list { list-style: none; padding-left: 0; }"
        "li.task-list-item { list-style: none; }"
        "s { color: #888; }"
        "u { text-decoration: underline; }"
        ".center, div[style*=\"text-align:center\"] { text-align: center; }"
        "div[style*=\"text-align:left\"] { text-align: left; }"
        "div[style*=\"text-align:right\"] { text-align: right; }"
        "div[style*=\"text-align:justify\"] { text-align: justify; }"
        "p[style*=\"text-align\"] { text-align: inherit; }"
        "</style></head><body>%4</body></html>"
    ).arg(style::BG_PRIMARY, style::TEXT_PRIMARY, style::ACCENT, htmlQ);

    m_preview->setHtml(styled);
}

void Editor::updateCodeBlockBackgrounds()
{
    QList<QTextEdit::ExtraSelection> selections;
    auto *doc = m_sourceEdit->document();
    bool inCode = false;

    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QString text = block.text().trimmed();
        if (text.startsWith(QStringLiteral("```"))) {
            inCode = !inCode;
            continue;
        }
        if (!inCode)
            continue;

        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(QColor("#252526"));
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        QTextCursor cursor(block);
        cursor.select(QTextCursor::BlockUnderCursor);
        sel.cursor = cursor;
        selections.append(sel);
    }

    m_sourceEdit->setExtraSelections(selections);
}

void Editor::onTextChanged()
{
    auto current = m_sourceEdit->toPlainText();
    if (current != m_savedContent) {
        if (!m_modified) {
            m_modified = true;
            emit modifiedChanged(true);
        }
        m_autosaveTimer->start();
    } else if (m_modified) {
        m_modified = false;
        emit modifiedChanged(false);
        m_autosaveTimer->stop();
    }
    updateCodeBlockBackgrounds();
    emit textChanged();
}

void Editor::onCursorPosChanged()
{
    emit cursorPositionChanged(cursorLine(), cursorColumn());
}

void Editor::autosave()
{
    if (isModified() && !m_currentFile.empty())
        saveFile();
}

int Editor::cursorLine() const
{
    return m_sourceEdit->textCursor().blockNumber() + 1;
}

int Editor::cursorColumn() const
{
    return m_sourceEdit->textCursor().columnNumber() + 1;
}

int Editor::wordCount() const
{
    const auto text = m_sourceEdit->toPlainText();
    if (text.trimmed().isEmpty())
        return 0;
    int count = 0;
    bool inWord = false;
    for (const QChar &c : text) {
        if (c.isSpace()) {
            inWord = false;
        } else if (!inWord) {
            ++count;
            inWord = true;
        }
    }
    return count;
}

int Editor::characterCount() const
{
    return m_sourceEdit->toPlainText().length();
}

void Editor::setModified(bool m)
{
    m_modified = m;
    m_sourceEdit->document()->setModified(m);
}

bool Editor::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_sourceEdit->viewport() && event->type() == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent*>(event);
        auto cursor = m_sourceEdit->cursorForPosition(me->pos());
        auto block = cursor.block();
        QString line = block.text();
        int col = cursor.columnNumber();

        // Check if click is on a checkbox: - [ ] or - [x] within the line
        // Match the checkbox pattern and see if the click falls within it
        QRegularExpression cbRe(QStringLiteral("(- \\[)([ x])(\\])"));
        auto it = cbRe.globalMatch(line);
        while (it.hasNext()) {
            auto m = it.next();
            int start = m.capturedStart();
            int end = m.capturedEnd();
            if (col >= start && col <= end) {
                // Toggle the checkbox
                QTextCursor tc = m_sourceEdit->textCursor();
                tc.setPosition(block.position() + start + 3); // position of [x] or [ ]
                tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
                QString ch = tc.selectedText();
                tc.removeSelectedText();
                tc.insertText(ch == QStringLiteral("x") ? QStringLiteral(" ") : QStringLiteral("x"));
                tc.clearSelection();
                return true;
            }
        }
    }
    return QStackedWidget::eventFilter(obj, event);
}

void Editor::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && m_mode == Preview) {
        setMode(Source);
        return;
    }
    QStackedWidget::keyPressEvent(event);
}

} // namespace codex
