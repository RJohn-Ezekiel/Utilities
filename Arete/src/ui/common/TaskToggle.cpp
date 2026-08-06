#include "ui/common/TaskToggle.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QEnterEvent>
#include <QPainter>
#include <QPainterPath>

namespace arete::ui::common {

TaskToggle::TaskToggle(QWidget* parent)
    : QWidget(parent)
{
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);

    m_animation.setDuration(120);
    m_animation.setEasingCurve(QEasingCurve::OutCubic);
    m_animation.setStartValue(0.0);
    m_animation.setEndValue(1.0);
    connect(&m_animation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& value) {
                m_progress = value.toReal();
                update();
            });
}

void TaskToggle::setChecked(bool checked, bool animate)
{
    if (m_checked == checked) return;
    m_checked = checked;
    if (animate) {
        m_animation.stop();
        m_animation.setStartValue(m_progress);
        m_animation.setEndValue(checked ? 1.0 : 0.0);
        m_animation.start();
    } else {
        m_animation.stop();
        m_progress = checked ? 1.0 : 0.0;
        update();
    }
    emit toggled(checked);
}

void TaskToggle::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal p = m_progress;
    const QRectF box(3.0, 3.0, 20.0, 20.0);

    // Box: fills toward the checked colour as progress grows.
    QColor fill(0x1C, 0x1C, 0x1C);
    fill = QColor::fromHsl(fill.hslHue(), fill.hslSaturation(),
                           qRound(fill.lightness() + (0xD0 - 0x1C) * p));
    painter.setPen(QPen(m_focused ? QColor(0x7A, 0x9B, 0xC7)
                                  : (m_hover ? QColor(0x6A, 0x6A, 0x6A)
                                             : QColor(0x4A, 0x4A, 0x4A)),
                        1.5));
    painter.setBrush(fill);
    painter.drawRoundedRect(box, 5, 5);

    // Glyphs: the ✕ fades out while the ✓ fades in.
    const qreal scale = 0.62 + 0.38 * p;
    painter.save();
    painter.translate(box.center());
    painter.scale(scale, scale);

    if (p < 1.0) {
        painter.setPen(QPen(QColor(0x9A, 0x9A, 0x9A, qRound(255 * (1.0 - p))), 2.0,
                            Qt::SolidLine, Qt::RoundCap));
        const qreal m = 3.5;
        painter.drawLine(QPointF(-m, -m), QPointF(m, m));
        painter.drawLine(QPointF(-m, m), QPointF(m, -m));
    }
    if (p > 0.0) {
        painter.setPen(QPen(QColor(0x18, 0x18, 0x18, qRound(255 * p)), 2.0,
                            Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath tick;
        tick.moveTo(-4.5, 0.5);
        tick.lineTo(-1.5, 3.5);
        tick.lineTo(4.5, -3.0);
        painter.drawPath(tick);
    }
    painter.restore();
}

void TaskToggle::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        setChecked(!m_checked);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void TaskToggle::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return
        || event->key() == Qt::Key_Enter) {
        setChecked(!m_checked);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void TaskToggle::focusInEvent(QFocusEvent* event)
{
    m_focused = true;
    update();
    QWidget::focusInEvent(event);
}

void TaskToggle::focusOutEvent(QFocusEvent* event)
{
    m_focused = false;
    update();
    QWidget::focusOutEvent(event);
}

void TaskToggle::enterEvent(QEnterEvent* event)
{
    m_hover = true;
    update();
    QWidget::enterEvent(event);
}

void TaskToggle::leaveEvent(QEvent* event)
{
    m_hover = false;
    update();
    QWidget::leaveEvent(event);
}

} // namespace arete::ui::common
