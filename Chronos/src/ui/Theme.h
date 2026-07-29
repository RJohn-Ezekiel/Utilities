#pragma once

#include <QColor>
#include <QFont>

namespace chronos::Theme {

// Backgrounds
inline constexpr QColor Background(27, 27, 27);       // #1B1B1B
inline constexpr QColor Panel(35, 35, 35);             // #232323
inline constexpr QColor Toolbar(37, 37, 38);           // #252526
inline constexpr QColor StatusBarBg(37, 37, 38);       // #252526
inline constexpr QColor Card(42, 42, 42);              // #2A2A2A

// Borders
inline constexpr QColor Border(53, 53, 53);            // #353535

// Text
inline constexpr QColor PrimaryText(216, 216, 216);     // #D8D8D8
inline constexpr QColor SecondaryText(169, 169, 169);   // #A9A9A9

// Interactive
inline constexpr QColor Selection(58, 61, 65);          // #3A3D41
inline constexpr QColor Hover(46, 46, 46);              // #2E2E2E

// Accent — muted blue-grey
inline constexpr QColor Accent(122, 138, 154);          // #7A8A9A
inline constexpr QColor AccentDim(90, 102, 114);        // darker accent for pressed

// Semantic
inline constexpr QColor Success(90, 138, 90);           // #5A8A5A
inline constexpr QColor Warning(184, 160, 96);          // #B8A060
inline constexpr QColor Error(138, 74, 74);             // #8A4A4A

} // namespace chronos::Theme
