#pragma once

#include <QColor>
#include <QFont>

namespace chronos::Theme {

// Shared Arete palette (spec)
inline constexpr QColor Background(0x11, 0x11, 0x11);    // #111111
inline constexpr QColor Panel(0x24, 0x24, 0x24);          // #242424
inline constexpr QColor Toolbar(0x24, 0x24, 0x24);        // #242424
inline constexpr QColor StatusBarBg(0x24, 0x24, 0x24);    // #242424
inline constexpr QColor Card(0x2E, 0x2E, 0x2E);           // slightly lighter panel

// Borders
inline constexpr QColor Border(53, 53, 53);               // #353535

// Text
inline constexpr QColor PrimaryText(0xC4, 0xC4, 0xC4);    // #C4C4C4 heading
inline constexpr QColor SecondaryText(0xA0, 0xA0, 0xA0);  // #A0A0A0 text

// Interactive
inline constexpr QColor Selection(0x3A, 0x3A, 0x3A);
inline constexpr QColor Hover(0x2E, 0x2E, 0x2E);

// Accent — light grey
inline constexpr QColor Accent(0xD0, 0xD0, 0xD0);         // #D0D0D0 highlight
inline constexpr QColor AccentDim(0xB0, 0xB0, 0xB0);

// Semantic
inline constexpr QColor Success(0x6A, 0xA0, 0x6A);
inline constexpr QColor Warning(0xC4, 0xA0, 0x50);
inline constexpr QColor Error(0xC4, 0x50, 0x50);

} // namespace chronos::Theme