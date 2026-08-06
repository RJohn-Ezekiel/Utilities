#include "ui/modules/HomeModule.h"
#include "app/AppContext.h"
#include "data/TaskRepository.h"
#include "data/HabitRepository.h"
#include "data/CheckinRepository.h"
#include "services/TimerService.h"
#include "services/WeatherService.h"
#include "services/VerseService.h"
#include "models/Task.h"
#include "models/Habit.h"
#include "models/Checkin.h"
#include "ui/common/SectionLabel.h"
#include "ui/common/TaskRowWidget.h"
#include "ui/common/TaskToggle.h"
#include "ui/MainWindow.h"
#include "arete/notifications/NotificationCenter.h"
#include "arete/settings/SettingsManager.h"
#include "core/Icons.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollArea>
#include <QPushButton>
#include <QSet>
#include <QDateTime>

namespace arete::ui {

namespace {
const QString kNoTask = QStringLiteral("No active task. Enjoy the quiet.");
constexpr int kWeeklyFocusTarget = 360; // 6h of deep work per week

QString fmtMin(int minutes)
{
    if (minutes < 60) return QStringLiteral("%1m").arg(minutes);
    return QStringLiteral("%1h %2m").arg(minutes / 60).arg(minutes % 60);
}
} // namespace

HomeModule::HomeModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; }"));

    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(48, 32, 48, 32);
    layout->setSpacing(20);
    layout->setAlignment(Qt::AlignTop);

    // ── Header: greeting · date ──
    m_greeting = new QLabel(QStringLiteral("Summary"), content);
    m_greeting->setStyleSheet(QStringLiteral("font-size: 26px; color: #E2E2E2; letter-spacing: 1px;"));
    layout->addWidget(m_greeting);

    m_birthdayLabel = new QLabel(content);
    m_birthdayLabel->setVisible(false);
    m_birthdayLabel->setStyleSheet(QStringLiteral(
        "QLabel { color: #181818; background-color: #D0D0D0; border-radius: 8px;"
        " padding: 8px 14px; font-size: 14px; font-weight: 600; }"));
    layout->addWidget(m_birthdayLabel);

    m_dateLabel = new QLabel(content);
    m_dateLabel->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 13px;"));
    layout->addWidget(m_dateLabel);

    // ── KPI band ──
    auto* kpiRow = new QHBoxLayout;
    kpiRow->setSpacing(16);

    struct KpiSpec { const char* label; QLabel** value; QProgressBar** bar; };
    const QList<KpiSpec> specs = {
        { "FOCUS", &m_focusValue, &m_focusBar },
        { "ISSUES", &m_issuesValue, &m_issuesBar },
        { "HABITS", &m_habitsValue, &m_habitsBar },
        { "MOMENTUM", &m_momentumValue, &m_momentumBar },
    };
    for (const KpiSpec& spec : specs) {
        auto* card = new QWidget(content);
        card->setStyleSheet(QStringLiteral("QWidget { background-color: #1C1C1C;"
                                           " border: 1px solid #2A2A2A; border-radius: 12px; }"));
        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 14, 16, 14);
        cardLayout->setSpacing(8);

        auto* label = new QLabel(QString::fromLatin1(spec.label), card);
        label->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 11px; letter-spacing: 1px;"));
        cardLayout->addWidget(label);

        auto* value = new QLabel(card);
        value->setStyleSheet(QStringLiteral("color: #D6D6D6; font-size: 17px;"));
        cardLayout->addWidget(value);

        auto* bar = new QProgressBar(card);
        bar->setRange(0, 100);
        bar->setTextVisible(false);
        bar->setFixedHeight(4);
        bar->setStyleSheet(QStringLiteral(
            "QProgressBar { background: #262626; border: none; border-radius: 2px; }"
            "QProgressBar::chunk { background: #7A7A7A; border-radius: 2px; }"));
        cardLayout->addWidget(bar);

        *spec.value = value;
        *spec.bar = bar;
        kpiRow->addWidget(card, 1);
    }
    layout->addLayout(kpiRow);

    // ── Current task + focus ──
    layout->addWidget(new common::SectionLabel(QStringLiteral("CURRENT TASK"), content));

    m_currentTaskTitle = new QLabel(kNoTask, content);
    m_currentTaskTitle->setWordWrap(true);
    m_currentTaskTitle->setStyleSheet(QStringLiteral("font-size: 18px; color: #C4C4C4;"));
    layout->addWidget(m_currentTaskTitle);

    m_progress = new QProgressBar(content);
    m_progress->setRange(0, 100);
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(6);
    layout->addWidget(m_progress);

    m_startFocus = new QPushButton(QStringLiteral("START FOCUS"), content);
    m_startFocus->setProperty("primary", true);
    m_startFocus->setFixedHeight(42);
    m_startFocus->setCursor(Qt::PointingHandCursor);
    m_startFocus->setStyleSheet(QStringLiteral(
        "QPushButton { font-size: 13px; letter-spacing: 2px; border-radius: 10px; }"));
    connect(m_startFocus, &QPushButton::clicked, this, &HomeModule::startFocusOnCurrent);
    layout->addWidget(m_startFocus, 0, Qt::AlignLeft);

    // ── Agenda | Signals ──
    auto* columns = new QHBoxLayout;
    columns->setSpacing(24);

    auto* agendaBox = new QVBoxLayout;
    agendaBox->addWidget(new common::SectionLabel(QStringLiteral("AGENDA"), content));
    m_agendaList = new QListWidget(content);
    m_agendaList->setFrameShape(QFrame::NoFrame);
    m_agendaList->setSelectionMode(QAbstractItemView::NoSelection);
    m_agendaList->setFocusPolicy(Qt::NoFocus);
    m_agendaList->setStyleSheet(QStringLiteral(
        "QListWidget { background: transparent; outline: none; }"
        "QListWidget::item { padding: 5px 0; border-bottom: 1px solid #222222;"
        " color: #A0A0A0; font-size: 13px; }"));
    agendaBox->addWidget(m_agendaList, 1);
    columns->addLayout(agendaBox, 3);

    auto* signalsBox = new QVBoxLayout;
    signalsBox->addWidget(new common::SectionLabel(QStringLiteral("SIGNALS"), content));
    m_signalsList = new QListWidget(content);
    m_signalsList->setFrameShape(QFrame::NoFrame);
    m_signalsList->setSelectionMode(QAbstractItemView::NoSelection);
    m_signalsList->setFocusPolicy(Qt::NoFocus);
    m_signalsList->setStyleSheet(QStringLiteral(
        "QListWidget { background: transparent; outline: none; }"
        "QListWidget::item { padding: 5px 0; border-bottom: 1px solid #222222;"
        " color: #A0A0A0; font-size: 13px; }"));
    signalsBox->addWidget(m_signalsList, 1);

    // Full daily scripture, word-wrapped and clickable into Logos.
    m_verseLabel = new QLabel(content);
    m_verseLabel->setWordWrap(true);
    m_verseLabel->setTextFormat(Qt::RichText);
    m_verseLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    m_verseLabel->setOpenExternalLinks(false);
    m_verseLabel->setCursor(Qt::PointingHandCursor);
    m_verseLabel->setStyleSheet(QStringLiteral(
        "QLabel { color: #A0A0A0; background-color: #1C1C1C; border: 1px solid #2A2A2A;"
        " border-radius: 12px; padding: 14px; font-size: 14px; }"));
    connect(m_verseLabel, &QLabel::linkActivated, this, [this, context] {
        if (!m_lastVerse.valid) return;
        QString ref = QStringLiteral("%1 %2:%3")
                          .arg(m_lastVerse.book).arg(m_lastVerse.chapter)
                          .arg(m_lastVerse.verseStart);
        if (m_lastVerse.verseEnd > m_lastVerse.verseStart) {
            ref += QStringLiteral("-%1").arg(m_lastVerse.verseEnd);
        }
        context->mainWindow()->activateModule(QStringLiteral("logos"));
        context->mainWindow()->sendCommandToModule(QStringLiteral("logos"),
                                                   QStringLiteral("logos:") + ref);
    });
    signalsBox->addWidget(m_verseLabel);

    columns->addLayout(signalsBox, 2);

    layout->addLayout(columns, 1);

    m_scroll->setWidget(content);
    outer->addWidget(m_scroll);
}

void HomeModule::onActivated()
{
    refresh();
}

void HomeModule::refreshView()
{
    refresh();
}

void HomeModule::refresh()
{
    refreshGreeting();
    m_dateLabel->setText(QStringLiteral("For %1  \u00B7  %2")
                             .arg(QDate::currentDate().toString(QStringLiteral("dddd, MMM d")))
                             .arg(QDate::currentDate().toString(QStringLiteral("yyyy"))));
    refreshKpi();
    refreshCurrentTask();
    refreshAgenda();
    refreshSignals();
}

void HomeModule::refreshGreeting()
{
    const QString name = context()->settings()->get(QStringLiteral("profile/name"), QString());

    const int hour = QTime::currentTime().hour();
    const QString part = hour < 12 ? QStringLiteral("Good morning")
                         : hour < 17 ? QStringLiteral("Good afternoon")
                                     : QStringLiteral("Good evening");
    m_greeting->setText(name.isEmpty() ? QStringLiteral("Summary")
                                       : QStringLiteral("%1, %2").arg(part, name));

    // Birthday banner + a one-per-day notification.
    const QDate today = QDate::currentDate();
    const QDate dob = QDate::fromString(
        context()->settings()->get(QStringLiteral("profile/dob"), QString()), Qt::ISODate);
    const bool isBirthday = dob.isValid() && dob.month() == today.month() && dob.day() == today.day();
    m_birthdayLabel->setVisible(isBirthday);
    if (isBirthday) {
        m_birthdayLabel->setText(name.isEmpty()
                                     ? QStringLiteral("Happy birthday! Today is yours.")
                                     : QStringLiteral("Happy birthday, %1! Today is yours.").arg(name));
        static QDate lastNotified;
        if (lastNotified != today) {
            lastNotified = today;
            arete::notifications::NotificationCenter::instance().showInfo(
                QStringLiteral("Happy birthday!"),
                name.isEmpty() ? QStringLiteral("Today is yours.") : QStringLiteral("Happy birthday, %1!").arg(name));
        }
    }
}

void HomeModule::refreshKpi()
{
    auto* timer = context()->timer();

    // Focus: today's minutes; bar vs the 6h weekly target.
    m_focusValue->setText(QStringLiteral("%1  today").arg(fmtMin(timer->focusMinutesToday())));
    const int weekly = timer->weeklyFocusMinutes();
    m_focusBar->setValue(qMin(100, qRound(weekly * 100.0 / kWeeklyFocusTarget)));
    m_focusBar->setToolTip(QStringLiteral("Week: %1 / 6h").arg(fmtMin(weekly)));

    // Issues: due today, resolved = completed.
    int total = 0;
    int resolved = 0;
    const auto tasks = context()->tasks()->active();
    for (const models::Task& t : tasks) {
        if (!t.dueAt.isValid() || t.dueAt.date() != QDate::currentDate()) continue;
        ++total;
        if (t.status == models::TaskStatus::Completed || t.status == models::TaskStatus::Archived) {
            ++resolved;
        }
    }
    m_issuesValue->setText(QStringLiteral("%1 / %2 resolved").arg(resolved).arg(total));
    m_issuesBar->setValue(total > 0 ? qRound(resolved * 100.0 / total) : 0);

    // Habits: done today.
    const QVector<models::Habit> habits = context()->habits()->all();
    int done = 0;
    for (const models::Habit& h : habits) {
        if (h.doneToday) ++done;
    }
    m_habitsValue->setText(QStringLiteral("%1 / %2 done").arg(done).arg(habits.size()));
    m_habitsBar->setValue(habits.isEmpty() ? 0 : qRound(done * 100.0 / habits.size()));

    // Momentum: focus streak; bar vs check-in streak (14-day cap).
    const QSet<QDate> checkinDates = context()->checkins()->allDates();
    int checkInStreak = 0;
    for (QDate d = QDate::currentDate(); checkinDates.contains(d); d = d.addDays(-1)) {
        ++checkInStreak;
    }
    m_momentumValue->setText(QStringLiteral("%1-day focus streak").arg(timer->focusStreak()));
    m_momentumBar->setValue(qMin(100, qRound(checkInStreak * 100.0 / 14)));
    m_momentumBar->setToolTip(QStringLiteral("Check-in streak: %1 days").arg(checkInStreak));
}

void HomeModule::refreshCurrentTask()
{
    const auto tasks = context()->tasks()->active();
    models::Task current;
    for (const models::Task& t : tasks) {
        if (t.isDueToday() || !t.dueAt.isValid()) {
            if (current.id < 0 || t.priority > current.priority) {
                current = t;
            }
        }
    }
    if (current.id < 0 && !tasks.isEmpty()) current = tasks.first();

    if (current.id < 0) {
        m_currentTaskTitle->setText(kNoTask);
        m_progress->setValue(0);
        m_startFocus->setEnabled(false);
        return;
    }

    m_currentTaskTitle->setText(current.title);
    m_progress->setValue(qRound(current.completion() * 100.0));
    m_startFocus->setEnabled(true);
    m_startFocus->setText(QStringLiteral("START FOCUS \u00B7 %1").arg(current.title.left(24)));
}

void HomeModule::refreshAgenda()
{
    m_agendaList->clear();
    const QDate today = QDate::currentDate();

    auto* issuesHeader = new QListWidgetItem(QStringLiteral("Issues"), m_agendaList);
    issuesHeader->setForeground(QColor(QStringLiteral("#9A9A9A")));

    const auto tasks = context()->tasks()->active();
    bool anyIssue = false;
    for (const models::Task& t : tasks) {
        if (!t.dueAt.isValid() || t.dueAt.date() != today) continue;
        anyIssue = true;
        auto* item = new QListWidgetItem(m_agendaList);
        item->setData(Qt::UserRole, t.id);
        item->setSizeHint(QSize(0, 40));
        auto* row = new common::TaskRowWidget(t, m_agendaList);
        connect(row, &common::TaskRowWidget::toggled, this, [this](qint64 id, bool completed) {
            models::Task task = context()->tasks()->byId(id);
            if (task.id < 0) return;
            task.status = completed ? models::TaskStatus::Completed : models::TaskStatus::Active;
            task.completedAt = completed ? QDateTime::currentDateTime() : QDateTime();
            context()->tasks()->update(task);
            if (completed) context()->mainWindow()->showTaskCompletion(id, task.title);
            refreshAgenda();
            refreshKpi();
        });
        connect(row, &common::TaskRowWidget::focusRequested, this,
                [this](qint64 id, const QString& title) {
                    context()->timer()->startFocus(id, title);
                    context()->mainWindow()->activateModule(QStringLiteral("chronos"));
                });
        m_agendaList->setItemWidget(item, row);
    }
    if (!anyIssue) {
        auto* item = new QListWidgetItem(QStringLiteral("No issues scheduled."), m_agendaList);
        item->setForeground(QColor(QStringLiteral("#7A7A7A")));
    }

    auto* habitsHeader = new QListWidgetItem(QStringLiteral("Habits"), m_agendaList);
    habitsHeader->setForeground(QColor(QStringLiteral("#9A9A9A")));

    const QVector<models::Habit> habits = context()->habits()->all();
    if (habits.isEmpty()) {
        auto* item = new QListWidgetItem(QStringLiteral("No habits due."), m_agendaList);
        item->setForeground(QColor(QStringLiteral("#7A7A7A")));
    } else {
        for (const models::Habit& h : habits) {
            auto* item = new QListWidgetItem(m_agendaList);
            item->setData(Qt::UserRole, h.id);
            item->setSizeHint(QSize(0, 34));

            auto* widget = new QWidget(m_agendaList);
            auto* layout = new QHBoxLayout(widget);
            layout->setContentsMargins(4, 0, 4, 0);
            layout->setSpacing(10);
            auto* toggle = new common::TaskToggle(widget);
            toggle->setChecked(h.doneToday);
            toggle->setToolTip(QStringLiteral("Mark done for today"));
            connect(toggle, &common::TaskToggle::toggled, this, [this, h](bool) {
                context()->habits()->toggle(h.id, QDate::currentDate());
                refreshAgenda();
                refreshKpi();
            });
            layout->addWidget(toggle);
            auto* name = new QLabel(h.name, widget);
            name->setStyleSheet(QStringLiteral("font-size: 13px; color: %1;")
                                    .arg(h.doneToday ? QStringLiteral("#5A5A5A")
                                                     : QStringLiteral("#D6D6D6")));
            layout->addWidget(name, 1);
            m_agendaList->setItemWidget(item, widget);
        }
    }
}

void HomeModule::refreshSignals()
{
    m_signalsList->clear();

    auto addNavItem = [this](const QString& text, const QString& target, const QString& color) {
        auto* item = new QListWidgetItem(text, m_signalsList);
        item->setForeground(QColor(color));
        item->setData(Qt::UserRole, target);
        item->setToolTip(QStringLiteral("Open %1").arg(target));
        return item;
    };
    // Clicks on rows navigate to the matching tab.
    connect(m_signalsList, &QListWidget::itemClicked, this,
            [this](QListWidgetItem* item) {
                const QString target = item->data(Qt::UserRole).toString();
                if (!target.isEmpty() && context()->mainWindow()) {
                    context()->mainWindow()->activateModule(target);
                }
            },
            Qt::UniqueConnection);

    auto* wellbeingHeader = new QListWidgetItem(QStringLiteral("Wellbeing"), m_signalsList);
    wellbeingHeader->setForeground(QColor(QStringLiteral("#9A9A9A")));

    const models::Checkin checkin = context()->checkins()->entryFor(QDate::currentDate());
    if (checkin.id >= 0) {
        addNavItem(QStringLiteral("mood %1/5  \u00B7  energy %2/5").arg(checkin.mood).arg(checkin.energy),
                   QStringLiteral("wellbeing"), QStringLiteral("#D6D6D6"));
        if (!checkin.note.trimmed().isEmpty()) {
            auto* note = new QListWidgetItem(QStringLiteral("\u201C%1\u201D").arg(checkin.note.left(60)),
                                             m_signalsList);
            note->setForeground(QColor(QStringLiteral("#7A7A7A")));
        }
    } else {
        addNavItem(QStringLiteral("Check in now"), QStringLiteral("wellbeing"),
                   QStringLiteral("#7A9BC7"));
    }

    auto* streakHeader = new QListWidgetItem(QStringLiteral("Momentum"), m_signalsList);
    streakHeader->setForeground(QColor(QStringLiteral("#9A9A9A")));

    auto* timer = context()->timer();
    const QSet<QDate> checkinDates = context()->checkins()->allDates();
    int checkInStreak = 0;
    for (QDate d = QDate::currentDate(); checkinDates.contains(d); d = d.addDays(-1)) {
        ++checkInStreak;
    }
    int bestHabitStreak = 0;
    for (const models::Habit& h : context()->habits()->all()) {
        bestHabitStreak = qMax(bestHabitStreak, h.currentStreak);
    }
    addNavItem(QStringLiteral("focus %1  \u00B7  check-ins %2  \u00B7  habits %3")
                   .arg(timer->focusStreak()).arg(checkInStreak).arg(bestHabitStreak),
               QStringLiteral("statistics"), QStringLiteral("#D6D6D6"));

    const auto weather = context()->weather()->snapshot();
    if (weather.valid) {
        auto* weatherHeader = new QListWidgetItem(QStringLiteral("Environment"), m_signalsList);
        weatherHeader->setForeground(QColor(QStringLiteral("#9A9A9A")));
        addNavItem(QStringLiteral("%1\u00B0C  \u00B7  %2")
                       .arg(qRound(weather.temperatureC)).arg(weather.location),
                   QString(), QStringLiteral("#D6D6D6"));
    }

    // Full daily scripture, word-wrapped, lives on Home only.
    const auto verse = context()->verse()->dailyVerse();
    m_lastVerse = verse;
    if (verse.valid) {
        m_verseLabel->setText(QStringLiteral(
            "<a href=\"passage\" style=\"color:#C4C4C4; text-decoration:none;\">"
            "\u201C%1\u201D</a><br><span style=\"color:#7A7A7A;\">%2</span>")
            .arg(verse.text)
            .arg(verse.reference));
    } else {
        m_verseLabel->setText(QStringLiteral("Stillness and focus."));
    }
}

void HomeModule::startFocusOnCurrent()
{
    const auto tasks = context()->tasks()->active();
    if (tasks.isEmpty()) {
        context()->mainWindow()->openQuickCapture(QStringLiteral("task"));
        return;
    }
    models::Task current = tasks.first();
    for (const models::Task& t : tasks) {
        if (t.isDueToday() || !t.dueAt.isValid()) {
            if (t.priority > current.priority) current = t;
        }
    }
    context()->timer()->startFocus(current.id, current.title);
    context()->mainWindow()->activateModule(QStringLiteral("chronos"));
}

} // namespace arete::ui