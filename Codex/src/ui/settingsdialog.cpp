#include "ui/settingsdialog.h"

#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileDialog>
#include <QVBoxLayout>

namespace codex {

SettingsDialog::SettingsDialog(const ConfigData &current, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Settings"));
    setMinimumWidth(450);

    auto *form = new QFormLayout;

    m_vaultPath = new QLineEdit(QString::fromStdString(current.vaultPath.string()), this);
    auto *browseBtn = new QPushButton(QStringLiteral("Browse..."), this);
    auto *pathLayout = new QHBoxLayout;
    pathLayout->addWidget(m_vaultPath);
    pathLayout->addWidget(browseBtn);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        auto dir = QFileDialog::getExistingDirectory(this,
            QStringLiteral("Select Vault Directory"),
            m_vaultPath->text());
        if (!dir.isEmpty())
            m_vaultPath->setText(dir);
    });
    form->addRow(QStringLiteral("Vault Path:"), pathLayout);

    m_mediaInsert = new QComboBox(this);
    m_mediaInsert->addItem(QStringLiteral("Copy to Vault"));
    m_mediaInsert->addItem(QStringLiteral("Relative Path"));
    m_mediaInsert->addItem(QStringLiteral("Absolute Path"));
    m_mediaInsert->setCurrentIndex(static_cast<int>(current.mediaInsert));
    form->addRow(QStringLiteral("Media Insert:"), m_mediaInsert);

    m_autosaveInterval = new QSpinBox(this);
    m_autosaveInterval->setRange(500, 60000);
    m_autosaveInterval->setSingleStep(500);
    m_autosaveInterval->setSuffix(QStringLiteral(" ms"));
    m_autosaveInterval->setValue(current.autosaveIntervalMs);
    form->addRow(QStringLiteral("Autosave Interval:"), m_autosaveInterval);

    m_fontSize = new QSpinBox(this);
    m_fontSize->setRange(8, 32);
    m_fontSize->setValue(current.fontSize);
    form->addRow(QStringLiteral("Font Size:"), m_fontSize);

    m_rememberWindowSize = new QCheckBox(this);
    m_rememberWindowSize->setChecked(current.rememberWindowSize);
    form->addRow(QStringLiteral("Remember Window Size:"), m_rememberWindowSize);

    m_rememberLastNote = new QCheckBox(this);
    m_rememberLastNote->setChecked(current.rememberLastNote);
    form->addRow(QStringLiteral("Remember Last Note:"), m_rememberLastNote);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(form);
    mainLayout->addWidget(buttons);
}

ConfigData SettingsDialog::result() const
{
    ConfigData d;
    d.vaultPath = m_vaultPath->text().toStdString();
    d.mediaInsert = static_cast<MediaInsert>(m_mediaInsert->currentIndex());
    d.autosaveIntervalMs = m_autosaveInterval->value();
    d.fontSize = m_fontSize->value();
    d.rememberWindowSize = m_rememberWindowSize->isChecked();
    d.rememberLastNote = m_rememberLastNote->isChecked();
    return d;
}

} // namespace codex
