#include "ui/charts/BarChartWidget.h"

#include <QPainter>
#include <QFontMetrics>

namespace {
constexpr int kTopPad = 14;
constexpr int kBottomPad = 16;
} // namespace

BarChartWidget::BarChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
}

void BarChartWidget::setData(const QVector<int>& values, const QStringList& labels)
{
    m_values = values;
    m_labels = labels;
    update();
}

void BarChartWidget::setBarColor(const QColor& color)
{
    m_barColor = color;
    update();
}

void BarChartWidget::setEmptyText(const QString& text)
{
    m_emptyText = text;
    update();
}

void BarChartWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int maxValue = m_values.isEmpty() ? 0 : *std::max_element(m_values.cbegin(), m_values.cend());

    if (m_values.isEmpty() || maxValue <= 0) {
        p.setPen(QColor(0x5A, 0x5A, 0x5A));
        p.drawText(rect(), Qt::AlignCenter,
                   m_emptyText.isEmpty() ? QStringLiteral("No data yet") : m_emptyText);
        return;
    }

    const QRect plot(0, kTopPad, width(), height() - kTopPad - kBottomPad);
    const int n = m_values.size();
    const double slot = double(plot.width()) / double(qMax(1, n));
    const double barWidth = qMax(2.0, slot * 0.62);

    p.setPen(m_barColor);
    for (int i = 0; i < n; ++i) {
        const int v = qMax(0, m_values.at(i));
        const double h = double(v) / double(maxValue) * double(plot.height());
        const QRectF bar(plot.left() + slot * i + (slot - barWidth) / 2.0,
                         plot.bottom() - h, barWidth, h);
        p.setBrush(m_barColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(bar, 2.0, 2.0);

        if (!m_labels.isEmpty() && i < m_labels.size()) {
            p.setPen(QColor(0x6E, 0x6E, 0x6E));
            const QFont f = font();
            p.setFont(f);
            const QString label = m_labels.at(i);
            p.drawText(QRectF(plot.left() + slot * i, plot.bottom() + 2, slot, kBottomPad - 2),
                       Qt::AlignHCenter | Qt::AlignTop, label);
        }
    }

    // Max value annotation.
    if (maxValue > 0) {
        p.setPen(QColor(0x8A, 0x8A, 0x8A));
        p.drawText(QRect(2, 0, width() - 4, kTopPad - 2), Qt::AlignRight | Qt::AlignBottom,
                   QString::number(maxValue));
    }
}
