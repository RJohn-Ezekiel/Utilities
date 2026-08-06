#include "ui/charts/ActivityHeatmapWidget.h"

#include <QPainter>
#include <QDate>

namespace {
// Monochrome intensity ramp: none -> light -> mid -> dark.
QColor cellColor(int minutes)
{
    if (minutes <= 0) return QColor(0x24, 0x24, 0x24);
    if (minutes < 30) return QColor(0x3C, 0x3C, 0x3C);
    if (minutes < 60) return QColor(0x5A, 0x5A, 0x5A);
    if (minutes < 120) return QColor(0x8A, 0x8A, 0x8A);
    return QColor(0xC8, 0xC8, 0xC8);
}
} // namespace

ActivityHeatmapWidget::ActivityHeatmapWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(96);
}

void ActivityHeatmapWidget::setIntensity(const QMap<QDate, int>& intensity, int weeks)
{
    m_intensity = intensity;
    m_weeks = qMax(1, weeks);
    update();
}

void ActivityHeatmapWidget::setMetricName(const QString& name)
{
    m_metricName = name;
    update();
}

void ActivityHeatmapWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    const int cell = 11;
    const int gap = 3;
    const QDate today = QDate::currentDate();
    const QDate start = today.addDays(-(m_weeks * 7 - 1));

    const int weekCount = m_weeks;
    const int columns = weekCount;
    const int rows = 7;
    const int plotWidth = columns * (cell + gap) - gap;
    const int plotHeight = rows * (cell + gap) - gap;

    const int originX = 8;
    const int originY = (height() - plotHeight) / 2;

    // Grid.
    for (int col = 0; col < columns; ++col) {
        for (int row = 0; row < rows; ++row) {
            const QDate d = start.addDays(col * 7 + row);
            if (d > today) break;
            const int x = originX + col * (cell + gap);
            const int y = originY + row * (cell + gap);
            p.fillRect(QRect(x, y, cell, cell), cellColor(m_intensity.value(d, 0)));
        }
    }

    // Month markers along the top (first week of each month).
    p.setPen(QColor(0x6E, 0x6E, 0x6E));
    QDate previousMonth;
    for (int col = 0; col < columns; ++col) {
        const QDate d = start.addDays(col * 7);
        if (d > today) break;
        if (d.month() != previousMonth.month()) {
            previousMonth = d;
            p.drawText(QRect(originX + col * (cell + gap), originY - 14, 40, 12),
                       Qt::AlignLeft | Qt::AlignBottom, d.toString(QStringLiteral("MMM")));
        }
    }

    // Legend.
    const QString legend = QStringLiteral("%1: 0    <30m    <1h    <2h    2h+").arg(m_metricName);
    p.setPen(QColor(0x6E, 0x6E, 0x6E));
    p.drawText(QRect(originX, originY + plotHeight + 6, plotWidth, 14),
               Qt::AlignRight | Qt::AlignTop, legend);
}
