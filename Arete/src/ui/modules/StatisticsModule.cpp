#include "ui/modules/StatisticsModule.h"
#include "app/AppContext.h"
#include "services/TimerService.h"
#include "data/JournalRepository.h"
#include "data/StatsRepository.h"
#include "data/CheckinRepository.h"
#include "data/HabitRepository.h"
#include "models/Checkin.h"
#include "models/Habit.h"
#include "ui/common/SectionLabel.h"
#include "ui/charts/BarChartWidget.h"
#include "ui/charts/LineChartWidget.h"

#include "models/Statistics.h"
#include "services/StatisticsService.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDate>

namespace arete::ui {

StatisticsModule::StatisticsModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(48, 32, 48, 32);
    outer->setSpacing(16);

    auto* title = new QLabel(QStringLiteral("Statistics"), this);
    title->setStyleSheet(QStringLiteral("font-size: 22px; color: #E2E2E2; letter-spacing: 1px;"));
    outer->addWidget(title);

    auto* row = new QHBoxLayout;
    row->setSpacing(16);
    auto card = [this]() {
        auto* c = new QLabel(this);
        c->setAlignment(Qt::AlignCenter);
        c->setStyleSheet(QStringLiteral(
            "QLabel { color: #D6D6D6; background-color: #1C1C1C; border: 1px solid #2A2A2A;"
            " border-radius: 12px; padding: 18px 10px; font-size: 15px; }"));
        return c;
    };
    m_todayFocus = card();
    m_todaySessions = card();
    m_weekFocus = card();
    m_streak = card();
    row->addWidget(m_todayFocus, 1);
    row->addWidget(m_todaySessions, 1);
    row->addWidget(m_weekFocus, 1);
    row->addWidget(m_streak, 1);
    outer->addLayout(row);

    auto* chartRow = new QHBoxLayout;
    chartRow->setSpacing(16);

    auto* focusBox = new QVBoxLayout;
    focusBox->addWidget(new common::SectionLabel(QStringLiteral("FOCUS  \u00B7  LAST 14 DAYS"), this));
    m_focusBars = new BarChartWidget(this);
    m_focusBars->setMinimumHeight(140);
    focusBox->addWidget(m_focusBars, 1);
    chartRow->addLayout(focusBox, 3);

    auto* habitBox = new QVBoxLayout;
    habitBox->addWidget(new common::SectionLabel(QStringLiteral("HABIT COMPLETION  \u00B7  LAST 7 DAYS"), this));
    m_habitBars = new BarChartWidget(this);
    m_habitBars->setMinimumHeight(140);
    habitBox->addWidget(m_habitBars, 1);
    chartRow->addLayout(habitBox, 2);

    outer->addLayout(chartRow, 2);

    auto* lowerRow = new QHBoxLayout;
    lowerRow->setSpacing(16);

    auto* moodBox = new QVBoxLayout;
    moodBox->addWidget(new common::SectionLabel(QStringLiteral("MOOD / ENERGY  \u00B7  LAST 14 DAYS"), this));
    m_moodTrend = new LineChartWidget(this);
    m_moodTrend->setMinimumHeight(140);
    moodBox->addWidget(m_moodTrend, 1);
    lowerRow->addLayout(moodBox, 3);

    auto* cardBox = new QVBoxLayout;
    cardBox->setSpacing(12);
    m_momentumCard = new QLabel(this);
    m_momentumCard->setStyleSheet(QStringLiteral(
        "QLabel { color: #D6D6D6; background-color: #1C1C1C; border: 1px solid #2A2A2A;"
        " border-radius: 12px; padding: 14px; font-size: 13px; }"));
    m_momentumCard->setWordWrap(true);
    m_habitCard = new QLabel(this);
    m_habitCard->setStyleSheet(QStringLiteral(
        "QLabel { color: #D6D6D6; background-color: #1C1C1C; border: 1px solid #2A2A2A;"
        " border-radius: 12px; padding: 14px; font-size: 13px; }"));
    m_habitCard->setWordWrap(true);
    cardBox->addWidget(m_momentumCard);
    cardBox->addWidget(m_habitCard);
    lowerRow->addLayout(cardBox, 2);

    outer->addLayout(lowerRow, 2);

    auto* weekLabel = new QLabel(QStringLiteral("LAST SEVEN DAYS"), this);
    weekLabel->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 12px; letter-spacing: 1px;"));
    outer->addWidget(weekLabel);

    m_weekList = new QListWidget(this);
    m_weekList->setFrameShape(QFrame::NoFrame);
    m_weekList->setSelectionMode(QAbstractItemView::NoSelection);
    m_weekList->setFocusPolicy(Qt::NoFocus);
    m_weekList->setStyleSheet(QStringLiteral(
        "QListWidget { background: transparent; outline: none; }"
        "QListWidget::item { padding: 8px 0; border-bottom: 1px solid #262626;"
        " color: #A0A0A0; font-size: 13px; }"));
    outer->addWidget(m_weekList, 1);
}

void StatisticsModule::onActivated()
{
    refresh();
}

void StatisticsModule::refreshView()
{
    refresh();
}

QString StatisticsModule::fmtDuration(int minutes)
{
    if (minutes < 60) return QStringLiteral("%1 min").arg(minutes);
    return QStringLiteral("%1 h %2 min").arg(minutes / 60).arg(minutes % 60);
}

QString StatisticsModule::momentumLabel(int weeklyMinutes)
{
    if (weeklyMinutes < 120) return QStringLiteral("MOMENTUM\nbuilding — aim for 2+ h of focus this week");
    if (weeklyMinutes < 360) return QStringLiteral("MOMENTUM\nsteady — 3+ h of deep work this week");
    return QStringLiteral("MOMENTUM\non a roll — 6+ h of focus this week");
}

void StatisticsModule::refresh()
{
    auto* timer = context()->timer();
    const QDate today = QDate::currentDate();

    m_todayFocus->setText(QStringLiteral("TODAY FOCUS\n%1").arg(fmtDuration(timer->focusMinutesToday())));
    m_todaySessions->setText(QStringLiteral("SESSIONS\n%1").arg(timer->sessionsToday()));
    m_weekFocus->setText(QStringLiteral("WEEK FOCUS\n%1").arg(fmtDuration(timer->weeklyFocusMinutes())));
    m_streak->setText(QStringLiteral("STREAK\n%1 days").arg(timer->focusStreak()));

    // Focus bars: last 14 days, minutes per day.
    {
        const QDate start = today.addDays(-13);
        const QVector<int> minutes = context()->stats()->focusMinutesPerDay(start, today);
        QStringList labels;
        for (int i = 0; i < minutes.size(); ++i) {
            const QDate day = start.addDays(i);
            labels.append(day.day() == 1 ? day.toString(QStringLiteral("MMM")) : QString());
        }
        m_focusBars->setData(minutes, labels);
    }

    // Habit completion: per-habit completed count over the last 7 days.
    {
        const QDate start = today.addDays(-6);
        const QVector<models::Habit> habits = context()->habits()->all();
        QVector<int> counts;
        counts.reserve(habits.size());
        for (const models::Habit& h : habits) {
            counts.append(context()->habits()->completedInRange(h.id, start, today));
        }
        QStringList labels;
        for (const models::Habit& h : habits) labels.append(h.name.left(6));
        m_habitBars->setData(counts, labels);
    }

    // Mood / energy trend: last 14 days.
    {
        const QDate start = today.addDays(-13);
        const QVector<models::Checkin> entries = context()->checkins()->range(start, today);
        QMap<QDate, models::Checkin> byDate;
        for (const models::Checkin& c : entries) byDate.insert(c.date, c);

        LineChartWidget::Series mood;
        mood.name = QStringLiteral("mood");
        mood.color = QColor(0xC8, 0xC8, 0xC8);
        LineChartWidget::Series energy;
        energy.name = QStringLiteral("energy");
        energy.color = QColor(0x7A, 0x7A, 0x7A);

        QStringList labels;
        for (QDate d = start; d <= today; d = d.addDays(1)) {
            const auto it = byDate.find(d);
            if (it != byDate.end()) {
                mood.values.append(it->mood);
                energy.values.append(it->energy);
            } else {
                mood.values.append(0);
                energy.values.append(0);
            }
            labels.append(d.day() == 1 ? d.toString(QStringLiteral("MMM")) : QString());
        }
        m_moodTrend->setSeries({mood, energy}, labels);
        m_moodTrend->setRange(0, 5);
    }

    // Cards: momentum + habit week overview.
    m_momentumCard->setText(momentumLabel(timer->weeklyFocusMinutes()));
    const QVector<models::Habit> habits = context()->habits()->all();
    if (habits.isEmpty()) {
        m_habitCard->setText(QStringLiteral("HABITS\nNo habits yet — add some to track streaks."));
    } else {
        const QDate start = today.addDays(-6);
        int done = 0;
        for (const models::Habit& h : habits) {
            done += context()->habits()->completedInRange(h.id, start, today);
        }
        m_habitCard->setText(QStringLiteral("HABITS\n%1 / %2 completions this week")
                                 .arg(done)
                                 .arg(habits.size() * 7));
    }

    const auto history = context()->chronosStats()->compute().dailyHistory;
    m_weekList->clear();
    for (int i = 6; i >= 0; --i) {
        const QDate day = today.addDays(-i);
        const chronos::DailyStats stats = history.value(day);
        const int minutes = static_cast<int>(stats.focusSeconds / 60);
        auto* item = new QListWidgetItem(QStringLiteral("%1  \u00B7  %2 focus  \u00B7  %3 sessions")
                                             .arg(day.toString(QStringLiteral("ddd, MMM d")))
                                             .arg(fmtDuration(minutes))
                                             .arg(stats.sessionsCompleted),
                                         m_weekList);
        if (i != 0) {
            item->setForeground(QColor(QStringLiteral("#7A7A7A")));
        }
    }
}

} // namespace arete::ui