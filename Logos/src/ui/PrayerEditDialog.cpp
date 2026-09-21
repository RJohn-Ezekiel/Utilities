#include "PrayerEditDialog.h"
#include "ui/Theme.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString styledFieldStyle()
{
    return QStringLiteral("QLineEdit, QPlainTextEdit { background: %1; border: 1px solid %2;"
                          " color: %3; border-radius: 6px; padding: 6px 8px; }")
        .arg(Theme::sidebar.name(), Theme::borders.name(), Theme::primaryText.name());
}

} // namespace

PrayerEditDialog::PrayerEditDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Edit Prayer"));
    setMinimumSize(620, 560);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto* form = new QFormLayout;
    form->setSpacing(8);

    titleEdit_ = new QLineEdit;
    categoryEdit_ = new QLineEdit;
    authorEdit_ = new QLineEdit;
    sourceEdit_ = new QLineEdit;
    tagsEdit_ = new QLineEdit;
    tagsEdit_->setPlaceholderText(QStringLiteral("comma, separated"));

    const QString fieldStyle = styledFieldStyle();
    for (QLineEdit* e : {titleEdit_, categoryEdit_, authorEdit_, sourceEdit_, tagsEdit_}) {
        e->setStyleSheet(fieldStyle);
    }

    form->addRow(QStringLiteral("Title"), titleEdit_);
    form->addRow(QStringLiteral("Category"), categoryEdit_);
    form->addRow(QStringLiteral("Author"), authorEdit_);
    form->addRow(QStringLiteral("Source"), sourceEdit_);
    form->addRow(QStringLiteral("Tags"), tagsEdit_);
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
    englishEdit_->setPlaceholderText(QStringLiteral("English text..."));
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

void PrayerEditDialog::setPrayer(const Prayer& prayer)
{
    base_ = prayer;
    titleEdit_->setText(prayer.title);
    categoryEdit_->setText(prayer.category);
    authorEdit_->setText(prayer.author);
    sourceEdit_->setText(prayer.source);
    tagsEdit_->setText(prayer.tags.join(QStringLiteral(", ")));
    latinEdit_->setPlainText(prayer.latin);
    englishEdit_->setPlainText(prayer.english);
    notesEdit_->setPlainText(prayer.notes);
}

Prayer PrayerEditDialog::prayer() const
{
    Prayer p = base_;
    p.title = titleEdit_->text().trimmed();
    p.category = categoryEdit_->text().trimmed();
    p.author = authorEdit_->text().trimmed();
    p.source = sourceEdit_->text().trimmed();
    p.tags.clear();
    const QStringList parts = tagsEdit_->text().split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString& part : parts) p.tags.append(part.trimmed());
    p.latin = latinEdit_->toPlainText().trimmed();
    p.english = englishEdit_->toPlainText().trimmed();
    p.notes = notesEdit_->toPlainText().trimmed();
    return p;
}

Prayer PrayerEditDialog::editPrayer(QWidget* parent, const Prayer& prayer)
{
    PrayerEditDialog dialog(parent);
    dialog.setPrayer(prayer);
    dialog.setWindowTitle(QStringLiteral("Edit Prayer"));
    if (dialog.exec() != QDialog::Accepted) return Prayer();
    Prayer result = dialog.prayer();
    return result.isValid() ? result : Prayer();
}

Prayer PrayerEditDialog::newPrayer(QWidget* parent)
{
    PrayerEditDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("New Prayer"));
    if (dialog.exec() != QDialog::Accepted) return Prayer();
    Prayer result = dialog.prayer();
    return result.isValid() ? result : Prayer();
}
