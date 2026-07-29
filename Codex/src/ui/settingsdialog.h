#pragma once

#include "core/types.h"
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>

namespace codex {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const ConfigData &current, QWidget *parent = nullptr);

    ConfigData result() const;

private:
    QLineEdit *m_vaultPath;
    QComboBox *m_mediaInsert;
    QSpinBox *m_autosaveInterval;
    QSpinBox *m_fontSize;
    QCheckBox *m_rememberWindowSize;
    QCheckBox *m_rememberLastNote;
};

} // namespace codex
