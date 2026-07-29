#pragma once

#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QStackedWidget>
#include <QTimer>
#include <QWidget>
#include <filesystem>

namespace codex {

class MarkdownHighlighter;

class Editor : public QStackedWidget {
    Q_OBJECT
public:
    enum Mode { Source, Preview };

    explicit Editor(QWidget *parent = nullptr);

    void loadFile(const std::filesystem::path &path);
    void clear();
    void saveFile();
    bool isModified() const noexcept;
    void setVaultRoot(const std::filesystem::path &root);
    std::filesystem::path currentFile() const noexcept { return m_currentFile; }
    bool hasFile() const noexcept { return !m_currentFile.empty(); }

    void setMode(Mode mode);
    Mode mode() const { return m_mode; }
    void toggleMode();

    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor &cursor);
    void setFocus();
    int cursorLine() const;
    int cursorColumn() const;
    int wordCount() const;
    int characterCount() const;
    QString rawContent() const;

signals:
    void fileSaved(const std::filesystem::path &path);
    void cursorPositionChanged(int line, int col);
    void modifiedChanged(bool modified);
    void textChanged();
    void wikiLinkClicked(const QString &target);
    void modeChanged(Editor::Mode mode);

public slots:
    void setModified(bool m);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onTextChanged();
    void onCursorPosChanged();
    void autosave();

private:
    std::filesystem::path m_currentFile;
    std::filesystem::path m_vaultRoot;
    QPlainTextEdit *m_sourceEdit;
    QTextBrowser *m_preview;
    MarkdownHighlighter *m_highlighter = nullptr;
    Mode m_mode = Source;
    bool m_modified = false;
    QTimer *m_autosaveTimer;
    QString m_savedContent;
    void applyRendering();
    void updateCodeBlockBackgrounds();
};

} // namespace codex
