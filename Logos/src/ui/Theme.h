#pragma once

#include <QColor>
#include <QString>

namespace Theme {

inline constexpr QColor background    = QColor(0x1B, 0x1B, 0x1B);
inline constexpr QColor sidebar       = QColor(0x23, 0x23, 0x23);
inline constexpr QColor readingArea   = QColor(0x20, 0x20, 0x20);
inline constexpr QColor toolbar       = QColor(0x25, 0x25, 0x26);
inline constexpr QColor statusBar     = QColor(0x25, 0x25, 0x26);
inline constexpr QColor borders       = QColor(0x35, 0x35, 0x35);
inline constexpr QColor primaryText   = QColor(0xD8, 0xD8, 0xD8);
inline constexpr QColor secondaryText = QColor(0xA9, 0xA9, 0xA9);
inline constexpr QColor selected      = QColor(0x3A, 0x3D, 0x41);
inline constexpr QColor hover         = QColor(0x2E, 0x2E, 0x2E);
inline constexpr QColor errorBg       = QColor(0x40, 0x20, 0x20);
inline constexpr QColor successBg     = QColor(0x20, 0x40, 0x20);

[[nodiscard]] inline QColor accent()
{
    return QColor(0x50, 0x57, 0x5B);
}

[[nodiscard]] QString styleSheet();

void apply();

} // namespace Theme
