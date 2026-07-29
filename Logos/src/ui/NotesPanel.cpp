#include "NotesPanel.h"
#include "Theme.h"
#include "storage/NoteStorage.h"

#include <QVBoxLayout>

NotesPanel::NotesPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void NotesPanel::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    header_ = new QLabel("Notes");
    header_->setStyleSheet(QStringLiteral(
        "font-size:11px; color:%1; text-transform:uppercase; letter-spacing:1px;"
    ).arg(Theme::secondaryText.name()));

    editor_ = new QPlainTextEdit;
    editor_->setPlaceholderText("Add a note for this passage...");
    editor_->setVisible(false);

    saveBtn_ = new QPushButton("Save Note");
    saveBtn_->setVisible(false);

    emptyLabel_ = new QLabel("Select a passage\nto add notes");
    emptyLabel_->setAlignment(Qt::AlignCenter);
    emptyLabel_->setStyleSheet(QStringLiteral(
        "color:%1; font-size:12px; padding:20px;"
    ).arg(Theme::secondaryText.name()));

    layout->addWidget(header_);
    layout->addWidget(editor_);
    layout->addWidget(saveBtn_);
    layout->addWidget(emptyLabel_);
    layout->addStretch();

    connect(saveBtn_, &QPushButton::clicked, this, &NotesPanel::saveNote);
    connect(editor_, &QPlainTextEdit::textChanged, this, [this]() {
        saveBtn_->setVisible(true);
    });
}

void NotesPanel::setStorage(NoteStorage* storage)
{
    storage_ = storage;
}

void NotesPanel::setCurrentReference(const Reference& ref)
{
    currentRef_ = ref;
    saveBtn_->setVisible(false);

    if (!ref.isValid()) {
        emptyLabel_->setVisible(true);
        editor_->setVisible(false);
        saveBtn_->setVisible(false);
        return;
    }

    emptyLabel_->setVisible(false);
    editor_->setVisible(true);
    loadNote();
}

void NotesPanel::loadNote()
{
    if (!storage_ || !currentRef_.isValid())
        return;

    std::string key = currentRef_.toString();
    QString text = QString::fromStdString(storage_->note(key));
    editor_->setPlainText(text);
    saveBtn_->setVisible(false);
}

void NotesPanel::saveNote()
{
    if (!storage_ || !currentRef_.isValid())
        return;

    std::string key = currentRef_.toString();
    std::string text = editor_->toPlainText().toStdString();

    if (text.empty())
        storage_->deleteNote(key);
    else
        storage_->setNote(key, text);

    saveBtn_->setVisible(false);
    emit noteChanged(key);
}
