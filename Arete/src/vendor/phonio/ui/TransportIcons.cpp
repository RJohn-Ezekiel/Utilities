#include "ui/TransportIcons.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace phonio {
namespace TransportIcons {

namespace {

QPixmap makePixmap(const std::function<void(QPainter&)>& draw, int size = 24)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    draw(p);
    return pm;
}

void drawPlay(QPainter& p, const QColor& c)
{
    p.setBrush(c);
    QPainterPath path;
    path.moveTo(8.0, 5.5);
    path.lineTo(8.0, 18.5);
    path.lineTo(19.5, 12.0);
    path.closeSubpath();
    p.drawPath(path);
}

void drawPause(QPainter& p, const QColor& c)
{
    p.setBrush(c);
    p.drawRoundedRect(QRectF(7.0, 5.0, 3.6, 14.0), 1.2, 1.2);
    p.drawRoundedRect(QRectF(13.4, 5.0, 3.6, 14.0), 1.2, 1.2);
}

void drawSkip(QPainter& p, const QColor& c, bool forward)
{
    p.setBrush(c);
    QPainterPath tri;
    const double tx = forward ? 8.0 : 13.5;
    tri.moveTo(tx, 6.0);
    tri.lineTo(tx, 18.0);
    tri.lineTo(tx + 7.0, 12.0);
    tri.closeSubpath();
    p.drawPath(tri);
    p.drawRoundedRect(QRectF(forward ? 5.5 : 11.0, 6.0, 2.2, 12.0), 1.0, 1.0);
}

void drawShuffle(QPainter& p, const QColor& c)
{
    QPen pen(c, 2.0);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    QPainterPath path;
    path.moveTo(5.0, 7.0);
    path.cubicTo(8.5, 7.0, 9.5, 17.0, 13.0, 17.0);
    path.moveTo(19.0, 7.0);
    path.cubicTo(15.5, 7.0, 14.5, 17.0, 11.0, 17.0);
    p.drawPath(path);
    QPolygonF left{ QPointF(2.0, 7.0), QPointF(7.5, 4.5), QPointF(7.5, 9.5) };
    QPolygonF right{ QPointF(22.0, 7.0), QPointF(16.5, 4.5), QPointF(16.5, 9.5) };
    p.setBrush(c);
    p.drawPolygon(left);
    p.drawPolygon(right);
}

void drawRepeat(QPainter& p, const QColor& c)
{
    QPen pen(c, 2.0);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    QPainterPath path;
    path.moveTo(6.0, 9.5);
    path.arcTo(QRectF(6.0, 6.0, 12.0, 7.0), 180.0, 180.0);
    p.drawPath(path);
    QPolygonF arrow{ QPointF(3.0, 9.5), QPointF(6.5, 6.0), QPointF(6.5, 13.0) };
    p.setBrush(c);
    p.drawPolygon(arrow);
    path = QPainterPath();
    path.moveTo(18.0, 14.5);
    path.arcTo(QRectF(6.0, 11.0, 12.0, 7.0), 0.0, 180.0);
    p.drawPath(path);
    QPolygonF arrow2{ QPointF(21.0, 14.5), QPointF(17.5, 11.0), QPointF(17.5, 18.0) };
    p.setBrush(c);
    p.drawPolygon(arrow2);
}

void drawVolume(QPainter& p, const QColor& c)
{
    QPainterPath speaker;
    speaker.moveTo(5.0, 9.5);
    speaker.lineTo(8.0, 9.5);
    speaker.lineTo(11.5, 6.5);
    speaker.lineTo(11.5, 17.5);
    speaker.lineTo(8.0, 14.5);
    speaker.lineTo(5.0, 14.5);
    speaker.closeSubpath();
    p.setBrush(c);
    p.setPen(Qt::NoPen);
    p.drawPath(speaker);
    QPen pen(c, 1.8);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    QPainterPath waves;
    waves.moveTo(14.0, 9.0);
    waves.arcTo(QRectF(12.5, 8.0, 5.0, 8.0), -50.0, 100.0);
    waves.moveTo(16.0, 7.0);
    waves.arcTo(QRectF(14.5, 6.0, 7.0, 12.0), -50.0, 100.0);
    p.drawPath(waves);
}

void drawBackArrow(QPainter& p, const QColor& c)
{
    QPen pen(c, 2.2);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    QPainterPath path;
    path.moveTo(17.0, 5.5);
    path.lineTo(9.5, 12.0);
    path.lineTo(17.0, 18.5);
    p.drawPath(path);
}

} // namespace

QIcon play(const QColor& color)
{
    return QIcon(makePixmap([&](QPainter& p) { drawPlay(p, color); }));
}

QIcon pause(const QColor& color)
{
    return QIcon(makePixmap([&](QPainter& p) { drawPause(p, color); }));
}

QIcon skipBack(const QColor& color)
{
    return QIcon(makePixmap([&](QPainter& p) { drawSkip(p, color, false); }));
}

QIcon skipForward(const QColor& color)
{
    return QIcon(makePixmap([&](QPainter& p) { drawSkip(p, color, true); }));
}

QIcon shuffle(const QColor& color)
{
    return QIcon(makePixmap([&](QPainter& p) { drawShuffle(p, color); }));
}

QIcon repeat(const QColor& color)
{
    return QIcon(makePixmap([&](QPainter& p) { drawRepeat(p, color); }));
}

QIcon volume(const QColor& color)
{
    return QIcon(makePixmap([&](QPainter& p) { drawVolume(p, color); }));
}

QIcon backArrow(const QColor& color)
{
    return QIcon(makePixmap([&](QPainter& p) { drawBackArrow(p, color); }));
}

} // namespace TransportIcons
} // namespace phonio
