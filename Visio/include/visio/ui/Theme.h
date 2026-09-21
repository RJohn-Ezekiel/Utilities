#pragma once

#include <QColor>
#include <QString>

namespace visio::Theme {

// ============================================================================
// Common dark palette shared across Chronos, Codex, Logos, and Ytsurf
// ============================================================================

// -- Backgrounds --
inline constexpr QColor background  {0x11, 0x11, 0x11}; // #111111 // Main window / primary bg
inline constexpr QColor panel       {0x24, 0x24, 0x24}; // #242424 // Panel / sidebar / card
inline constexpr QColor readingArea {0x18, 0x18, 0x18}; // Reading pane / video list
inline constexpr QColor toolbar     {0x24, 0x24, 0x24}; // #242424 // Toolbar, status bar, menu bar
inline constexpr QColor input       {0x33, 0x33, 0x33}; // Input fields (search bar)
inline constexpr QColor hover       {0x2E, 0x2E, 0x2E}; // Hover state (buttons, list items)
inline constexpr QColor selection   {0x3A, 0x3A, 0x3A}; // Selected item highlight

// -- Borders --
inline constexpr QColor border      {0x35, 0x35, 0x35}; // Borders, separators, dividers

// -- Text --
inline constexpr QColor primaryText   {0xC4, 0xC4, 0xC4}; // #C4C4C4 // Primary / heading text
inline constexpr QColor secondaryText {0xA0, 0xA0, 0xA0}; // #A0A0A0 // Secondary / muted text

// -- Accent --
inline constexpr QColor accent      {0xD0, 0xD0, 0xD0}; // #D0D0D0 // Accent — grey
inline constexpr QColor accentDim   {0xB0, 0xB0, 0xB0}; // Accent dimmed (pressed)

// -- Semantic --
inline constexpr QColor success     {0x6A, 0xA0, 0x6A}; // Success / positive
inline constexpr QColor warning     {0xC4, 0xA0, 0x50}; // Warning / caution
inline constexpr QColor error       {0xC4, 0x50, 0x50}; // Error / destructive
inline constexpr QColor errorBg     {0x3A, 0x1A, 0x1A}; // Error background

// ============================================================================
// Ytsurf-specific semantic tokens
// ============================================================================

// Video card / result list
inline constexpr QColor videoCardBg       {0x24, 0x24, 0x24};
inline constexpr QColor videoCardHover    {0x2E, 0x2E, 0x2E};
inline constexpr QColor videoTitle        {0xC4, 0xC4, 0xC4};
inline constexpr QColor videoChannel      {0xA0, 0xA0, 0xA0};
inline constexpr QColor videoDuration     {0xA0, 0xA0, 0xA0};
inline constexpr QColor videoViews        {0xA0, 0xA0, 0xA0};

// Progress indicators
inline constexpr QColor progressFg        {0xD0, 0xD0, 0xD0};
inline constexpr QColor progressBg        {0x35, 0x35, 0x35};

// Queue / playlist
inline constexpr QColor queueItemBg       {0x24, 0x24, 0x24};
inline constexpr QColor playingHighlight  {0x3A, 0x3A, 0x3A};

// ============================================================================
// Stylesheet helper
// ============================================================================

/// Returns a complete Qt stylesheet using the palette above.
[[nodiscard]] inline QString appStyleSheet()
{
    return QStringLiteral(R"(
        QMainWindow, QWidget {
            background-color: #111111;
            color: #C4C4C4;
        }
        QMenuBar {
            background-color: #242424;
            color: #C4C4C4;
            border-bottom: 1px solid #353535;
        }
        QMenuBar::item:selected {
            background-color: #3A3A3A;
        }
        QMenu {
            background-color: #242424;
            color: #C4C4C4;
            border: 1px solid #353535;
        }
        QMenu::item:selected {
            background-color: #3A3A3A;
        }
        QToolBar {
            background-color: #242424;
            border: none;
            spacing: 4px;
        }
        QStatusBar {
            background-color: #242424;
            color: #A0A0A0;
            border-top: 1px solid #353535;
        }
        QPushButton {
            background-color: #2E2E2E;
            color: #C4C4C4;
            border: 1px solid #353535;
            padding: 6px 16px;
            border-radius: 4px;
        }
        QPushButton:hover {
            background-color: #2E2E2E;
            border-color: #D0D0D0;
        }
        QPushButton:pressed {
            background-color: #3A3A3A;
        }
        QLineEdit, QTextEdit, QPlainTextEdit {
            background-color: #333333;
            color: #C4C4C4;
            border: 1px solid #353535;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
            border-color: #D0D0D0;
        }
        QListWidget, QTreeWidget, QListView {
            background-color: #111111;
            color: #C4C4C4;
            border: none;
            outline: none;
        }
        QListWidget::item, QTreeWidget::item {
            padding: 6px 10px;
            border-radius: 4px;
        }
        QListWidget::item:hover, QTreeWidget::item:hover {
            background-color: #2E2E2E;
        }
        QListWidget::item:selected, QTreeWidget::item:selected {
            background-color: #3A3A3A;
            color: #C4C4C4;
        }
        QScrollBar:vertical {
            background: #111111;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #353535;
            min-height: 30px;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical:hover {
            background: #D0D0D0;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar:horizontal {
            background: #111111;
            height: 10px;
        }
        QScrollBar::handle:horizontal {
            background: #353535;
            min-width: 30px;
            border-radius: 5px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #D0D0D0;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0;
        }
        QTabWidget::pane {
            border: 1px solid #353535;
            background-color: #111111;
        }
        QTabBar::tab {
            background-color: #242424;
            color: #A0A0A0;
            padding: 8px 20px;
            border: 1px solid #353535;
            border-bottom: none;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
        QTabBar::tab:selected {
            background-color: #111111;
            color: #C4C4C4;
            border-bottom: 2px solid #D0D0D0;
        }
        QTabBar::tab:hover {
            background-color: #2E2E2E;
        }
        QProgressBar {
            background-color: #353535;
            color: #C4C4C4;
            border: none;
            border-radius: 4px;
            text-align: center;
            height: 8px;
        }
        QProgressBar::chunk {
            background-color: #D0D0D0;
            border-radius: 4px;
        }
        QSplitter::handle {
            background-color: #353535;
            width: 1px;
        }
        QLabel {
            color: #C4C4C4;
        }
        QComboBox {
            background-color: #333333;
            color: #C4C4C4;
            border: 1px solid #353535;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QComboBox:hover {
            border-color: #D0D0D0;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background-color: #242424;
            color: #C4C4C4;
            border: 1px solid #353535;
            selection-background-color: #3A3A3A;
        }
        QCheckBox {
            color: #C4C4C4;
            spacing: 8px;
        }
        QCheckBox::indicator:unchecked {
            border: 1px solid #353535;
            background-color: #333333;
            width: 16px;
            height: 16px;
            border-radius: 3px;
        }
        QCheckBox::indicator:checked {
            background-color: #D0D0D0;
            border: 1px solid #D0D0D0;
            width: 16px;
            height: 16px;
            border-radius: 3px;
        }
    )");
}

} // namespace visio::Theme
