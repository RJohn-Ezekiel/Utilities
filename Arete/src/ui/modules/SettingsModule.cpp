#include "ui/modules/SettingsModule.h"
#include "services/WeatherService.h"
#include "services/VerseService.h"
#include "services/UpdateChecker.h"
#include "app/AppContext.h"
#include "arete/settings/SettingsManager.h"
#include "arete/update/UpdateService.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QScrollArea>
#include <QDateEdit>
#include <QMessageBox>
#include <QProcess>
#include <QCoreApplication>

namespace arete::ui {

SettingsModule::SettingsModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(48, 32, 48, 32);
    outer->setSpacing(16);

    auto* title = new QLabel(QStringLiteral("Settings"), this);
    title->setStyleSheet(QStringLiteral("font-size: 22px; color: #E2E2E2; letter-spacing: 1px;"));
    outer->addWidget(title);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; }"));

    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 8, 8, 8);
    layout->setSpacing(6);
    layout->setAlignment(Qt::AlignTop);

    auto* settings = context->settings();
    const QString fieldStyle = QStringLiteral(
        "QLineEdit, QSpinBox, QComboBox, QDateEdit { background-color: #242424; color: #C4C4C4;"
        " border: 1px solid #353535; border-radius: 8px; padding: 6px 10px; font-size: 13px; }"
        "QLineEdit:focus, QSpinBox:focus, QComboBox:focus, QDateEdit:focus { border-color: #4A4A4A; }");

    // ── Updates: fetch the newest release from GitHub and self-update when
    //    there is internet; otherwise stays idle and offline-safe ──
    auto* updateLabel = new QLabel(QStringLiteral("UPDATES"), content);
    updateLabel->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 12px; letter-spacing: 1px;"
                                              " margin-top: 10px;"));
    layout->addWidget(updateLabel);

    auto* versionRow = new QHBoxLayout;
    versionRow->setSpacing(8);
    m_updateStatus = new QLabel(
        QStringLiteral("Arete %1").arg(QStringLiteral(ARETE_APP_VERSION)), content);
    m_updateStatus->setStyleSheet(QStringLiteral("color: #C4C4C4; font-size: 13px;"));
    versionRow->addWidget(m_updateStatus);
    versionRow->addStretch(1);
    layout->addLayout(versionRow);

    m_updateButton = new QPushButton(QStringLiteral("Check for updates"), content);
    m_updateButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #242424; color: #C4C4C4;"
        " border: 1px solid #353535; border-radius: 8px; padding: 6px 16px; font-size: 13px; }"
        "QPushButton:hover { border-color: #4A4A4A; }"
        "QPushButton:disabled { color: #5A5A5A; }"));
    m_updateButton->setFixedHeight(32);
    layout->addWidget(makeRow(QStringLiteral("Self-update"), m_updateButton));

    m_updater = new services::UpdateChecker(
        QStringLiteral(ARETE_GITHUB_REPO), QStringLiteral(ARETE_GITHUB_ASSET),
        QStringLiteral(ARETE_APP_VERSION), this);
    connect(m_updater, &services::UpdateChecker::stateChanged, this,
            [this] { onUpdateStateChanged(); });
    connect(m_updateButton, &QPushButton::clicked, this, [this] {
        if (m_updater->state() == services::UpdateChecker::State::UpdateAvailable) {
            m_updater->install();
        } else {
            m_updater->check();
        }
    });

    // ── Profile (stored locally in Arete.ini; used for greetings and
    //    birthday reminders — nothing leaves the device) ──
    auto* profileLabel = new QLabel(QStringLiteral("PROFILE"), content);
    profileLabel->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 12px; letter-spacing: 1px;"
                                               " margin-top: 10px;"));
    layout->addWidget(profileLabel);

    m_profileName = new QLineEdit(content);
    m_profileName->setPlaceholderText(QStringLiteral("Your name"));
    m_profileName->setText(settings->get(QStringLiteral("profile/name"), QString()));
    m_profileName->setStyleSheet(fieldStyle);
    connect(m_profileName, &QLineEdit::editingFinished, this, [settings, this] {
        settings->set(QStringLiteral("profile/name"), m_profileName->text().trimmed());
        settings->sync();
    });
    layout->addWidget(makeRow(QStringLiteral("Name"), m_profileName));

    m_profileDob = new QDateEdit(content);
    m_profileDob->setCalendarPopup(true);
    m_profileDob->setDisplayFormat(QStringLiteral("MMM d, yyyy"));
    m_profileDob->setDateRange(QDate(1900, 1, 1), QDate::currentDate());
    const QDate dob = QDate::fromString(settings->get(QStringLiteral("profile/dob"), QString()),
                                        Qt::ISODate);
    m_profileDob->setDate(dob.isValid() ? dob : QDate(2000, 1, 1));
    m_profileDob->setStyleSheet(fieldStyle);
    connect(m_profileDob, &QDateEdit::dateChanged, this, [settings](const QDate& date) {
        settings->set(QStringLiteral("profile/dob"), date.toString(Qt::ISODate));
        settings->sync();
    });
    layout->addWidget(makeRow(QStringLiteral("Date of birth"), m_profileDob));

    m_profileBlood = new QComboBox(content);
    const QStringList bloodGroups = {QStringLiteral("A+"), QStringLiteral("A-"),
                                     QStringLiteral("B+"), QStringLiteral("B-"),
                                     QStringLiteral("AB+"), QStringLiteral("AB-"),
                                     QStringLiteral("O+"), QStringLiteral("O-")};
    const QString blood = settings->get(QStringLiteral("profile/bloodGroup"), QString());
    for (const QString& group : bloodGroups) {
        m_profileBlood->addItem(group, group);
        if (group == blood) m_profileBlood->setCurrentIndex(m_profileBlood->count() - 1);
    }
    m_profileBlood->setStyleSheet(fieldStyle);
    connect(m_profileBlood, &QComboBox::currentIndexChanged, this, [settings, this](int index) {
        settings->set(QStringLiteral("profile/bloodGroup"),
                      m_profileBlood->itemData(index).toString());
        settings->sync();
    });
    layout->addWidget(makeRow(QStringLiteral("Blood group"), m_profileBlood));

    m_profileAddress = new QLineEdit(content);
    m_profileAddress->setPlaceholderText(QStringLiteral("Address"));
    m_profileAddress->setText(settings->get(QStringLiteral("profile/address"), QString()));
    m_profileAddress->setStyleSheet(fieldStyle);
    connect(m_profileAddress, &QLineEdit::editingFinished, this, [settings, this] {
        settings->set(QStringLiteral("profile/address"), m_profileAddress->text().trimmed());
        settings->sync();
    });
    layout->addWidget(makeRow(QStringLiteral("Address"), m_profileAddress));

    m_profileNotes = new QLineEdit(content);
    m_profileNotes->setPlaceholderText(QStringLiteral("Anything else — emergency contact, allergies\u2026"));
    m_profileNotes->setText(settings->get(QStringLiteral("profile/notes"), QString()));
    m_profileNotes->setStyleSheet(fieldStyle);
    connect(m_profileNotes, &QLineEdit::editingFinished, this, [settings, this] {
        settings->set(QStringLiteral("profile/notes"), m_profileNotes->text().trimmed());
        settings->sync();
    });
    layout->addWidget(makeRow(QStringLiteral("Notes"), m_profileNotes));

    auto* localNote = new QLabel(QStringLiteral("All profile data stays on this device."), content);
    localNote->setStyleSheet(QStringLiteral("color: #6A6A6A; font-size: 11px;"));
    layout->addWidget(localNote);

    // ── Theme ──
    m_theme = new QComboBox(content);
    m_theme->addItem(QStringLiteral("Dark (default)"), QStringLiteral("dark"));
    m_theme->addItem(QStringLiteral("Light"), QStringLiteral("light"));
    m_theme->setStyleSheet(fieldStyle);
    m_theme->setCurrentIndex(settings->get(QStringLiteral("ui/theme"), QStringLiteral("dark"))
                                  == QLatin1String("dark") ? 0 : 1);
    connect(m_theme, &QComboBox::currentIndexChanged, this, [settings](int index) {
        settings->set(QStringLiteral("ui/theme"),
                      index == 0 ? QStringLiteral("dark") : QStringLiteral("light"));
        settings->sync();
    });
    layout->addWidget(makeRow(QStringLiteral("Theme"), m_theme));

    // ── Focus durations (Chronos) ──
    m_focusMinutes = new QSpinBox(content);
    m_focusMinutes->setRange(5, 180);
    m_focusMinutes->setValue(settings->get(QStringLiteral("chronos/focusMinutes"), 25));
    m_focusMinutes->setStyleSheet(fieldStyle);
    connect(m_focusMinutes, &QSpinBox::valueChanged, this, [settings](int value) {
        settings->set(QStringLiteral("chronos/focusMinutes"), value);
        settings->sync();
    });
    layout->addWidget(makeRow(QStringLiteral("Focus session (min)"), m_focusMinutes));

    m_shortBreak = new QSpinBox(content);
    m_shortBreak->setRange(1, 60);
    m_shortBreak->setValue(settings->get(QStringLiteral("chronos/shortBreak"), 5));
    m_shortBreak->setStyleSheet(fieldStyle);
    connect(m_shortBreak, &QSpinBox::valueChanged, this, [settings](int value) {
        settings->set(QStringLiteral("chronos/shortBreak"), value);
        settings->sync();
    });
    layout->addWidget(makeRow(QStringLiteral("Short break (min)"), m_shortBreak));

    m_longBreak = new QSpinBox(content);
    m_longBreak->setRange(1, 120);
    m_longBreak->setValue(settings->get(QStringLiteral("chronos/longBreak"), 15));
    m_longBreak->setStyleSheet(fieldStyle);
    connect(m_longBreak, &QSpinBox::valueChanged, this, [settings](int value) {
        settings->set(QStringLiteral("chronos/longBreak"), value);
        settings->sync();
    });
    layout->addWidget(makeRow(QStringLiteral("Long break (min)"), m_longBreak));

    // ── Weather refresh ──
    m_weatherInterval = new QSpinBox(content);
    m_weatherInterval->setRange(10, 240);
    m_weatherInterval->setValue(settings->get(QStringLiteral("weather/intervalMinutes"), 30));
    m_weatherInterval->setStyleSheet(fieldStyle);
    connect(m_weatherInterval, &QSpinBox::valueChanged, this, [this, settings, context](int value) {
        settings->set(QStringLiteral("weather/intervalMinutes"), value);
        settings->sync();
        context->weather()->refresh();
    });
    layout->addWidget(makeRow(QStringLiteral("Weather refresh (min)"), m_weatherInterval));

    // ── Logos bibles directory ──
    m_biblesDir = new QLineEdit(content);
    m_biblesDir->setPlaceholderText(QStringLiteral("Folder containing kjv.json / vulg.json"));
    m_biblesDir->setText(settings->get(QStringLiteral("logos/biblesDir"), QString()));
    m_biblesDir->setStyleSheet(fieldStyle);
    connect(m_biblesDir, &QLineEdit::editingFinished, this, [this, settings, context] {
        settings->set(QStringLiteral("logos/biblesDir"), m_biblesDir->text().trimmed());
        settings->sync();
        context->verse()->ensureLoaded();
    });
    layout->addWidget(makeRow(QStringLiteral("Bibles directory"), m_biblesDir));

    // ── Codex vault ──
    m_vaultPath = new QLineEdit(content);
    m_vaultPath->setPlaceholderText(QStringLiteral("Markdown vault root"));
    m_vaultPath->setText(settings->get(QStringLiteral("codex/vaultPath"), QString()));
    m_vaultPath->setStyleSheet(fieldStyle);
    connect(m_vaultPath, &QLineEdit::editingFinished, this, [this, settings] {
        settings->set(QStringLiteral("codex/vaultPath"), m_vaultPath->text().trimmed());
        settings->sync();
    });
    layout->addWidget(makeRow(QStringLiteral("Codex vault"), m_vaultPath));

    // ── Reminders (user choice: every category can be switched off) ──
    auto* reminderLabel = new QLabel(QStringLiteral("REMINDERS"), content);
    reminderLabel->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 12px; letter-spacing: 1px;"
                                                " margin-top: 10px;"));
    layout->addWidget(reminderLabel);

    auto toggle = [this, content, settings, fieldStyle](const QString& key, bool def) {
        auto* box = new QComboBox(content);
        box->addItem(QStringLiteral("On"), true);
        box->addItem(QStringLiteral("Off"), false);
        box->setStyleSheet(fieldStyle);
        box->setCurrentIndex(settings->get(key, def) ? 0 : 1);
        connect(box, &QComboBox::currentIndexChanged, this, [settings, key](int index) {
            settings->set(key, index == 0);
            settings->sync();
        });
        return box;
    };

    layout->addWidget(makeRow(QStringLiteral("Reminders enabled"), toggle(QStringLiteral("reminders/enabled"), true)));
    layout->addWidget(makeRow(QStringLiteral("Notify for events"), toggle(QStringLiteral("reminders/events"), true)));
    layout->addWidget(makeRow(QStringLiteral("Notify for tasks"), toggle(QStringLiteral("reminders/tasks"), true)));
    layout->addWidget(makeRow(QStringLiteral("Daily habit reminder"), toggle(QStringLiteral("reminders/habits"), true)));
    layout->addWidget(makeRow(QStringLiteral("Desktop notifications"), toggle(QStringLiteral("notifications/system"), true)));

    m_reminderLead = new QSpinBox(content);
    m_reminderLead->setRange(5, 120);
    m_reminderLead->setValue(settings->get(QStringLiteral("reminders/leadMinutes"), 15));
    m_reminderLead->setStyleSheet(fieldStyle);
    connect(m_reminderLead, &QSpinBox::valueChanged, this, [settings](int value) {
        settings->set(QStringLiteral("reminders/leadMinutes"), value);
        settings->sync();
    });
    layout->addWidget(makeRow(QStringLiteral("Remind before (min)"), m_reminderLead));

    m_habitHour = new QSpinBox(content);
    m_habitHour->setRange(0, 23);
    m_habitHour->setValue(settings->get(QStringLiteral("reminders/habitHour"), 20));
    m_habitHour->setStyleSheet(fieldStyle);
    connect(m_habitHour, &QSpinBox::valueChanged, this, [settings](int value) {
        settings->set(QStringLiteral("reminders/habitHour"), value);
        settings->sync();
    });
    layout->addWidget(makeRow(QStringLiteral("Habit reminder hour"), m_habitHour));

    scroll->setWidget(content);
    outer->addWidget(scroll, 1);
}

QWidget* SettingsModule::makeRow(const QString& label, QWidget* field)
{
    auto* row = new QWidget(this);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 6, 0, 6);
    auto* text = new QLabel(label, row);
    text->setStyleSheet(QStringLiteral("color: #A0A0A0; font-size: 13px;"));
    layout->addWidget(text);
    layout->addStretch(1);
    field->setParent(row);
    field->setFixedWidth(220);
    layout->addWidget(field);
    return row;
}

void SettingsModule::onActivated()
{
}

void SettingsModule::refreshView()
{
}

void SettingsModule::onUpdateStateChanged()
{
    using services::UpdateChecker;
    switch (m_updater->state()) {
    case UpdateChecker::State::Checking:
        m_updateStatus->setText(QStringLiteral("Checking for updates\u2026"));
        m_updateButton->setText(QStringLiteral("Check for updates"));
        m_updateButton->setEnabled(false);
        break;
    case UpdateChecker::State::Offline:
        m_updateStatus->setText(QStringLiteral("Offline \u2014 cannot reach GitHub"));
        m_updateButton->setText(QStringLiteral("Check for updates"));
        m_updateButton->setEnabled(true);
        break;
    case UpdateChecker::State::UpToDate:
        m_updateStatus->setText(QStringLiteral("You are up to date (v%1)")
                                    .arg(m_updater->latestVersion()));
        m_updateButton->setText(QStringLiteral("Check for updates"));
        m_updateButton->setEnabled(true);
        break;
    case UpdateChecker::State::UpdateAvailable:
        m_updateStatus->setText(QStringLiteral("Arete v%1 available")
                                    .arg(m_updater->latestVersion()));
        m_updateButton->setText(QStringLiteral("Update now"));
        m_updateButton->setEnabled(true);
        break;
    case UpdateChecker::State::Downloading:
        m_updateStatus->setText(QStringLiteral("Downloading update\u2026"));
        m_updateButton->setEnabled(false);
        break;
    case UpdateChecker::State::ReadyToRestart: {
        m_updateStatus->setText(QStringLiteral("Updated to v%1 \u2014 restart to finish")
                                    .arg(m_updater->latestVersion()));
        m_updateButton->setEnabled(true);
        const auto choice = QMessageBox::question(
            this, QStringLiteral("Update installed"),
            QStringLiteral("Arete v%1 was installed. Restart now?")
                .arg(m_updater->latestVersion()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (choice == QMessageBox::Yes) {
            QProcess::startDetached(
                arete::update::UpdateService::installDir() + QStringLiteral("/arete"));
            QCoreApplication::quit();
        }
        break;
    }
    case UpdateChecker::State::Failed:
        m_updateStatus->setText(QStringLiteral("Update failed: %1").arg(m_updater->error()));
        m_updateButton->setText(QStringLiteral("Try again"));
        m_updateButton->setEnabled(true);
        break;
    case UpdateChecker::State::Idle:
        break;
    }
}

} // namespace arete::ui
