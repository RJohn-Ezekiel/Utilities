#include "ui/ArtworkLabel.h"

#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>

namespace phonio {

ArtworkLabel::ArtworkLabel(QWidget* parent)
    : QLabel(parent)
    , m_fade(new QPropertyAnimation(this, "fadeProgress"))
{
    setMinimumSize(1, 1);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_fade->setDuration(250);
    m_fade->setEasingCurve(QEasingCurve::OutCubic);
}

void ArtworkLabel::setArtwork(const QPixmap& pixmap, bool animate)
{
    if (pixmap.cacheKey() == m_pixmap.cacheKey())
        return;
    if (animate && !m_pixmap.isNull() && !pixmap.isNull()) {
        m_oldPixmap = m_pixmap;
        m_fade->stop();
        m_fade->setStartValue(0.0);
        m_fade->setEndValue(1.0);
        m_fade->start();
    } else {
        m_oldPixmap = {};
        m_fadeProgress = 1.0;
    }
    m_pixmap = pixmap;
    update();
}

void ArtworkLabel::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF target = QRectF(rect()).adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(target, m_cornerRadius, m_cornerRadius);
    p.setClipPath(path);

    if (m_fadeProgress < 1.0 && !m_oldPixmap.isNull()) {
        p.setOpacity(1.0 - m_fadeProgress);
        p.drawPixmap(target, m_oldPixmap,
                     m_oldPixmap.rect().intersected(
                         QRect(0, 0, m_oldPixmap.width(), m_oldPixmap.height())));
        p.setOpacity(1.0);
    }
    if (!m_pixmap.isNull())
        p.drawPixmap(target, m_pixmap, m_pixmap.rect());
    else {
        p.setBrush(QColor(58, 58, 58));
        p.setPen(Qt::NoPen);
        p.drawRect(target);
    }
}

} // namespace phonio
