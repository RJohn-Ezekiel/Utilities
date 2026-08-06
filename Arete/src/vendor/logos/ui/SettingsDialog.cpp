#include "SettingsDialog.h"
#include "Theme.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

#include <QFontDatabase>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setMinimumWidth(420);
    setupUI();
}

void SettingsDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // ── Appearance ──
    auto* appearanceLabel = new QLabel("Appearance");
    appearanceLabel->setStyleSheet(QStringLiteral(
        "font-size:11px; font-weight:bold; color:%1; letter-spacing:1px; text-transform:uppercase;"
    ).arg(Theme::secondaryText.name()));

    auto* form = new QFormLayout;
    form->setSpacing(8);
    form->setContentsMargins(0, 4, 0, 0);

    fontFamily_ = new QFontComboBox;
    fontFamily_->setFontFilters(QFontComboBox::MonospacedFonts);

    fontSize_ = new QSpinBox;
    fontSize_->setRange(8, 48);
    fontSize_->setValue(14);

    lineSpacing_ = new QDoubleSpinBox;
    lineSpacing_->setRange(1.0, 3.0);
    lineSpacing_->setSingleStep(0.1);
    lineSpacing_->setValue(1.8);

    form->addRow("Font:", fontFamily_);
    form->addRow("Size:", fontSize_);
    form->addRow("Line Spacing:", lineSpacing_);

    // ── Behaviour ──
    auto* behaviourLabel = new QLabel("Behaviour");
    behaviourLabel->setStyleSheet(QStringLiteral(
        "font-size:11px; font-weight:bold; color:%1; letter-spacing:1px; text-transform:uppercase; margin-top:8px;"
    ).arg(Theme::secondaryText.name()));

    rememberLast_ = new QCheckBox("Remember last opened passage on quit");
    rememberLast_->setChecked(true);

    auto* clearRow = new QHBoxLayout;
    clearHistoryBtn_ = new QPushButton("Clear History");
    clearHistoryBtn_->setStyleSheet(QStringLiteral(
        "QPushButton { color:%1; } QPushButton:hover { color:%2; }"
    ).arg(Theme::secondaryText.name(), Theme::primaryText.name()));
    clearRow->addWidget(clearHistoryBtn_);
    clearRow->addStretch();

    mainLayout->addWidget(appearanceLabel);
    mainLayout->addLayout(form);
    mainLayout->addWidget(behaviourLabel);
    mainLayout->addWidget(rememberLast_);
    mainLayout->addLayout(clearRow);
    mainLayout->addStretch();

    // ── Buttons ──
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        emit settingsChanged();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(clearHistoryBtn_, &QPushButton::clicked, this, [this]() {
        clearHistory_ = true;
    });
}

std::string SettingsDialog::fontFamily() const
{
    return fontFamily_->currentFont().family().toStdString();
}

int SettingsDialog::fontSize() const
{
    return fontSize_->value();
}

double SettingsDialog::lineSpacing() const
{
    return lineSpacing_->value();
}

bool SettingsDialog::rememberLastPassage() const
{
    return rememberLast_->isChecked();
}

void SettingsDialog::setFontFamily(const std::string& family)
{
    fontFamily_->setCurrentFont(QFont(QString::fromStdString(family)));
}

void SettingsDialog::setFontSize(int size)
{
    fontSize_->setValue(size);
}

void SettingsDialog::setLineSpacing(double spacing)
{
    lineSpacing_->setValue(spacing);
}

void SettingsDialog::setRememberLastPassage(bool remember)
{
    rememberLast_->setChecked(remember);
}
