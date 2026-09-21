#include "HymnEditDialog.h"
#include "ui/Theme.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

QString styledFieldStyle()
{
    return QStringLiteral("QLineEdit, QPlainTextEdit { background: %1; border: 1px solid %2;"
                          " color: %3; border-radius: 6px; padding: 6px 8px; }")
        .arg(Theme::sidebar.name(), Theme::borders.name(), Theme::primaryText.name());
}

} // namespace

HymnEditDialog::HymnEditDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Edit Hymn"));
    setMinimumSize(620, 560);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* form = new QFormLayout;
    form->setSpacing(8);

    numberEdit_ = new QSpinBox;
    numberEdit_->setRange(0, 99999);
    titleEdit_ = new QLineEdit;
    tuneEdit_ = new QLineEdit;
    authorEdit_ = new QLineEdit;
    composerEdit_ = new QLineEdit;
    categoryEdit_ = new QLineEdit;

    const QString fieldStyle = styledFieldStyle();
    for (QLineEdit* e : {titleEdit_, tuneEdit_, authorEdit_, composerEdit_, categoryEdit_}) {
        e->setStyleSheet(fieldStyle);
    }
    numberEdit_->setStyleSheet(fieldStyle);

    form->addRow(QStringLiteral("Number"), numberEdit_);
    form->addRow(QStringLiteral("Title"), titleEdit_);
    form->addRow(QStringLiteral("Tune"), tuneEdit_);
    form->addRow(QStringLiteral("Author"), authorEdit_);
    form->addRow(QStringLiteral("Composer"), composerEdit_);
    form->addRow(QStringLiteral("Category"), categoryEdit_);
    root->addLayout(form);

    auto* latinLabel = new QLabel(QStringLiteral("Latin"));
    latinLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: 600;")
                                  .arg(Theme::secondaryText.name()));
    latinEdit_ = new QPlainTextEdit;
    latinEdit_->setStyleSheet(fieldStyle);
    latinEdit_->setPlaceholderText(QStringLiteral("Latin text..."));
    root->addWidget(latinLabel);
    root->addWidget(latinEdit_, 3);

    auto* englishLabel = new QLabel(QStringLiteral("English"));
    englishLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: 600;")
                                    .arg(Theme::secondaryText.name()));
    englishEdit_ = new QPlainTextEdit;
    englishEdit_->setStyleSheet(fieldStyle);
    englishEdit_->setPlaceholderText(QStringLiteral("English translation..."));
    root->addWidget(englishLabel);
    root->addWidget(englishEdit_, 3);

    auto* notesLabel = new QLabel(QStringLiteral("Notes"));
    notesLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: 600;")
                                  .arg(Theme::secondaryText.name()));
    notesEdit_ = new QPlainTextEdit;
    notesEdit_->setStyleSheet(fieldStyle);
    notesEdit_->setMaximumHeight(90);
    root->addWidget(notesLabel);
    root->addWidget(notesEdit_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Save"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

void HymnEditDialog::setHymn(const Hymn& hymn)
{
    base_ = hymn;
    numberEdit_->setValue(hymn.number);
    titleEdit_->setText(hymn.title);
    tuneEdit_->setText(hymn.tune);
    authorEdit_->setText(hymn.author);
    composerEdit_->setText(hymn.composer);
    categoryEdit_->setText(hymn.category);
    latinEdit_->setPlainText(hymn.latin);
    englishEdit_->setPlainText(hymn.english);
    notesEdit_->setPlainText(hymn.notes);
}

Hymn HymnEditDialog::hymn() const
{
    Hymn h = base_;
    h.number = numberEdit_->value();
    h.title = titleEdit_->text().trimmed();
    h.tune = tuneEdit_->text().trimmed();
    h.author = authorEdit_->text().trimmed();
    h.composer = composerEdit_->text().trimmed();
    h.category = categoryEdit_->text().trimmed();
    h.latin = latinEdit_->toPlainText().trimmed();
    h.english = englishEdit_->toPlainText().trimmed();
    h.notes = notesEdit_->toPlainText().trimmed();
    return h;
}

Hymn HymnEditDialog::editHymn(QWidget* parent, const Hymn& hymn)
{
    HymnEditDialog dialog(parent);
    dialog.setHymn(hymn);
    dialog.setWindowTitle(QStringLiteral("Edit Hymn"));
    if (dialog.exec() != QDialog::Accepted) return Hymn();
    const Hymn result = dialog.hymn();
    return result.isValid() ? result : Hymn();
}

Hymn HymnEditDialog::newHymn(QWidget* parent)
{
    HymnEditDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("New Hymn"));
    if (dialog.exec() != QDialog::Accepted) return Hymn();
    const Hymn result = dialog.hymn();
    return result.isValid() ? result : Hymn();
}