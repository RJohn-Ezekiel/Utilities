#pragma once

#include <QColor>
#include <QString>

namespace arete::theme {

// ============================================================================
// Shared palette (spec: Background=#111111, Panel=#242424, Border=#353535,
// Text=#A0A0A0, Heading=#C4C4C4, Highlight=#D0D0D0)
// ============================================================================

inline constexpr QColor Background   {0x11, 0x11, 0x11}; // #111111
inline constexpr QColor Panel        {0x24, 0x24, 0x24}; // #242424
inline constexpr QColor Border       {0x35, 0x35, 0x35}; // #353535
inline constexpr QColor Text         {0xA0, 0xA0, 0xA0}; // #A0A0A0
inline constexpr QColor Heading      {0xC4, 0xC4, 0xC4}; // #C4C4C4
inline constexpr QColor Highlight    {0xD0, 0xD0, 0xD0}; // #D0D0D0

// Derived / semantic colors
inline constexpr QColor Surface       = Panel;
inline constexpr QColor SurfaceAlt    = QColor(0x2A, 0x2A, 0x2A); // Slightly lighter panel
inline constexpr QColor InputBg       = QColor(0x2A, 0x2A, 0x2A);
inline constexpr QColor Hover         = QColor(0x2E, 0x2E, 0x2E);
inline constexpr QColor Selection     = QColor(0x3A, 0x3A, 0x3A);
inline constexpr QColor Accent        = Highlight;
inline constexpr QColor AccentDim     = QColor(0xB0, 0xB0, 0xB0);
inline constexpr QColor Success       = QColor(0x6A, 0xA0, 0x6A);
inline constexpr QColor Warning       = QColor(0xC4, 0xA0, 0x50);
inline constexpr QColor Error         = QColor(0xC4, 0x50, 0x50);
inline constexpr QColor ErrorBg       = QColor(0x3A, 0x1A, 0x1A);
inline constexpr QColor FocusBorder   = Highlight;

// ============================================================================
// Stylesheet generator
// ============================================================================

[[nodiscard]] inline QString appStyleSheet()
{
    return QStringLiteral(R"(
        /* Base */
        QMainWindow, QDialog, QWidget {
            background-color: #111111;
            color: #A0A0A0;
            font-family: "JetBrains Mono", "Cascadia Code", "Noto Sans Mono", "Fira Code", monospace;
            font-size: 13px;
        }

        /* Menus */
        QMenuBar {
            background-color: #242424;
            color: #A0A0A0;
            border-bottom: 1px solid #353535;
        }
        QMenuBar::item:selected {
            background-color: #3A3A3A;
        }
        QMenu {
            background-color: #242424;
            color: #A0A0A0;
            border: 1px solid #353535;
        }
        QMenu::item:selected {
            background-color: #3A3A3A;
        }

        /* Toolbars */
        QToolBar {
            background-color: #242424;
            border: none;
            spacing: 4px;
            padding: 2px;
        }

        /* Buttons - purpose-specific, not generic */
        QPushButton {
            background-color: #2A2A2A;
            color: #A0A0A0;
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
        QPushButton:disabled {
            color: #505050;
            border-color: #353535;
        }

        /* Primary action button */
        QPushButton[primary="true"] {
            background-color: #D0D0D0;
            color: #111111;
            border: 1px solid #D0D0D0;
            font-weight: 600;
        }
        QPushButton[primary="true"]:hover {
            background-color: #E0E0E0;
        }
        QPushButton[primary="true"]:pressed {
            background-color: #C0C0C0;
        }

        /* Destructive button */
        QPushButton[destructive="true"] {
            background-color: #3A1A1A;
            color: #C45050;
            border: 1px solid #C45050;
        }
        QPushButton[destructive="true"]:hover {
            background-color: #4A1A1A;
        }

        /* Inputs */
        QLineEdit, QTextEdit, QPlainTextEdit {
            background-color: #2A2A2A;
            color: #A0A0A0;
            border: 1px solid #353535;
            border-radius: 4px;
            padding: 4px 8px;
            selection-background-color: #D0D0D0;
            selection-color: #111111;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
            border-color: #D0D0D0;
        }

        /* Lists / Trees / Tables */
        QListWidget, QTreeWidget, QTableWidget, QListView, QTreeView, QTableView {
            background-color: #111111;
            color: #A0A0A0;
            border: 1px solid #353535;
            outline: none;
            gridline-color: #353535;
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
            color: #D0D0D0;
        }
        QHeaderView::section {
            background-color: #242424;
            color: #C4C4C4;
            border: none;
            border-bottom: 1px solid #353535;
            padding: 6px 10px;
        }

        /* Scrollbars */
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

        /* Tabs */
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
            color: #D0D0D0;
            border-bottom: 2px solid #D0D0D0;
        }
        QTabBar::tab:hover {
            background-color: #2E2E2E;
        }

        /* Progress */
        QProgressBar {
            background-color: #353535;
            color: #A0A0A0;
            border: none;
            border-radius: 4px;
            text-align: center;
            height: 8px;
        }
        QProgressBar::chunk {
            background-color: #D0D0D0;
            border-radius: 4px;
        }

        /* Splitter */
        QSplitter::handle {
            background-color: #353535;
            width: 1px;
        }

        /* Labels */
        QLabel {
            color: #A0A0A0;
        }
        QLabel[heading="true"] {
            color: #C4C4C4;
            font-weight: 600;
        }

        /* ComboBox */
        QComboBox {
            background-color: #2A2A2A;
            color: #A0A0A0;
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
            color: #A0A0A0;
            border: 1px solid #353535;
            selection-background-color: #3A3A3A;
        }

        /* CheckBox / RadioButton */
        QCheckBox, QRadioButton {
            color: #A0A0A0;
            spacing: 8px;
        }
        QCheckBox::indicator, QRadioButton::indicator {
            border: 1px solid #353535;
            background-color: #2A2A2A;
            width: 16px;
            height: 16px;
            border-radius: 3px;
        }
        QCheckBox::indicator:checked {
            background-color: #D0D0D0;
            border: 1px solid #D0D0D0;
        }
        QCheckBox::indicator:checked:hover {
            background-color: #E0E0E0;
        }
        QRadioButton::indicator {
            border-radius: 8px;
        }
        QRadioButton::indicator:checked {
            background-color: #D0D0D0;
            border: 3px solid #D0D0D0;
        }

        /* StatusBar */
        QStatusBar {
            background-color: #242424;
            color: #A0A0A0;
            border-top: 1px solid #353535;
        }

        /* Tooltip */
        QToolTip {
            background-color: #2A2A2A;
            color: #A0A0A0;
            border: 1px solid #353535;
            padding: 4px 8px;
            border-radius: 4px;
        }

        /* Slider */
        QSlider::groove:horizontal {
            background: #353535;
            height: 4px;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #D0D0D0;
            width: 14px;
            height: 14px;
            border-radius: 7px;
            margin: -5px 0;
        }
        QSlider::handle:horizontal:hover {
            background: #E0E0E0;
        }
        QSlider::sub-page:horizontal {
            background: #D0D0D0;
            border-radius: 2px;
        }

        /* SpinBox */
        QSpinBox, QDoubleSpinBox {
            background-color: #2A2A2A;
            color: #A0A0A0;
            border: 1px solid #353535;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QSpinBox:hover, QDoubleSpinBox:hover {
            border-color: #D0D0D0;
        }
        QSpinBox::up-button, QDoubleSpinBox::up-button,
        QSpinBox::down-button, QDoubleSpinBox::down-button {
            background: transparent;
            border: none;
            width: 16px;
        }
        QSpinBox::up-arrow, QDoubleSpinBox::up-arrow,
        QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
            width: 8px;
            height: 8px;
        }

        /* GroupBox */
        QGroupBox {
            color: #C4C4C4;
            border: 1px solid #353535;
            border-radius: 4px;
            margin-top: 12px;
            padding-top: 8px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
        }

        /* DockWidget */
        QDockWidget {
            titlebar-close-icon: url(:/icons/close.png);
            titlebar-normal-icon: url(:/icons/undock.png);
        }
        QDockWidget::title {
            background-color: #242424;
            color: #C4C4C4;
            padding: 6px 10px;
            border-bottom: 1px solid #353535;
        }

        /* ToolButton */
        QToolButton {
            background-color: transparent;
            color: #A0A0A0;
            border: 1px solid transparent;
            padding: 6px;
            border-radius: 4px;
        }
        QToolButton:hover {
            background-color: #2E2E2E;
            border-color: #353535;
        }
        QToolButton:pressed {
            background-color: #3A3A3A;
        }
        QToolButton[popupMode="1"] { /* MenuButtonPopup */
            padding-right: 20px;
        }

        /* Focus indicator for keyboard navigation */
        QWidget:focus {
            outline: none;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus,
        QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus,
        QPushButton:focus, QToolButton:focus,
        QListWidget:focus, QTreeWidget:focus, QTableWidget:focus,
        QListView:focus, QTreeView:focus, QTableView:focus {
            border-color: #D0D0D0;
        }
    )");
}

} // namespace arete::theme