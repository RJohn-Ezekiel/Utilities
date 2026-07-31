#pragma once

#include <QIcon>
#include <QColor>

namespace phonio {

// Hand-painted transport icons (the platform's standard icons do not follow
// the application palette, so we render our own with the theme colors).
namespace TransportIcons {

QIcon play(const QColor& color);
QIcon pause(const QColor& color);
QIcon skipBack(const QColor& color);
QIcon skipForward(const QColor& color);
QIcon shuffle(const QColor& color);
QIcon repeat(const QColor& color);
QIcon volume(const QColor& color);
QIcon backArrow(const QColor& color);

} // namespace TransportIcons

} // namespace phonio
