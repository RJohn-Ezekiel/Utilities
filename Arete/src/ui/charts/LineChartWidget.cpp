#include "ui/charts/LineChartWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <algorithm>

namespace {
constexpr int kTopPad = 8;
constexpr int kBottomPad = 16;
} // namespace

LineChartWidget::LineChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(100);
}

void LineChartWidget::setSeries(const QVector<Series>& series, const QStringList& xLabels)
{
    m_series = series;
    m_xLabels = xLabels;
    update();
}

void LineChartWidget::setRange(double min, double max)
{
    m_min = min;
    m_max = max;
    update();
}

void LineChartWidget::setEmptyText(const QString& text)
{
    m_emptyText = text;
    update();
}

void LineChartWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    int count = 0;
    for (const Series& s : m_series) count = qMax(count, s.values.size());
    if (count == 0) {
        p.setPen(QColor(0x5A, 0x5A, 0x5A));
        p.drawText(rect(), Qt::AlignCenter,
                   m_emptyText.isEmpty() ? QStringLiteral("No data yet") : m_emptyText);
        return;
    }

    const QRect plot(0, kTopPad, width(), height() - kTopPad - kBottomPad);
    const double span = qMax(m_max - m_min, 1e-9);

    auto yFor = [&](double v) {
        return plot.bottom() - (v - m_min) / span * double(plot.height());
    };
    auto xFor = [&](int i) {
        return double(plot.left()) + (count == 1 ? plot.width() / 2.0
                                                 : double(i) * plot.width() / double(count - 1));
    };

    // Horizontal grid lines at integer values.
    p.setPen(QColor(0x2A, 0x2A, 0x2A));
    for (double v = std::ceil(m_min); v <= m_max; v += 1.0) {
        const int y = int(yFor(v));
        p.drawLine(plot.left(), y, plot.right(), y);
    }

    for (const Series& s : m_series) {
        if (s.values.isEmpty()) continue;
        QPainterPath path;
        path.moveTo(xFor(0), yFor(s.values.first()));
        for (int i = 1; i < s.values.size(); ++i) {
            path.lineTo(xFor(i), yFor(s.values.at(i)));
        }
        p.setPen(QPen(s.color, 2.0));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        // Dots on the points.
        p.setBrush(s.color);
        p.setPen(Qt::NoPen);
        for (int i = 0; i < s.values.size(); ++i) {
            p.drawEllipse(QPointF(xFor(i), yFor(s.values.at(i))), 2.5, 2.5);
        }
    }

    // X labels.
    if (!m_xLabels.isEmpty()) {
        p.setPen(QColor(0x6E, 0x6E, 0x6E));
        for (int i = 0; i < qMin(m_xLabels.size(), count); ++i) {
            const QString label = m_xLabels.at(i);
            const QRectF labelRect(xFor(i) - 16, plot.bottom() + 2, 32, kBottomPad - 2);
            p.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, label);
        }
    }
}
