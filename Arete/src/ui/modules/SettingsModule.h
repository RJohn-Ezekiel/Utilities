#pragma once

#include "core/Module.h"

#include <QWidget>

class QLineEdit;
class QSpinBox;
class QComboBox;
class QDateEdit;
class QLabel;
class QPushButton;

namespace arete {
namespace app { class AppContext; }
namespace services { class UpdateChecker; }

namespace ui {

// Settings tab: shell preferences persisted through the Arete settings
// manager (INI in the user data directory).
class SettingsModule : public core::Module
{
    Q_OBJECT

public:
    explicit SettingsModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("settings"); }
    QString moduleTitle() const override { return QStringLiteral("Settings"); }
    QString moduleIcon() const override { return QString(); }
    void onActivated() override;
    void refreshView() override;

private:
    QWidget* makeRow(const QString& label, QWidget* field);
    void onUpdateStateChanged();

    QComboBox* m_theme;
    QSpinBox* m_focusMinutes;
    QSpinBox* m_shortBreak;
    QSpinBox* m_longBreak;
    QSpinBox* m_weatherInterval;
    QLineEdit* m_biblesDir;
    QLineEdit* m_vaultPath;
    QSpinBox* m_reminderLead;
    QSpinBox* m_habitHour;
    QLineEdit* m_profileName;
    QDateEdit* m_profileDob;
    QComboBox* m_profileBlood;
    QLineEdit* m_profileAddress;
    QLineEdit* m_profileNotes;

    // Update-to-GitHub self-service widget state.
    services::UpdateChecker* m_updater = nullptr;
    QLabel* m_updateStatus = nullptr;
    QPushButton* m_updateButton = nullptr;
};

} // namespace ui
} // namespace arete
