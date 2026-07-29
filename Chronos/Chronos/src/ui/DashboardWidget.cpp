#include "DashboardWidget.h"
#include "Theme.h"
#include "WaterMeterWidget.h"
#include "storage/StorageManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>

namespace chronos {

DashboardWidget::DashboardWidget(StatisticsService* statsService,
                                 StorageManager* storage,
                                 QWidget* parent)
    : QWidget(parent)
    , m_statsService(statsService)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // Header
    auto* header = new QLabel(QStringLiteral("Dashboard"), this);
    header->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 20px; font-weight: bold;"
    ).arg(Theme::PrimaryText.name()));
    root->addWidget(header);

    auto* subHeader = new QLabel(QStringLiteral("Today"), this);
    subHeader->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 13px;"
    ).arg(Theme::SecondaryText.name()));
    root->addWidget(subHeader);

    // Cards grid
    auto* grid = new QGridLayout();
    grid->setSpacing(12);

    m_focusCard   = createCard(QStringLiteral("Focus Time"), this);
    m_breakCard   = createCard(QStringLiteral("Break Time"), this);
    m_sessionsCard = createCard(QStringLiteral("Sessions"), this);
    m_tasksCard   = createCard(QStringLiteral("Tasks Done"), this);
    m_streakCard  = createCard(QStringLiteral("Streak"), this);
    m_waterCard   = createCard(QStringLiteral("Water"), this);
    m_standCard   = createCard(QStringLiteral("Stand"), this);
    m_eyeCard     = createCard(QStringLiteral("Eye Break"), this);

    grid->addWidget(m_focusCard.container,   0, 0);
    grid->addWidget(m_breakCard.container,   0, 1);
    grid->addWidget(m_sessionsCard.container, 0, 2);
    grid->addWidget(m_tasksCard.container,   0, 3);
    grid->addWidget(m_streakCard.container,  1, 0);
    grid->addWidget(m_waterCard.container,   1, 1);
    grid->addWidget(m_standCard.container,   1, 2);
    grid->addWidget(m_eyeCard.container,     1, 3);

    root->addLayout(grid);

    m_waterMeter = new WaterMeterWidget(storage, this);
    root->addWidget(m_waterMeter);

    root->addStretch();

    refresh();
}

DashboardWidget::StatCard DashboardWidget::createCard(const QString& title,
                                                       QWidget* parent)
{
    StatCard card;
    card.container = new QWidget(parent);
    card.container->setFixedSize(160, 90);
    card.container->setStyleSheet(QStringLiteral(
        "background: %1; border: 1px solid %2;"
    ).arg(Theme::Card.name()).arg(Theme::Border.name()));

    auto* layout = new QVBoxLayout(card.container);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(4);

    card.valueLabel = new QLabel(QStringLiteral("--"), card.container);
    card.valueLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 22px; font-weight: bold; background: transparent;"
    ).arg(Theme::PrimaryText.name()));
    layout->addWidget(card.valueLabel);

    card.titleLabel = new QLabel(title, card.container);
    card.titleLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 11px; background: transparent;"
    ).arg(Theme::SecondaryText.name()));
    layout->addWidget(card.titleLabel);

    return card;
}

void DashboardWidget::setCardValue(StatCard& card, const QString& value)
{
    card.valueLabel->setText(value);
}

static QString formatTime(qint64 seconds)
{
    int hours = static_cast<int>(seconds / 3600);
    int mins = static_cast<int>((seconds % 3600) / 60);
    if (hours > 0) {
        return QStringLiteral("%1h %2m").arg(hours).arg(mins);
    }
    return QStringLiteral("%1m").arg(mins);
}

void DashboardWidget::refresh()
{
    auto stats = m_statsService->compute();

    setCardValue(m_focusCard, formatTime(stats.today.focusSeconds));
    setCardValue(m_breakCard, formatTime(stats.today.breakSeconds));
    setCardValue(m_sessionsCard, QString::number(stats.today.sessionsCompleted));
    setCardValue(m_tasksCard, QString::number(stats.today.tasksCompleted));
    setCardValue(m_streakCard, QStringLiteral("%1d").arg(stats.currentStreakDays));
    setCardValue(m_waterCard, QString::number(stats.today.waterAcks));
    setCardValue(m_standCard, QString::number(stats.today.standAcks));
    setCardValue(m_eyeCard, QString::number(stats.today.eyeAcks));
}

} // namespace chronos
