#include "ui/TransportIcons.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

#include <vector>

// Icons traced from the shared SVG button set (24x24 viewBox, currentColor).
// The geometry matches the original paths exactly; arcs are lifted with
// QPainterPath::arcTo.

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

QIcon makeIcon(const std::function<void(QPainter&)>& draw)
{
    return QIcon(makePixmap(draw));
}

void drawPlay(QPainter& p, const QColor& c)
{
    p.setBrush(c);
    QPainterPath path;
    path.moveTo(8.0, 5.0);
    path.lineTo(8.0, 19.0);
    path.lineTo(19.0, 12.0);
    path.closeSubpath();
    p.drawPath(path);
}

void drawPause(QPainter& p, const QColor& c)
{
    p.setBrush(c);
    p.drawRect(QRectF(6.0, 5.0, 4.0, 14.0));
    p.drawRect(QRectF(14.0, 5.0, 4.0, 14.0));
}

void drawSkip(QPainter& p, const QColor& c, bool forward)
{
    p.setBrush(c);
    QPainterPath tri;
    if (forward) {
        tri.moveTo(6.0, 18.0);
        tri.lineTo(14.5, 12.0);
        tri.lineTo(6.0, 6.0);
        tri.closeSubpath();
        p.drawPath(tri);
        p.drawRect(QRectF(16.0, 6.0, 2.0, 12.0));
    } else {
        p.drawRect(QRectF(6.0, 6.0, 2.0, 12.0));
        tri.moveTo(9.5, 12.0);
        tri.lineTo(18.0, 18.0);
        tri.lineTo(18.0, 6.0);
        tri.closeSubpath();
        p.drawPath(tri);
    }
}

void drawStrokes(QPainter& p, const QColor& c, const std::vector<std::vector<QPointF>>& polylines)
{
    QPen pen(c, 2.0);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    for (const auto& pts : polylines) {
        if (pts.empty())
            continue;
        QPainterPath path(pts.front());
        for (size_t i = 1; i < pts.size(); ++i)
            path.lineTo(pts[i]);
        p.drawPath(path);
    }
}

QIcon strokeIcon(const QColor& c, const std::vector<std::vector<QPointF>>& polylines)
{
    return makeIcon([&](QPainter& p) { drawStrokes(p, c, polylines); });
}

} // namespace

QIcon play(const QColor& color)
{
    return makeIcon([&](QPainter& p) { drawPlay(p, color); });
}

QIcon pause(const QColor& color)
{
    return makeIcon([&](QPainter& p) { drawPause(p, color); });
}

QIcon skipBack(const QColor& color)
{
    return makeIcon([&](QPainter& p) { drawSkip(p, color, false); });
}

QIcon skipForward(const QColor& color)
{
    return makeIcon([&](QPainter& p) { drawSkip(p, color, true); });
}

QIcon shuffle(const QColor& color)
{
    return strokeIcon(color, {
        { {16, 3}, {21, 3}, {21, 8} },
        { {4, 20}, {21, 3} },
        { {21, 16}, {21, 21}, {16, 21} },
        { {15, 15}, {21, 21} },
        { {4, 4}, {9, 9} },
    });
}

// Repeat loop: two rounded corners + corner arrows (from the SVG path):
//   M3 11V9a4 4 0 0 1 4-4h14  → (3,11) up, arc to (7,5), across to (21,5)
//   M21 13v2a4 4 0 0 1-4 4H3  → (21,13) down, arc to (17,19), across to (3,19)
void drawRepeatBody(QPainter& p, const QColor& c)
{
    QPen pen(c, 2.0);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    // Top: chevron (17,1)->(21,5)->(17,9)
    p.drawLine(QPointF(17, 1), QPointF(21, 5));
    p.drawLine(QPointF(21, 5), QPointF(17, 9));

    // Top edge: (3,11) up to (3,9), arc r=4 to (7,5), across to (21,5)
    QPainterPath top(QPointF(3, 11));
    top.lineTo(3, 9);
    top.arcTo(QRectF(3, 5, 8, 8), 180.0, -90.0); // center (7,9)
    top.lineTo(21, 5);

    // Bottom edge: mirrored arc
    QPainterPath bot(QPointF(21, 13));
    bot.lineTo(21, 15);
    bot.arcTo(QRectF(13, 11, 8, 8), 0.0, -90.0); // center (17,15)
    bot.lineTo(3, 19);

    p.drawPath(top);
    p.drawPath(bot);

    // Bottom-left chevron (7,23)->(3,19)->(7,15)
    p.drawLine(QPointF(7, 23), QPointF(3, 19));
    p.drawLine(QPointF(3, 19), QPointF(7, 15));
}

QIcon repeat(const QColor& color)
{
    return makeIcon([&](QPainter& p) { drawRepeatBody(p, color); });
}

QIcon repeatOne(const QColor& color)
{
    return makeIcon([&](QPainter& p) {
        drawRepeatBody(p, color);
        QPen pen(color, 1.5);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        QPainterPath one(QPointF(11, 10));
        one.lineTo(12, 10);
        one.lineTo(12, 14);
        p.drawPath(one);
    });
}

QIcon queue(const QColor& color)
{
    return strokeIcon(color, {
        { {3, 6}, {21, 6} },
        { {3, 12}, {15, 12} },
        { {3, 18}, {11, 18} },
        { {16, 16}, {19, 19}, {22, 16} },
        { {19, 12}, {19, 19} },
    });
}

QIcon volume(const QColor& color)
{
    return makeIcon([&](QPainter& p) {
        QPainterPath speaker;
        speaker.moveTo(5.0, 9.5);
        speaker.lineTo(8.0, 9.5);
        speaker.lineTo(11.5, 6.5);
        speaker.lineTo(11.5, 17.5);
        speaker.lineTo(8.0, 14.5);
        speaker.lineTo(5.0, 14.5);
        speaker.closeSubpath();
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawPath(speaker);
        QPen pen(color, 1.8);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        QPainterPath waves;
        waves.moveTo(14.0, 9.0);
        waves.arcTo(QRectF(12.5, 8.0, 5.0, 8.0), -50.0, 100.0);
        waves.moveTo(16.0, 7.0);
        waves.arcTo(QRectF(14.5, 6.0, 7.0, 12.0), -50.0, 100.0);
        p.drawPath(waves);
    });
}

QIcon backArrow(const QColor& color)
{
    return makeIcon([&](QPainter& p) {
        QPen pen(color, 2.2);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        QPainterPath path;
        path.moveTo(17.0, 5.5);
        path.lineTo(9.5, 12.0);
        path.lineTo(17.0, 18.5);
        p.drawPath(path);
    });
}

} // namespace TransportIcons
} // namespace phonio