#pragma once

#include <QIcon>
#include <QColor>

namespace phonio {

// Transport icons rendered from the shared SVG button set (the platform's
// standard icons do not follow the application palette, so we draw the SVG
// geometry ourselves with the theme colors).
namespace TransportIcons {

QIcon play(const QColor& color);
QIcon pause(const QColor& color);
QIcon skipBack(const QColor& color);
QIcon skipForward(const QColor& color);
QIcon shuffle(const QColor& color);
QIcon repeat(const QColor& color);
QIcon repeatOne(const QColor& color);
QIcon queue(const QColor& color);
QIcon volume(const QColor& color);
QIcon backArrow(const QColor& color);

} // namespace TransportIcons

} // namespace phonio
