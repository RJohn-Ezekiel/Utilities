#pragma once

#include <QColor>
#include <QString>

namespace visio::Theme {

// ============================================================================
// Common dark palette shared across Chronos, Codex, Logos, and Ytsurf
// ============================================================================

// -- Backgrounds --
inline constexpr QColor background  {0x1B, 0x1B, 0x1B}; // Main window / primary bg
inline constexpr QColor panel       {0x23, 0x23, 0x23}; // Panel / sidebar / card
inline constexpr QColor readingArea {0x20, 0x20, 0x20}; // Reading pane / video list
inline constexpr QColor toolbar     {0x25, 0x25, 0x26}; // Toolbar, status bar, menu bar
inline constexpr QColor input       {0x33, 0x33, 0x33}; // Input fields (search bar)
inline constexpr QColor hover       {0x2E, 0x2E, 0x2E}; // Hover state (buttons, list items)
inline constexpr QColor selection   {0x3A, 0x3D, 0x41}; // Selected item highlight

// -- Borders --
inline constexpr QColor border      {0x35, 0x35, 0x35}; // Borders, separators, dividers

// -- Text --
inline constexpr QColor primaryText   {0xD8, 0xD8, 0xD8}; // Primary / heading text
inline constexpr QColor secondaryText {0xA9, 0xA9, 0xA9}; // Secondary / muted text

// -- Accent --
inline constexpr QColor accent      {0x7A, 0x8A, 0x9A}; // Accent — muted blue-grey
inline constexpr QColor accentDim   {0x5A, 0x66, 0x72}; // Accent dimmed (pressed)

// -- Semantic --
inline constexpr QColor success     {0x5A, 0x8A, 0x5A}; // Success / positive
inline constexpr QColor warning     {0xB8, 0xA0, 0x60}; // Warning / caution
inline constexpr QColor error       {0x8A, 0x4A, 0x4A}; // Error / destructive
inline constexpr QColor errorBg     {0x40, 0x20, 0x20}; // Error background

// ============================================================================
// Ytsurf-specific semantic tokens
// ============================================================================

// Video card / result list
inline constexpr QColor videoCardBg       {0x23, 0x23, 0x23};
inline constexpr QColor videoCardHover    {0x2A, 0x2A, 0x2A};
inline constexpr QColor videoTitle        {0xD8, 0xD8, 0xD8};
inline constexpr QColor videoChannel      {0xA9, 0xA9, 0xA9};
inline constexpr QColor videoDuration     {0x7A, 0x8A, 0x9A};
inline constexpr QColor videoViews        {0x9E, 0x9E, 0x9E};

// Progress indicators
inline constexpr QColor progressFg        {0x7A, 0x8A, 0x9A};
inline constexpr QColor progressBg        {0x35, 0x35, 0x35};

// Queue / playlist
inline constexpr QColor queueItemBg       {0x25, 0x25, 0x26};
inline constexpr QColor playingHighlight  {0x3A, 0x3D, 0x41};

// ============================================================================
// Stylesheet helper
// ============================================================================

/// Returns a complete Qt stylesheet using the palette above.
[[nodiscard]] inline QString appStyleSheet()
{
    return QStringLiteral(R"(
        QMainWindow, QWidget {
            background-color: #1B1B1B;
            color: #D8D8D8;
        }
        QMenuBar {
            background-color: #252526;
            color: #D8D8D8;
            border-bottom: 1px solid #353535;
        }
        QMenuBar::item:selected {
            background-color: #3A3D41;
        }
        QMenu {
            background-color: #252526;
            color: #D8D8D8;
            border: 1px solid #353535;
        }
        QMenu::item:selected {
            background-color: #3A3D41;
        }
        QToolBar {
            background-color: #252526;
            border: none;
            spacing: 4px;
        }
        QStatusBar {
            background-color: #252526;
            color: #A9A9A9;
            border-top: 1px solid #353535;
        }
        QPushButton {
            background-color: #333333;
            color: #D8D8D8;
            border: 1px solid #353535;
            padding: 6px 16px;
            border-radius: 4px;
        }
        QPushButton:hover {
            background-color: #2E2E2E;
            border-color: #7A8A9A;
        }
        QPushButton:pressed {
            background-color: #3A3D41;
        }
        QLineEdit, QTextEdit, QPlainTextEdit {
            background-color: #333333;
            color: #D8D8D8;
            border: 1px solid #353535;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
            border-color: #7A8A9A;
        }
        QListWidget, QTreeWidget, QListView {
            background-color: #1B1B1B;
            color: #D8D8D8;
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
            background-color: #3A3D41;
            color: #D8D8D8;
        }
        QScrollBar:vertical {
            background: #1B1B1B;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #353535;
            min-height: 30px;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical:hover {
            background: #7A8A9A;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar:horizontal {
            background: #1B1B1B;
            height: 10px;
        }
        QScrollBar::handle:horizontal {
            background: #353535;
            min-width: 30px;
            border-radius: 5px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #7A8A9A;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0;
        }
        QTabWidget::pane {
            border: 1px solid #353535;
            background-color: #1B1B1B;
        }
        QTabBar::tab {
            background-color: #252526;
            color: #A9A9A9;
            padding: 8px 20px;
            border: 1px solid #353535;
            border-bottom: none;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
        QTabBar::tab:selected {
            background-color: #1B1B1B;
            color: #D8D8D8;
            border-bottom: 2px solid #7A8A9A;
        }
        QTabBar::tab:hover {
            background-color: #2E2E2E;
        }
        QProgressBar {
            background-color: #353535;
            color: #D8D8D8;
            border: none;
            border-radius: 4px;
            text-align: center;
            height: 8px;
        }
        QProgressBar::chunk {
            background-color: #7A8A9A;
            border-radius: 4px;
        }
        QSplitter::handle {
            background-color: #353535;
            width: 1px;
        }
        QLabel {
            color: #D8D8D8;
        }
        QComboBox {
            background-color: #333333;
            color: #D8D8D8;
            border: 1px solid #353535;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QComboBox:hover {
            border-color: #7A8A9A;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background-color: #252526;
            color: #D8D8D8;
            border: 1px solid #353535;
            selection-background-color: #3A3D41;
        }
        QCheckBox {
            color: #D8D8D8;
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
            background-color: #7A8A9A;
            border: 1px solid #7A8A9A;
            width: 16px;
            height: 16px;
            border-radius: 3px;
        }
    )");
}

} // namespace visio::Theme
