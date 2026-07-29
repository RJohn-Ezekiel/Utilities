#include "WaterMeterWidget.h"
#include "Theme.h"
#include "storage/StorageManager.h"
#include "models/Statistics.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

namespace chronos {

WaterMeterWidget::WaterMeterWidget(StorageManager* storage, QWidget* parent)
    : QWidget(parent)
    , m_storage(storage)
{
    setFixedSize(160, 110);
    setStyleSheet(QStringLiteral(
        "background: %1; border: 1px solid %2;"
    ).arg(Theme::Card.name()).arg(Theme::Border.name()));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 10);
    root->setSpacing(6);

    m_descLabel = new QLabel(QStringLiteral("Water Intake"), this);
    m_descLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 11px; background: transparent;"
    ).arg(Theme::SecondaryText.name()));
    root->addWidget(m_descLabel);

    auto* row = new QHBoxLayout();
    row->setSpacing(8);

    const QString btnStyle = QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %3;"
        "  min-width: 26px; max-width: 26px; min-height: 26px; max-height: 26px;"
        "  font-size: 16px; }"
        "QPushButton:hover { background: %4; }"
    ).arg(Theme::Panel.name()).arg(Theme::PrimaryText.name())
     .arg(Theme::Border.name()).arg(Theme::Hover.name());

    m_minusBtn = new QPushButton(QStringLiteral("\u2212"), this);
    m_minusBtn->setStyleSheet(btnStyle);
    connect(m_minusBtn, &QPushButton::clicked, this, &WaterMeterWidget::removeGlass);
    row->addWidget(m_minusBtn);

    m_countLabel = new QLabel(QStringLiteral("0.0"), this);
    m_countLabel->setAlignment(Qt::AlignCenter);
    m_countLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 24px; font-weight: bold; background: transparent;"
    ).arg(Theme::Accent.name()));
    row->addWidget(m_countLabel, 1);

    m_plusBtn = new QPushButton(QStringLiteral("+"), this);
    m_plusBtn->setStyleSheet(btnStyle);
    connect(m_plusBtn, &QPushButton::clicked, this, &WaterMeterWidget::addGlass);
    row->addWidget(m_plusBtn);

    root->addLayout(row);

    m_unitLabel = new QLabel(QStringLiteral("L"), this);
    m_unitLabel->setAlignment(Qt::AlignCenter);
    m_unitLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 10px; background: transparent;"
    ).arg(Theme::SecondaryText.name()));
    root->addWidget(m_unitLabel);

    refresh();
}

void WaterMeterWidget::refresh()
{
    auto stats = m_storage->loadStatistics();
    m_glasses = stats.today.waterGlasses;
    double litres = m_glasses * 0.25;
    m_countLabel->setText(QStringLiteral("%1").arg(litres, 0, 'f', 1));
}

void WaterMeterWidget::addGlass()
{
    auto stats = m_storage->loadStatistics();
    stats.today.waterGlasses++;
    m_storage->saveStatistics(stats);
    refresh();
    emit waterUpdated();
}

void WaterMeterWidget::removeGlass()
{
    if (m_glasses <= 0) return;
    auto stats = m_storage->loadStatistics();
    stats.today.waterGlasses--;
    m_storage->saveStatistics(stats);
    refresh();
    emit waterUpdated();
}

} // namespace chronos
