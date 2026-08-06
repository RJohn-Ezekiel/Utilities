#include "ui/modules/TodayModule.h"
#include "app/AppContext.h"
#include "data/TaskRepository.h"
#include "data/HabitRepository.h"
#include "data/CheckinRepository.h"
#include "models/Task.h"
#include "models/Habit.h"
#include "models/Checkin.h"
#include "ui/common/SectionLabel.h"
#include "ui/common/TaskRowWidget.h"
#include "ui/common/EmptyStateWidget.h"
#include "ui/common/TaskToggle.h"
#include "ui/MainWindow.h"
#include "services/TimerService.h"
#include "core/Icons.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QScrollBar>
#include <QLabel>
#include <QProgressBar>
#include <QDateTime>
#include <QTimer>
#include <QBrush>
#include <QSet>

namespace arete::ui {

namespace {
QString weekLabel(const QDate& date)
{
    int week = 0;
    date.weekNumber(&week);
    return QStringLiteral("Week %1").arg(week);
}

QString fmtMin(int minutes)
{
    if (minutes < 60) return QStringLiteral("%1m").arg(minutes);
    return QStringLiteral("%1h %2m").arg(minutes / 60).arg(minutes % 60);
}
} // namespace

TodayModule::TodayModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(48, 32, 48, 32);
    outer->setSpacing(16);

    // ── Header: title + date navigation ──
    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("Daily Dashboard"), this);
    title->setStyleSheet(QStringLiteral("font-size: 22px; color: #E2E2E2; letter-spacing: 1px;"));
    header->addWidget(title);

    m_dateLabel = new QLabel(this);
    m_dateLabel->setStyleSheet(QStringLiteral("color: #C4C4C4; font-size: 15px;"));
    header->addSpacing(12);
    header->addWidget(m_dateLabel);
    header->addStretch(1);

    auto navButton = [this](const QString& text, const QString& tip) {
        auto* b = new QPushButton(text, this);
        b->setFixedSize(28, 28);
        b->setCursor(Qt::PointingHandCursor);
        b->setToolTip(tip);
        b->setStyleSheet(QStringLiteral(
            "QPushButton { color: #9A9A9A; background-color: #1C1C1C;"
            " border: 1px solid #2A2A2A; border-radius: 6px; font-size: 14px; }"
            "QPushButton:hover { color: #D6D6D6; background-color: #242424; }"));
        return b;
    };
    m_prevButton = navButton(QStringLiteral("<"), QStringLiteral("Previous day"));
    m_todayButton = navButton(QStringLiteral("T"), QStringLiteral("Today"));
    m_nextButton = navButton(QStringLiteral(">"), QStringLiteral("Next day"));
    header->addWidget(m_prevButton);
    header->addWidget(m_todayButton);
    header->addWidget(m_nextButton);

    m_addButton = new QPushButton(QStringLiteral("+  NEW TASK"), this);
    m_addButton->setFixedHeight(28);
    m_addButton->setCursor(Qt::PointingHandCursor);
    m_addButton->setStyleSheet(QStringLiteral(
        "QPushButton { font-size: 12px; letter-spacing: 1px; color: #D6D6D6;"
        " background-color: #242424; border: 1px solid #353535; border-radius: 8px;"
        " padding: 0 14px; }"
        "QPushButton:hover { background-color: #2A2A2A; }"));
    header->addWidget(m_addButton);
    outer->addLayout(header);

    connect(m_addButton, &QPushButton::clicked, this, [this, context] {
        context->mainWindow()->openQuickCapture(QStringLiteral("task"));
    });
    connect(m_prevButton, &QPushButton::clicked, this,
            [this] { goToDate(m_selectedDate.addDays(-1)); });
    connect(m_nextButton, &QPushButton::clicked, this,
            [this] { goToDate(m_selectedDate.addDays(1)); });
    connect(m_todayButton, &QPushButton::clicked, this,
            [this] { goToDate(QDate::currentDate()); });

    // ── Summary panel (crona "Daily Dashboard" card) ──
    auto* summary = new QWidget(this);
    summary->setStyleSheet(QStringLiteral("QWidget { background-color: #1C1C1C;"
                                          " border: 1px solid #2A2A2A; border-radius: 12px; }"));
    auto* summaryLayout = new QVBoxLayout(summary);
    summaryLayout->setContentsMargins(18, 14, 18, 14);
    summaryLayout->setSpacing(10);

    auto summaryRow = [this, summary](const QString& heading, QLabel** out) {
        auto* row = new QHBoxLayout;
        auto* h = new QLabel(heading, summary);
        h->setStyleSheet(QStringLiteral("color: #9A9A9A; font-size: 12px; letter-spacing: 1px;"));
        row->addWidget(h);
        auto* value = new QLabel(summary);
        value->setStyleSheet(QStringLiteral("color: #D6D6D6; font-size: 13px;"));
        row->addWidget(value);
        row->addStretch(1);
        *out = value;
        return row;
    };
    auto thinBar = [this, summary]() {
        auto* bar = new QProgressBar(summary);
        bar->setRange(0, 100);
        bar->setTextVisible(false);
        bar->setFixedHeight(4);
        bar->setStyleSheet(QStringLiteral(
            "QProgressBar { background: #262626; border: none; border-radius: 2px; }"
            "QProgressBar::chunk { background: #7A7A7A; border-radius: 2px; }"));
        return bar;
    };

    auto* issueRow = summaryRow(QStringLiteral("ISSUES"), &m_issuesSummary);
    summaryLayout->addLayout(issueRow);
    m_issuesBar = thinBar();
    summaryLayout->addWidget(m_issuesBar);

    auto* habitRow = summaryRow(QStringLiteral("HABITS"), &m_habitsSummary);
    summaryLayout->addLayout(habitRow);
    m_habitsBar = thinBar();
    summaryLayout->addWidget(m_habitsBar);

    auto* momentumRow = new QHBoxLayout;
    momentumRow->setSpacing(24);
    m_signalsLabel = new QLabel(summary);
    m_streaksLabel = new QLabel(summary);
    for (QLabel* l : {m_signalsLabel, m_streaksLabel}) {
        l->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 12px;"));
    }
    momentumRow->addWidget(m_signalsLabel);
    momentumRow->addWidget(m_streaksLabel);
    momentumRow->addStretch(1);
    summaryLayout->addLayout(momentumRow);

    outer->addWidget(summary);

    // ── Body: issues (left) + habits (right) ──
    auto* body = new QHBoxLayout;
    body->setSpacing(16);

    auto* issuesBox = new QVBoxLayout;
    issuesBox->addWidget(new common::SectionLabel(QStringLiteral("ISSUES"), this));
    m_issueList = new QListWidget(this);
    m_issueList->setFrameShape(QFrame::NoFrame);
    m_issueList->setSelectionMode(QAbstractItemView::NoSelection);
    m_issueList->setFocusPolicy(Qt::NoFocus);
    m_issueList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_issueList->setStyleSheet(QStringLiteral(
        "QListWidget { background: transparent; outline: none; }"
        "QListWidget::item { border-bottom: 1px solid #262626; }"));
    issuesBox->addWidget(m_issueList, 1);
    body->addLayout(issuesBox, 3);

    auto* habitsBox = new QVBoxLayout;
    habitsBox->addWidget(new common::SectionLabel(QStringLiteral("HABITS"), this));
    m_habitList = new QListWidget(this);
    m_habitList->setFrameShape(QFrame::NoFrame);
    m_habitList->setSelectionMode(QAbstractItemView::NoSelection);
    m_habitList->setFocusPolicy(Qt::NoFocus);
    m_habitList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_habitList->setStyleSheet(QStringLiteral(
        "QListWidget { background: transparent; outline: none; }"
        "QListWidget::item { border-bottom: 1px solid #262626; }"));
    habitsBox->addWidget(m_habitList, 1);
    body->addLayout(habitsBox, 2);

    outer->addLayout(body, 1);
}

void TodayModule::onActivated()
{
    goToDate(QDate::currentDate());
    refresh();
}

void TodayModule::refreshView()
{
    refresh();
}

void TodayModule::goToDate(const QDate& date)
{
    m_selectedDate = date;
    refresh();
}

void TodayModule::refresh()
{
    refreshSummary();
    refreshIssues();
    refreshHabits();
    m_lastRefresh = QDate::currentDate();
}

// ── Summary panel ──
void TodayModule::refreshSummary()
{
    const bool isToday = m_selectedDate == QDate::currentDate();
    m_dateLabel->setText(QStringLiteral("For %1 \u00B7 %2")
                             .arg(m_selectedDate.toString(QStringLiteral("dddd, MMM d")))
                             .arg(weekLabel(m_selectedDate)));
    m_todayButton->setEnabled(!isToday);

    auto* timer = context()->timer();

    // Issues: resolved = completed (or archived) of those due today.
    int total = 0;
    int resolved = 0;
    const auto tasks = context()->tasks()->active();
    for (const models::Task& t : tasks) {
        if (!t.dueAt.isValid() || t.dueAt.date() != m_selectedDate) continue;
        ++total;
        if (t.status == models::TaskStatus::Completed || t.status == models::TaskStatus::Archived) {
            ++resolved;
        }
    }
    m_issuesSummary->setText(QStringLiteral("%1 / %2 resolved   \u00B7   focus %3 today")
                                 .arg(resolved).arg(total).arg(fmtMin(timer->focusMinutesToday())));
    m_issuesBar->setValue(total > 0 ? qRound(resolved * 100.0 / total) : 0);

    // Habits: completion for the selected date.
    const QVector<models::Habit> habits = context()->habits()->all();
    int done = 0;
    for (const models::Habit& h : habits) {
        if (context()->habits()->isDone(h.id, m_selectedDate)) ++done;
    }
    m_habitsSummary->setText(QStringLiteral("%1 / %2 completed   \u00B7   remaining %3")
                                 .arg(done).arg(habits.size()).arg(habits.size() - done));
    m_habitsBar->setValue(habits.isEmpty() ? 0 : qRound(done * 100.0 / habits.size()));

    // Signals: today's check-in (mood / energy).
    const models::Checkin checkin = context()->checkins()->entryFor(m_selectedDate);
    if (checkin.id >= 0) {
        m_signalsLabel->setText(QStringLiteral("SIGNALS   mood %1/5 \u00B7 energy %2/5")
                                    .arg(checkin.mood).arg(checkin.energy));
    } else {
        m_signalsLabel->setText(QStringLiteral("SIGNALS   no check-in for this day"));
    }

    // Streaks: focus · check-in · habit.
    int checkInStreak = 0;
    const QSet<QDate> checkinDates = context()->checkins()->allDates();
    for (QDate d = QDate::currentDate(); checkinDates.contains(d); d = d.addDays(-1)) {
        ++checkInStreak;
    }
    int bestHabitStreak = 0;
    for (const models::Habit& h : habits) {
        bestHabitStreak = qMax(bestHabitStreak, h.currentStreak);
    }
    m_streaksLabel->setText(QStringLiteral("STREAKS   focus %1  \u00B7  check-ins %2  \u00B7  habits %3")
                                .arg(timer->focusStreak()).arg(checkInStreak).arg(bestHabitStreak));
}

// ── Issues list ──
void TodayModule::refreshIssues()
{
    m_issueList->clear();
    const bool isToday = m_selectedDate == QDate::currentDate();
    const auto tasks = context()->tasks()->active();

    QVector<models::Task> sectionA; // overdue (today only) or due this date
    QVector<models::Task> sectionB; // due today (only when viewing another date)
    QVector<models::Task> sectionC; // next up
    for (const models::Task& t : tasks) {
        if (!t.dueAt.isValid()) {
            sectionC.append(t);
            continue;
        }
        const QDate due = t.dueAt.date();
        if (isToday) {
            if (t.isOverdue()) sectionA.append(t);
            else if (t.isDueToday()) sectionB.append(t);
            else sectionC.append(t);
        } else {
            if (due == m_selectedDate) sectionA.append(t);
            else sectionC.append(t);
        }
    }

    const auto sortByPriority = [](const models::Task& a, const models::Task& b) {
        return a.priority > b.priority;
    };
    std::sort(sectionA.begin(), sectionA.end(), sortByPriority);
    std::sort(sectionB.begin(), sectionB.end(), sortByPriority);
    std::sort(sectionC.begin(), sectionC.end(), sortByPriority);

    if (sectionA.isEmpty() && sectionB.isEmpty() && sectionC.isEmpty()) {
        m_issueList->addItem(QString());
        m_issueList->setItemWidget(
            m_issueList->item(0),
            new common::EmptyStateWidget(QStringLiteral(""),
                                         QStringLiteral("Nothing on today. Breathe in, breathe out."),
                                         this));
        return;
    }

    const auto addSection = [this](const QString& name, QVector<models::Task>& list) {
        if (list.isEmpty()) return;
        m_issueList->addItem(QString());
        m_issueList->setItemWidget(
            m_issueList->item(m_issueList->count() - 1),
            new common::SectionLabel(sectionTitle(name, list.size()), this));
        for (const models::Task& t : list) appendTask(t);
    };

    if (isToday) {
        addSection(QStringLiteral("OVERDUE"), sectionA);
        addSection(QStringLiteral("TODAY"), sectionB);
    } else {
        addSection(QStringLiteral("ON %1").arg(
                       m_selectedDate.toString(QStringLiteral("ddd MMM d")).toUpper()),
                   sectionA);
    }
    addSection(QStringLiteral("NEXT UP"), sectionC);
}

// ── Habits list ──
void TodayModule::refreshHabits()
{
    m_habitList->clear();
    const QVector<models::Habit> habits = context()->habits()->all();
    if (habits.isEmpty()) {
        m_habitList->addItem(QString());
        m_habitList->setItemWidget(
            m_habitList->item(0),
            new common::EmptyStateWidget(QStringLiteral(""),
                                         QStringLiteral("No habits yet \u2014 add one in the Habits tab."),
                                         this));
        return;
    }

    const QDate weekStart = QDate::currentDate().addDays(-6);
    for (const models::Habit& h : habits) {
        const bool done = context()->habits()->isDone(h.id, m_selectedDate);
        auto* item = new QListWidgetItem;
        item->setSizeHint(QSize(0, 44));
        m_habitList->addItem(item);

        auto* widget = new QWidget(this);
        auto* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(4, 0, 4, 0);
        layout->setSpacing(10);

        auto* check = new common::TaskToggle(widget);
        check->setChecked(done);
        check->setToolTip(QStringLiteral("Mark done for %1").arg(m_selectedDate.toString(Qt::ISODate)));
        connect(check, &common::TaskToggle::toggled, this, [this, h](bool) {
            context()->habits()->toggle(h.id, m_selectedDate);
            refresh();
        });
        layout->addWidget(check);

        auto* text = new QLabel(h.name, widget);
        text->setStyleSheet(QStringLiteral("font-size: 14px; color: #D6D6D6;"));
        layout->addWidget(text, 1);

        auto* strip = new QHBoxLayout;
        strip->setSpacing(2);
        for (int i = 0; i < 7; ++i) {
            const QDate d = weekStart.addDays(i);
            const bool dayDone = context()->habits()->isDone(h.id, d);
            auto* dot = new QLabel(dayDone ? QStringLiteral("*") : QStringLiteral("."),
                                   widget);
            dot->setStyleSheet(QStringLiteral("color: %1; font-size: 8px;")
                                   .arg(dayDone ? QStringLiteral("#9A9A9A")
                                                : QStringLiteral("#3A3A3A")));
            strip->addWidget(dot);
        }
        layout->addLayout(strip);

        auto* meta = new QLabel(QStringLiteral("%1 day streak").arg(h.currentStreak), widget);
        meta->setStyleSheet(QStringLiteral("color: #6E6E6E; font-size: 11px;"));
        layout->addWidget(meta);

        m_habitList->setItemWidget(item, widget);
    }
}

void TodayModule::appendTask(const models::Task& task)
{
    auto* row = new common::TaskRowWidget(task, this);
    connect(row, &common::TaskRowWidget::toggled, this, [this](qint64 id, bool completed) {
        models::Task t = context()->tasks()->byId(id);
        if (t.id < 0) return;
        t.status = completed ? models::TaskStatus::Completed : models::TaskStatus::Active;
        t.completedAt = completed ? QDateTime::currentDateTime() : QDateTime();
        t.checkedItems = t.checklist; // full checklist done when task completes
        context()->tasks()->update(t);
        if (completed) {
            context()->mainWindow()->showTaskCompletion(id, t.title);
        }
        refresh();
    });
    connect(row, &common::TaskRowWidget::focusRequested, this,
            [this](qint64 id, const QString& title) {
                context()->timer()->startFocus(id, title);
                context()->mainWindow()->activateModule(QStringLiteral("chronos"));
            });
    connect(row, &common::TaskRowWidget::editRequested, this,
            [this](qint64 id) { context()->tasks()->update(context()->tasks()->byId(id)); });

    m_issueList->addItem(QString());
    m_issueList->setItemWidget(m_issueList->item(m_issueList->count() - 1), row);
}

QString TodayModule::sectionTitle(const QString& label, int count)
{
    return QStringLiteral("%1  \u2014  %2").arg(label).arg(count);
}

void TodayModule::highlightTask(qint64 taskId)
{
    refresh();
    for (int i = 0; i < m_issueList->count(); ++i) {
        auto* row = qobject_cast<common::TaskRowWidget*>(m_issueList->itemWidget(m_issueList->item(i)));
        if (row && row->taskId() == taskId) {
            m_issueList->scrollToItem(m_issueList->item(i), QAbstractItemView::PositionAtCenter);
            auto* item = m_issueList->item(i);
            item->setBackground(QColor(QStringLiteral("#2E2E2E")));
            QTimer::singleShot(1200, this, [item] { item->setBackground(QBrush()); });
            return;
        }
    }
}

} // namespace arete::ui