#pragma once

#include <QColor>
#include <QFont>

namespace chronos::Theme {

// Backgrounds
inline constexpr QColor Background(27, 27, 27);       // #1B1B1B
inline constexpr QColor Panel(35, 35, 35);             // #232323
inline constexpr QColor Toolbar(37, 37, 37);           // #252525
inline constexpr QColor StatusBarBg(37, 37, 37);       // #252525
inline constexpr QColor Card(42, 42, 42);              // #2A2A2A

// Borders
inline constexpr QColor Border(53, 53, 53);            // #353535

// Text
inline constexpr QColor PrimaryText(216, 216, 216);     // #D8D8D8
inline constexpr QColor SecondaryText(169, 169, 169);   // #A9A9A9

// Interactive
inline constexpr QColor Selection(58, 58, 58);          // #3A3A3A
inline constexpr QColor Hover(46, 46, 46);              // #2E2E2E

// Accent — grey
inline constexpr QColor Accent(138, 138, 138);          // #8A8A8A
inline constexpr QColor AccentDim(110, 110, 110);        // darker accent for pressed

// Semantic
inline constexpr QColor Success(122, 122, 122);           // #7A7A7A
inline constexpr QColor Warning(154, 154, 154);          // #9A9A9A
inline constexpr QColor Error(110, 110, 110);             // #6E6E6E

} // namespace chronos::Theme
