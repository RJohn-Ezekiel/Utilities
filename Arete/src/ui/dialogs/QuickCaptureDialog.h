#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>

namespace arete {
namespace app { class AppContext; }

namespace ui {

enum class CaptureKind { Task, Note, Journal, Idea, Bookmark };

// Quick capture available everywhere (Ctrl+Shift+A): creates a task, note,
// journal entry, project idea or bookmark without switching tabs.
class QuickCaptureDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QuickCaptureDialog(app::AppContext* context, CaptureKind initialKind,
                                QWidget* parent = nullptr);

    static CaptureKind kindFromString(const QString& s);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void acceptCapture();
    void handleTask(const QString& text);
    void handleNote(const QString& text);
    void handleJournal(const QString& text);
    void handleIdea(const QString& text);
    void handleBookmark(const QString& text);

    app::AppContext* m_context;
    QComboBox* m_kind;
    QLineEdit* m_input;
};

} // namespace ui
} // namespace arete
