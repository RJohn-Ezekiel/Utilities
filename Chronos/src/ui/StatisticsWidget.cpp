#include "StatisticsWidget.h"
#include "Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDate>

namespace chronos {

StatisticsWidget::StatisticsWidget(StatisticsService* statsService,
                                   QWidget* parent)
    : QWidget(parent)
    , m_statsService(statsService)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // Header
    auto* header = new QLabel(QStringLiteral("Statistics"), this);
    header->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 20px; font-weight: bold;"
    ).arg(Theme::PrimaryText.name()));
    root->addWidget(header);

    // Period toggle
    auto* periodRow = new QHBoxLayout();
    periodRow->setSpacing(8);

    const QString btnBase = QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %3;"
        "  padding: 4px 16px; font-size: 13px; }"
        "QPushButton:hover { background: %4; }"
    ).arg(Theme::Card.name()).arg(Theme::PrimaryText.name())
     .arg(Theme::Border.name()).arg(Theme::Hover.name());

    const QString btnActive = QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %1;"
        "  padding: 4px 16px; font-size: 13px; }"
    ).arg(Theme::Accent.name()).arg(Theme::Background.name());

    m_weekBtn = new QPushButton(QStringLiteral("Week"), this);
    m_monthBtn = new QPushButton(QStringLiteral("Month"), this);
    m_weekBtn->setStyleSheet(btnActive);
    m_monthBtn->setStyleSheet(btnBase);

    periodRow->addWidget(m_weekBtn);
    periodRow->addWidget(m_monthBtn);
    periodRow->addStretch();

    connect(m_weekBtn, &QPushButton::clicked, this, [this]() {
        m_showingWeek = true;
        showWeek();
    });
    connect(m_monthBtn, &QPushButton::clicked, this, [this]() {
        m_showingWeek = false;
        showMonth();
    });

    root->addLayout(periodRow);

    // Period label
    m_periodLabel = new QLabel(this);
    m_periodLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 14px;"
    ).arg(Theme::SecondaryText.name()));
    root->addWidget(m_periodLabel);

    // Stats rows
    const QString rowStyle = QStringLiteral(
        "color: %1; font-size: 14px; padding: 4px 0;"
    ).arg(Theme::PrimaryText.name());

    auto addStat = [&](const QString& label, QLabel*& outLabel) {
        auto* row = new QHBoxLayout();
        auto* lbl = new QLabel(label, this);
        lbl->setStyleSheet(QStringLiteral(
            "color: %1; font-size: 14px;"
        ).arg(Theme::SecondaryText.name()));
        row->addWidget(lbl);
        outLabel = new QLabel(QStringLiteral("--"), this);
        outLabel->setStyleSheet(rowStyle);
        outLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(outLabel, 1);
        root->addLayout(row);
    };

    addStat(QStringLiteral("Focus Time:"), m_focusLabel);
    addStat(QStringLiteral("Break Time:"), m_breakLabel);
    addStat(QStringLiteral("Sessions:"), m_sessionsLabel);
    addStat(QStringLiteral("Tasks Completed:"), m_tasksLabel);
    addStat(QStringLiteral("Longest Session:"), m_longestLabel);
    addStat(QStringLiteral("Avg Daily Focus:"), m_avgLabel);

    root->addSpacing(24);

    // Placeholder for future charts
    m_chartPlaceholder = new QLabel(
        QStringLiteral("(Charts will appear here in a future update)"), this);
    m_chartPlaceholder->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 12px; padding: 32px;"
        "border: 1px dashed %2;"
    ).arg(Theme::SecondaryText.name()).arg(Theme::Border.name()));
    m_chartPlaceholder->setAlignment(Qt::AlignCenter);
    m_chartPlaceholder->setMinimumHeight(120);
    root->addWidget(m_chartPlaceholder);

    root->addStretch();
}

void StatisticsWidget::refresh()
{
    if (m_showingWeek) {
        showWeek();
    } else {
        showMonth();
    }
}

void StatisticsWidget::showWeek()
{
    m_weekBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %1;"
        "  padding: 4px 16px; font-size: 13px; }"
    ).arg(Theme::Accent.name()).arg(Theme::Background.name()));
    m_monthBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %3;"
        "  padding: 4px 16px; font-size: 13px; }"
        "QPushButton:hover { background: %4; }"
    ).arg(Theme::Card.name()).arg(Theme::PrimaryText.name())
     .arg(Theme::Border.name()).arg(Theme::Hover.name()));

    QDate today = QDate::currentDate();
    QDate weekStart = today.addDays(-(static_cast<int>(today.dayOfWeek()) - 1));
    auto stats = m_statsService->computeForDateRange(weekStart, today);
    updateDisplay(
        QStringLiteral("This week (%1 - %2)")
            .arg(weekStart.toString(QStringLiteral("MMM d")))
            .arg(today.toString(QStringLiteral("MMM d"))),
        stats);
}

void StatisticsWidget::showMonth()
{
    m_monthBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %1;"
        "  padding: 4px 16px; font-size: 13px; }"
    ).arg(Theme::Accent.name()).arg(Theme::Background.name()));
    m_weekBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %3;"
        "  padding: 4px 16px; font-size: 13px; }"
        "QPushButton:hover { background: %4; }"
    ).arg(Theme::Card.name()).arg(Theme::PrimaryText.name())
     .arg(Theme::Border.name()).arg(Theme::Hover.name()));

    QDate today = QDate::currentDate();
    QDate monthStart(today.year(), today.month(), 1);
    auto stats = m_statsService->computeForDateRange(monthStart, today);
    updateDisplay(
        QStringLiteral("This month (%1)").arg(today.toString(QStringLiteral("MMMM yyyy"))),
        stats);
}

static QString fmtTime(qint64 secs)
{
    int h = static_cast<int>(secs / 3600);
    int m = static_cast<int>((secs % 3600) / 60);
    if (h > 0) return QStringLiteral("%1h %2m").arg(h).arg(m);
    return QStringLiteral("%1m").arg(m);
}

void StatisticsWidget::updateDisplay(const QString& period, const DailyStats& stats)
{
    m_periodLabel->setText(period);
    m_focusLabel->setText(fmtTime(stats.focusSeconds));
    m_breakLabel->setText(fmtTime(stats.breakSeconds));
    m_sessionsLabel->setText(QString::number(stats.sessionsCompleted));
    m_tasksLabel->setText(QString::number(stats.tasksCompleted));

    auto fullStats = m_statsService->compute();
    m_longestLabel->setText(fmtTime(fullStats.longestSessionSeconds));
    m_avgLabel->setText(fmtTime(static_cast<qint64>(fullStats.averageDailyFocusSeconds)));
}

} // namespace chronos
