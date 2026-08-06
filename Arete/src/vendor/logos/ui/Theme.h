#pragma once

#include <QColor>
#include <QString>

namespace Theme {

// Shared Arete palette (spec)
inline constexpr QColor background    = QColor(0x11, 0x11, 0x11); // #111111
inline constexpr QColor sidebar       = QColor(0x24, 0x24, 0x24); // #242424 panel
inline constexpr QColor readingArea   = QColor(0x18, 0x18, 0x18); // #181818
inline constexpr QColor toolbar       = QColor(0x24, 0x24, 0x24);
inline constexpr QColor statusBar     = QColor(0x24, 0x24, 0x24);
inline constexpr QColor input         = QColor(0x33, 0x33, 0x33); // #333333 input fields
inline constexpr QColor borders       = QColor(0x35, 0x35, 0x35); // #353535
inline constexpr QColor primaryText   = QColor(0xC4, 0xC4, 0xC4); // #C4C4C4 heading
inline constexpr QColor secondaryText = QColor(0xA0, 0xA0, 0xA0); // #A0A0A0 text
inline constexpr QColor selected      = QColor(0x3A, 0x3A, 0x3A); // #3A3A3A selection
inline constexpr QColor hover         = QColor(0x2E, 0x2E, 0x2E); // #2E2E2E
inline constexpr QColor accentDim     = QColor(0xB0, 0xB0, 0xB0); // #B0B0B0 accent dimmed
inline constexpr QColor success       = QColor(0x6A, 0xA0, 0x6A); // #6AA06A
inline constexpr QColor warning       = QColor(0xC4, 0xA0, 0x50); // #C4A050
inline constexpr QColor error         = QColor(0xC4, 0x50, 0x50); // #C45050

[[nodiscard]] inline QColor accent()
{
    return QColor(0xD0, 0xD0, 0xD0);
}

[[nodiscard]] QString styleSheet();

void apply();

} // namespace Theme