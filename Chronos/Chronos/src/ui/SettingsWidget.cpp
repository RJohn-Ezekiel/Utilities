#include "SettingsWidget.h"
#include "Theme.h"
#include "services/TimerService.h"
#include "services/ReminderScheduler.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QCoreApplication>

namespace chronos {

SettingsWidget::SettingsWidget(StorageManager* storage, TimerService* timerService,
                               ReminderScheduler* reminderScheduler,
                               QWidget* parent)
    : QWidget(parent)
    , m_storage(storage)
    , m_timerService(timerService)
    , m_reminderScheduler(reminderScheduler)
{
    setupUi();
    loadSettings();
}

void SettingsWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    auto* header = new QLabel(QStringLiteral("Settings"), this);
    header->setStyleSheet(QStringLiteral("color: %1; font-size: 16px; font-weight: bold;")
                              .arg(Theme::PrimaryText.name()));
    root->addWidget(header);

    auto addSpin = [&](const QString& label, int min, int max, int def, int step) -> QSpinBox* {
        auto* spin = new QSpinBox(this);
        spin->setRange(min, max);
        spin->setValue(def);
        spin->setSingleStep(step);
        spin->setFixedWidth(250);
        spin->setStyleSheet(QStringLiteral(
            "QSpinBox { background: %1; color: %2; border: 1px solid %3;"
            "  padding: 5px; font-size: 10px; }"
            "QSpinBox::up-button, QSpinBox::down-button {"
            "  width: 18px; background: %4; border: none; }"
        ).arg(Theme::Card.name()).arg(Theme::PrimaryText.name())
         .arg(Theme::Border.name()).arg(Theme::Panel.name()));
        if (auto* le = spin->findChild<QLineEdit*>()) {
            le->setReadOnly(true);
            le->setStyleSheet(QStringLiteral(
                "background: transparent; color: %1; border: none;"
            ).arg(Theme::PrimaryText.name()));
        }
        return spin;
    };

    const QString groupStyle = QStringLiteral(
        "QGroupBox { color: %1; font-size: 13px; font-weight: bold;"
        "  border: 1px solid %2; margin-top: 10px; padding: 18px 12px 12px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 6px; }"
        "QGroupBox QLabel { color: %3; font-size: 14px; }"
    ).arg(Theme::PrimaryText.name()).arg(Theme::Border.name())
     .arg(Theme::SecondaryText.name());

    // ── Timer group ──
    auto* timerGroup = new QGroupBox(QStringLiteral("Timer Durations"), this);
    timerGroup->setStyleSheet(groupStyle);
    auto* timerForm = new QFormLayout(timerGroup);
    timerForm->setLabelAlignment(Qt::AlignRight);
    timerForm->setSpacing(8);

    m_focusMin = addSpin(QString(), 1, 180, 25, 5);
    timerForm->addRow(QStringLiteral("Focus (min):"), m_focusMin);

    m_shortBreakMin = addSpin(QString(), 1, 60, 5, 1);
    timerForm->addRow(QStringLiteral("Short Break (min):"), m_shortBreakMin);

    m_longBreakMin = addSpin(QString(), 1, 120, 15, 5);
    timerForm->addRow(QStringLiteral("Long Break (min):"), m_longBreakMin);

    m_sessionsBeforeLB = addSpin(QString(), 1, 20, 4, 1);
    timerForm->addRow(QStringLiteral("Sessions before LB:"), m_sessionsBeforeLB);
    root->addWidget(timerGroup);

    // ── Reminders group ──
    auto* remindGroup = new QGroupBox(QStringLiteral("Health Reminders"), this);
    remindGroup->setStyleSheet(groupStyle);
    auto* remindForm = new QFormLayout(remindGroup);
    remindForm->setLabelAlignment(Qt::AlignRight);
    remindForm->setSpacing(8);

    m_waterMin = addSpin(QString(), 1, 180, 30, 5);
    remindForm->addRow(QStringLiteral("Water (min):"), m_waterMin);

    m_standMin = addSpin(QString(), 1, 180, 30, 5);
    remindForm->addRow(QStringLiteral("Stand (min):"), m_standMin);

    m_stretchMin = addSpin(QString(), 1, 180, 30, 5);
    remindForm->addRow(QStringLiteral("Stretch (min):"), m_stretchMin);

    m_eyeMin = addSpin(QString(), 1, 180, 30, 5);
    remindForm->addRow(QStringLiteral("Eye Rest (min):"), m_eyeMin);
    root->addWidget(remindGroup);

    // ── Appearance group ──
    auto* appearanceGroup = new QGroupBox(QStringLiteral("Appearance"), this);
    appearanceGroup->setStyleSheet(groupStyle);
    auto* appearanceForm = new QFormLayout(appearanceGroup);
    appearanceForm->setLabelAlignment(Qt::AlignRight);
    appearanceForm->setSpacing(8);

    m_fontSize = addSpin(QString(), 8, 24, 14, 1);
    appearanceForm->addRow(QStringLiteral("Font Size:"), m_fontSize);
    root->addWidget(appearanceGroup);

    // ── Behaviour group ──
    auto* behaviourGroup = new QGroupBox(QStringLiteral("Behaviour"), this);
    behaviourGroup->setStyleSheet(groupStyle);
    auto* behaviourForm = new QFormLayout(behaviourGroup);
    behaviourForm->setLabelAlignment(Qt::AlignRight);
    behaviourForm->setSpacing(8);

    const QString cbStyle = QStringLiteral(
        "QCheckBox { color: %1; font-size: 14px; spacing: 8px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
    ).arg(Theme::PrimaryText.name());

    m_soundEnabled = new QCheckBox(QStringLiteral("Notification Sound"), this);
    m_soundEnabled->setStyleSheet(cbStyle);
    behaviourForm->addRow(QString(), m_soundEnabled);

    m_alwaysOnTop = new QCheckBox(QStringLiteral("Always on Top"), this);
    m_alwaysOnTop->setStyleSheet(cbStyle);
    behaviourForm->addRow(QString(), m_alwaysOnTop);

    m_launchAtStartup = new QCheckBox(QStringLiteral("Launch at Startup"), this);
    m_launchAtStartup->setStyleSheet(cbStyle);
    behaviourForm->addRow(QString(), m_launchAtStartup);

    m_rememberSize = new QCheckBox(QStringLiteral("Remember Window Size"), this);
    m_rememberSize->setStyleSheet(cbStyle);
    behaviourForm->addRow(QString(), m_rememberSize);

    m_rememberSession = new QCheckBox(QStringLiteral("Remember Session"), this);
    m_rememberSession->setStyleSheet(cbStyle);
    behaviourForm->addRow(QString(), m_rememberSession);
    root->addWidget(behaviourGroup);

    // ── Buttons ──
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();

    const QString btnStyle = QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %3;"
        "  padding: 6px 16px; font-size: 13px; }"
        "QPushButton:hover { background: %4; }"
    ).arg(Theme::Card.name()).arg(Theme::PrimaryText.name())
     .arg(Theme::Border.name()).arg(Theme::Hover.name());

    const QString accentStyle = QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %1;"
        "  padding: 6px 16px; font-size: 13px; }"
        "QPushButton:hover { background: %3; }"
    ).arg(Theme::Accent.name()).arg(Theme::Background.name())
     .arg(Theme::AccentDim.name());

    auto* saveBtn = new QPushButton(QStringLiteral("Save"), this);
    saveBtn->setStyleSheet(accentStyle);
    connect(saveBtn, &QPushButton::clicked, this, &SettingsWidget::saveSettings);
    btnRow->addWidget(saveBtn);

    auto* resetBtn = new QPushButton(QStringLiteral("Reset Defaults"), this);
    resetBtn->setStyleSheet(btnStyle);
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        Settings defaults;
        m_focusMin->setValue(defaults.focusDuration / 60);
        m_shortBreakMin->setValue(defaults.shortBreakDuration / 60);
        m_longBreakMin->setValue(defaults.longBreakDuration / 60);
        m_sessionsBeforeLB->setValue(defaults.sessionsBeforeLongBreak);
        m_waterMin->setValue(defaults.waterReminderInterval / 60);
        m_standMin->setValue(defaults.standReminderInterval / 60);
        m_stretchMin->setValue(defaults.stretchReminderInterval / 60);
        m_eyeMin->setValue(defaults.eyeReminderInterval / 60);
        m_fontSize->setValue(defaults.fontSize);
        m_soundEnabled->setChecked(defaults.notificationSound);
        m_alwaysOnTop->setChecked(defaults.alwaysOnTop);
        m_launchAtStartup->setChecked(defaults.launchAtStartup);
        m_rememberSize->setChecked(defaults.rememberWindowSize);
        m_rememberSession->setChecked(defaults.rememberSession);
    });
    btnRow->addWidget(resetBtn);

    const QString dangerStyle = QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %1;"
        "  padding: 6px 16px; font-size: 12px; }"
        "QPushButton:hover { background: %3; }"
    ).arg(Theme::Error.name()).arg(Theme::PrimaryText.name())
     .arg(Theme::Error.lighter(120).name());

    btnRow->addSpacing(12);
    auto* resetAllBtn = new QPushButton(QStringLiteral("Reset Everything"), this);
    resetAllBtn->setStyleSheet(dangerStyle);
    connect(resetAllBtn, &QPushButton::clicked, this, [this]() {
        emit resetAllRequested();
    });
    btnRow->addWidget(resetAllBtn);

    root->addLayout(btnRow);
    root->addStretch();
}

void SettingsWidget::loadSettings()
{
    Settings s = m_storage->loadSettings();
    m_original = s;

    m_focusMin->setValue(s.focusDuration / 60);
    m_shortBreakMin->setValue(s.shortBreakDuration / 60);
    m_longBreakMin->setValue(s.longBreakDuration / 60);
    m_sessionsBeforeLB->setValue(s.sessionsBeforeLongBreak);
    m_waterMin->setValue(s.waterReminderInterval / 60);
    m_standMin->setValue(s.standReminderInterval / 60);
    m_stretchMin->setValue(s.stretchReminderInterval / 60);
    m_eyeMin->setValue(s.eyeReminderInterval / 60);
    m_fontSize->setValue(s.fontSize);
    m_soundEnabled->setChecked(s.notificationSound);
    m_alwaysOnTop->setChecked(s.alwaysOnTop);
    m_launchAtStartup->setChecked(s.launchAtStartup);
    m_rememberSize->setChecked(s.rememberWindowSize);
    m_rememberSession->setChecked(s.rememberSession);
}

void SettingsWidget::saveSettings()
{
    Settings s;
    s.focusDuration = m_focusMin->value() * 60;
    s.shortBreakDuration = m_shortBreakMin->value() * 60;
    s.longBreakDuration = m_longBreakMin->value() * 60;
    s.sessionsBeforeLongBreak = m_sessionsBeforeLB->value();
    s.waterReminderInterval = m_waterMin->value() * 60;
    s.standReminderInterval = m_standMin->value() * 60;
    s.stretchReminderInterval = m_stretchMin->value() * 60;
    s.eyeReminderInterval = m_eyeMin->value() * 60;
    s.fontSize = m_fontSize->value();
    s.notificationSound = m_soundEnabled->isChecked();
    s.alwaysOnTop = m_alwaysOnTop->isChecked();
    s.launchAtStartup = m_launchAtStartup->isChecked();
    s.rememberWindowSize = m_rememberSize->isChecked();
    s.rememberSession = m_rememberSession->isChecked();

    s.windowWidth = m_original.windowWidth;
    s.windowHeight = m_original.windowHeight;

    m_storage->saveSettings(s);
    m_timerService->reloadSettings();
    m_reminderScheduler->configure(s);
    applyLaunchAtStartup(s.launchAtStartup);

    emit settingsChanged();
}

void SettingsWidget::applyLaunchAtStartup(bool enable)
{
#ifdef Q_OS_LINUX
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                   + QStringLiteral("/autostart/chronos.desktop");
    QDir dir = QFileInfo(path).absoluteDir();
    if (!dir.exists()) dir.mkpath(dir.absolutePath());

    if (enable) {
        QString execPath = QCoreApplication::applicationFilePath();
        QString content = QStringLiteral(
            "[Desktop Entry]\n"
            "Type=Application\n"
            "Name=Chronos\n"
            "Exec=%1\n"
            "Terminal=false\n"
            "X-GNOME-Autostart-enabled=true\n"
        ).arg(execPath);
        QFile f(path);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(content.toUtf8());
            f.close();
        }
    } else {
        QFile::remove(path);
    }
#else
    Q_UNUSED(enable);
#endif
}

} // namespace chronos
