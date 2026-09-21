#pragma once

#include <QString>

namespace codex {
namespace style {

// Colour palette — shared Arete palette
inline constexpr auto BG_PRIMARY    = "#111111";
inline constexpr auto BG_SECONDARY  = "#242424";
inline constexpr auto BG_TERTIARY   = "#333333";
inline constexpr auto BG_INPUT      = "#333333";
inline constexpr auto TEXT_PRIMARY  = "#C4C4C4";
inline constexpr auto TEXT_SECONDARY = "#A0A0A0";
inline constexpr auto ACCENT       = "#D0D0D0";
inline constexpr auto ACCENT_HOVER = "#B0B0B0";
inline constexpr auto BORDER       = "#353535";
inline constexpr auto RED          = "#C45050";
inline constexpr auto GREEN        = "#6AA06A";
inline constexpr auto YELLOW       = "#C4A050";

inline QString appStyleSheet()
{
    return QStringLiteral(R"(
        QMainWindow, QDialog, QWidget {
            background-color: %1;
            color: %2;
            font-family: "JetBrains Mono", "Cascadia Code", "Noto Sans Mono", "Fira Code", monospace;
            font-size: 13px;
        }
        QMenuBar {
            background-color: %3;
            border-bottom: 1px solid %5;
        }
        QMenuBar::item:selected {
            background-color: %4;
        }
        QMenu {
            background-color: %3;
            border: 1px solid %5;
        }
        QMenu::item:selected {
            background-color: %4;
        }
        QToolBar {
            background-color: %3;
            border-bottom: 1px solid %5;
            spacing: 4px;
            padding: 2px;
        }
        QPushButton {
            background-color: %4;
            color: %2;
            border: 1px solid %5;
            padding: 4px 12px;
            border-radius: 3px;
        }
        QPushButton:hover {
            background-color: %6;
        }
        QPushButton:pressed {
            background-color: %4;
        }
        QTreeView, QListView, QListWidget {
            background-color: %1;
            color: %2;
            border: 1px solid %5;
            outline: none;
        }
        QTreeView::item:selected, QListView::item:selected, QListWidget::item:selected {
            background-color: %4;
            color: %2;
        }
        QTreeView::item:hover, QListView::item:hover, QListWidget::item:hover {
            background-color: %3;
        }
        QPlainTextEdit, QTextEdit, QLineEdit {
            background-color: %7;
            color: %2;
            border: 1px solid %5;
            padding: 4px;
            selection-background-color: %4;
        }
        QSplitter::handle {
            background-color: %5;
            width: 1px;
        }
        QStatusBar {
            background-color: %3;
            border-top: 1px solid %5;
            color: %8;
        }
        QLabel {
            color: %2;
        }
        QScrollBar:vertical {
            background-color: %1;
            width: 8px;
        }
        QScrollBar::handle:vertical {
            background-color: %5;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: %4;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background-color: %1;
            height: 8px;
        }
        QScrollBar::handle:horizontal {
            background-color: %5;
            min-width: 20px;
            border-radius: 4px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: %4;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        QTabWidget::pane {
            border: 1px solid %5;
            background-color: %1;
        }
        QTabBar::tab {
            background-color: %3;
            color: %8;
            border: 1px solid %5;
            padding: 4px 10px;
        }
        QTabBar::tab:selected {
            background-color: %1;
            color: %2;
        }
        QDialog {
            background-color: %3;
        }
    )")
    .arg(BG_PRIMARY, TEXT_PRIMARY, BG_SECONDARY, ACCENT, BORDER,
         ACCENT_HOVER, BG_INPUT, TEXT_SECONDARY);
}

} // namespace style
} // namespace codex
