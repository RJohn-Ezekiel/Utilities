#include "ui/dialogs/QuickCaptureDialog.h"
#include "app/AppContext.h"
#include "data/TaskRepository.h"
#include "data/JournalRepository.h"
#include "models/Task.h"
#include "models/JournalEntry.h"
#include "core/vault.h"
#include "arete/settings/SettingsManager.h"
#include "arete/notifications/NotificationCenter.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QKeyEvent>
#include <QUrl>
#include <QDir>
#include <QFileInfo>

namespace arete::ui {

namespace {
const QStringList kKinds = {
    QStringLiteral("Task"),
    QStringLiteral("Note"),
    QStringLiteral("Journal"),
    QStringLiteral("Idea"),
    QStringLiteral("Bookmark"),
};
} // namespace

QuickCaptureDialog::QuickCaptureDialog(app::AppContext* context, CaptureKind initialKind,
                                       QWidget* parent)
    : QDialog(parent), m_context(context)
{
    setWindowTitle(QStringLiteral("Quick Capture"));
    setModal(true);
    setMinimumWidth(520);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto* hint = new QLabel(QStringLiteral("Capture without leaving your place. Enter to save, Esc to dismiss."),
                            this);
    hint->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 12px;"));

    auto* row = new QHBoxLayout;
    row->setSpacing(8);
    m_kind = new QComboBox(this);
    m_kind->addItems(kKinds);
    m_kind->setCurrentIndex(static_cast<int>(initialKind));

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText(QStringLiteral("What would you like to capture?"));
    m_input->setClearButtonEnabled(true);

    row->addWidget(m_kind);
    row->addWidget(m_input, 1);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);
    auto* cancel = new QPushButton(QStringLiteral("Cancel"), this);
    auto* save = new QPushButton(QStringLiteral("Save"), this);
    save->setProperty("primary", true);
    buttons->addWidget(cancel);
    buttons->addWidget(save);

    layout->addWidget(hint);
    layout->addLayout(row);
    layout->addLayout(buttons);

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(save, &QPushButton::clicked, this, &QuickCaptureDialog::acceptCapture);
    connect(m_input, &QLineEdit::returnPressed, this, &QuickCaptureDialog::acceptCapture);
    connect(m_input, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_input->setPlaceholderText(text.isEmpty()
            ? QStringLiteral("What would you like to capture?")
            : QString());
    });

    m_input->setFocus();
}

CaptureKind QuickCaptureDialog::kindFromString(const QString& s)
{
    const QString lower = s.toLower();
    if (lower == QLatin1String("note")) return CaptureKind::Note;
    if (lower == QLatin1String("journal")) return CaptureKind::Journal;
    if (lower == QLatin1String("idea")) return CaptureKind::Idea;
    if (lower == QLatin1String("bookmark")) return CaptureKind::Bookmark;
    return CaptureKind::Task;
}

void QuickCaptureDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}

void QuickCaptureDialog::acceptCapture()
{
    const QString text = m_input->text().trimmed();
    if (text.isEmpty()) return;

    switch (static_cast<CaptureKind>(m_kind->currentIndex())) {
    case CaptureKind::Task: handleTask(text); break;
    case CaptureKind::Note: handleNote(text); break;
    case CaptureKind::Journal: handleJournal(text); break;
    case CaptureKind::Idea: handleIdea(text); break;
    case CaptureKind::Bookmark: handleBookmark(text); break;
    }
    accept();
}

void QuickCaptureDialog::handleTask(const QString& text)
{
    models::Task task;
    task.title = text;
    task.createdAt = QDateTime::currentDateTime();
    const qint64 id = m_context->tasks()->insert(task);
    if (id > 0) {
        arete::notifications::NotificationCenter::instance().showSuccess(
            QStringLiteral("Task created"), text.left(60));
    }
}

void QuickCaptureDialog::handleNote(const QString& text)
{
    // Create a Codex note in the current vault.
    const QString vaultRoot = m_context->settings()
        ->get(QStringLiteral("codex/vaultPath"), QString());
    if (vaultRoot.isEmpty()) {
        arete::notifications::NotificationCenter::instance().showWarning(
            QStringLiteral("No Codex vault"),
            QStringLiteral("Open the Codex tab once to create a vault."));
        return;
    }
    codex::VaultManager manager;
    manager.ensureVault(vaultRoot.toStdString());
    const QString title = text.left(40).simplified();
    if (manager.createNote(title.toStdString())) {
        arete::notifications::NotificationCenter::instance().showSuccess(
            QStringLiteral("Note created"), title);
    }
}

void QuickCaptureDialog::handleJournal(const QString& text)
{
    models::JournalEntry entry = m_context->journal()->forDate(QDate::currentDate());
    entry.date = QDate::currentDate();
    if (!entry.reflection.isEmpty()) {
        entry.reflection += QStringLiteral("\n");
    }
    entry.reflection += text;
    m_context->journal()->upsert(entry);
    arete::notifications::NotificationCenter::instance().showSuccess(
        QStringLiteral("Journal updated"), QStringLiteral("Added to today's entry."));
}

void QuickCaptureDialog::handleIdea(const QString& text)
{
    models::Task task;
    task.title = QStringLiteral("[Idea] ") + text;
    task.priority = models::Priority::Low;
    task.createdAt = QDateTime::currentDateTime();
    m_context->tasks()->insert(task);
    arete::notifications::NotificationCenter::instance().showSuccess(
        QStringLiteral("Idea captured"), text.left(60));
}

void QuickCaptureDialog::handleBookmark(const QString& text)
{
    models::Task task;
    task.title = QStringLiteral("[Bookmark] ") + text;
    task.createdAt = QDateTime::currentDateTime();
    m_context->tasks()->insert(task);
    arete::notifications::NotificationCenter::instance().showSuccess(
        QStringLiteral("Bookmark saved"), text.left(60));
}

} // namespace arete::ui
