#include "CircularTimerWidget.h"
#include "Theme.h"

#include <QPainter>
#include <QPaintEvent>

namespace chronos {

CircularTimerWidget::CircularTimerWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(220, 220);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void CircularTimerWidget::setProgress(int remainingSeconds, int totalSeconds)
{
    m_remainingSeconds = remainingSeconds;
    m_totalSeconds = totalSeconds;
    update();
}

void CircularTimerWidget::setSessionLabel(const QString& label)
{
    m_sessionLabel = label;
    update();
}

void CircularTimerWidget::setTaskLabel(const QString& label)
{
    m_taskLabel = label;
    update();
}

void CircularTimerWidget::clear()
{
    m_remainingSeconds = 0;
    m_totalSeconds = 0;
    m_sessionLabel.clear();
    m_taskLabel.clear();
    update();
}

QSize CircularTimerWidget::minimumSizeHint() const
{
    return QSize(220, 220);
}

QSize CircularTimerWidget::sizeHint() const
{
    return QSize(360, 360);
}

void CircularTimerWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height()) - 20;
    if (side < 20) return;

    int penWidth = qBound(4, side / 30, 12);
    int margin = (width() - side) / 2;
    int yOffset = (height() - side) / 2;

    // --- Session label above ring ---
    if (!m_sessionLabel.isEmpty()) {
        QFont labelFont = painter.font();
        labelFont.setPointSize(qMax(10, side / 16));
        labelFont.setWeight(QFont::Normal);
        painter.setFont(labelFont);
        painter.setPen(Theme::SecondaryText);

        QRectF labelRect(0, yOffset - 28, width(), 24);
        painter.drawText(labelRect, Qt::AlignHCenter | Qt::AlignBottom, m_sessionLabel);
    }

    QRectF ringRect(margin, yOffset, side, side);

    // --- Background ring ---
    QPen bgPen(Theme::Border, penWidth, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(bgPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(ringRect, 0, 360 * 16);

    // --- Progress ring ---
    if (m_totalSeconds > 0) {
        double progress = static_cast<double>(m_remainingSeconds)
                        / static_cast<double>(m_totalSeconds);

        QPen fgPen(Theme::Accent, penWidth, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(fgPen);

        int spanAngle = static_cast<int>(-progress * 360.0 * 16.0);
        painter.drawArc(ringRect, 90 * 16, spanAngle);
    }

    // --- Center text: remaining time ---
    int minutes = m_remainingSeconds / 60;
    int seconds = m_remainingSeconds % 60;
    QString timeText = QStringLiteral("%1:%2")
                           .arg(minutes, 2, 10, QChar('0'))
                           .arg(seconds, 2, 10, QChar('0'));

    QFont timeFont = painter.font();
    timeFont.setPointSize(qMax(14, side / 6));
    timeFont.setWeight(QFont::Normal);
    painter.setFont(timeFont);

    QFontMetrics fm(timeFont);
    QPointF center = ringRect.center();
    QRectF textRect(center.x() - fm.horizontalAdvance(timeText) / 2.0,
                    center.y() - fm.height() / 2.0,
                    fm.horizontalAdvance(timeText),
                    fm.height());
    painter.setPen(Theme::PrimaryText);
    painter.drawText(textRect, Qt::AlignCenter, timeText);

    // --- Task label below ring ---
    if (!m_taskLabel.isEmpty()) {
        QFont taskFont = painter.font();
        taskFont.setPointSize(qMax(9, side / 18));
        painter.setFont(taskFont);
        painter.setPen(Theme::SecondaryText);

        QRectF taskRect(0, ringRect.bottom() + 10, width(), 30);
        painter.drawText(taskRect, Qt::AlignHCenter | Qt::AlignTop, m_taskLabel);
    }
}

} // namespace chronos
