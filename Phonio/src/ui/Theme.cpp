#include "ui/Theme.h"

#include <QApplication>
#include <QPalette>

namespace phonio {

QColor Theme::m_base = QColor(18, 18, 18);
QColor Theme::m_surface = QColor(26, 26, 26);
QColor Theme::m_surfaceAlt = QColor(35, 35, 35);
QColor Theme::m_border = QColor(46, 46, 46);
QColor Theme::m_textPrimary = QColor(184, 184, 184);
QColor Theme::m_textSecondary = QColor(125, 125, 125);
QColor Theme::m_accent = QColor(216, 216, 216);

void Theme::apply(QApplication& app, const QColor& accentColor)
{
    m_accent = accentColor;

    QPalette palette;
    palette.setColor(QPalette::Window, m_base);
    palette.setColor(QPalette::WindowText, m_textPrimary);
    palette.setColor(QPalette::Base, m_surface);
    palette.setColor(QPalette::AlternateBase, m_surfaceAlt);
    palette.setColor(QPalette::Text, m_textPrimary);
    palette.setColor(QPalette::Button, m_surfaceAlt);
    palette.setColor(QPalette::ButtonText, m_textPrimary);
    palette.setColor(QPalette::Highlight, m_accent.darker(130));
    palette.setColor(QPalette::HighlightedText, QColor(20, 20, 20));
    palette.setColor(QPalette::PlaceholderText, m_textSecondary);
    palette.setColor(QPalette::ToolTipBase, m_surfaceAlt);
    palette.setColor(QPalette::ToolTipText, m_textPrimary);
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(110, 110, 110));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(110, 110, 110));
    app.setPalette(palette);

    app.setStyleSheet(styleSheet(accentColor));
}

QString Theme::styleSheet(const QColor& accent)
{
    const QString accentCss = accent.name();
    return QStringLiteral(R"(
QWidget {
    font-size: 13px;
}
QMainWindow, QWidget#centralRoot {
    background-color: %1;
}
QFrame#sidebar {
    background-color: %2;
    border-right: 1px solid %3;
}
QLabel#appTitle {
    font-size: 17px;
    font-weight: 600;
    color: %4;
    letter-spacing: 1px;
}
QListWidget#sidebarList {
    background: transparent;
    border: none;
    outline: none;
    padding: 6px;
}
QListWidget#sidebarList::item {
    padding: 9px 14px;
    border-radius: 8px;
    margin: 2px 4px;
    color: %5;
}
QListWidget#sidebarList::item:hover {
    background-color: rgba(184,184,184,0.05);
}
QListWidget#sidebarList::item:selected {
    background-color: %2;
    color: %4;
}
QStackedWidget#contentStack {
    background-color: %1;
}
QTableView {
    background-color: transparent;
    border: none;
    gridline-color: transparent;
    color: %4;
    selection-background-color: rgba(184,184,184,0.08);
    selection-color: %4;
    outline: none;
}
QTableView::item {
    padding-left: 8px;
}
QHeaderView {
    background: transparent;
    border: none;
    color: %5;
    font-weight: 600;
}
QHeaderView::section {
    background: transparent;
    border: none;
    padding: 8px 8px;
    border-bottom: 1px solid %3;
}
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 2px;
}
QScrollBar::handle:vertical {
    background: rgba(184,184,184,0.15);
    border-radius: 4px;
    min-height: 30px;
}
QScrollBar::handle:vertical:hover {
    background: rgba(184,184,184,0.28);
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar:horizontal {
    background: transparent;
    height: 10px;
    margin: 2px;
}
QScrollBar::handle:horizontal {
    background: rgba(184,184,184,0.15);
    border-radius: 4px;
    min-width: 30px;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}
QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
    background-color: %6;
    border: 1px solid %3;
    border-radius: 8px;
    padding: 6px 10px;
    selection-background-color: %7;
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus {
    border: 1px solid %7;
}
QComboBox::drop-down {
    border: none;
    width: 22px;
}
QComboBox QAbstractItemView {
    background-color: %6;
    border: 1px solid %3;
    border-radius: 8px;
    selection-background-color: rgba(184,184,184,0.10);
    selection-color: %4;
    outline: none;
}
QPushButton {
    background-color: %6;
    border: 1px solid %3;
    border-radius: 8px;
    padding: 7px 16px;
    color: %4;
}
QPushButton:hover {
    background-color: rgba(184,184,184,0.08);
}
QPushButton:pressed {
    background-color: rgba(184,184,184,0.12);
}
QPushButton#accentButton {
    background-color: %7;
    color: #141414;
    border: none;
    font-weight: 600;
}
QPushButton#accentButton:hover {
    background-color: %7;
}
QToolButton#transportButton {
    border: none;
    background: transparent;
    border-radius: 20px;
    padding: 6px;
}
QToolButton#transportButton:hover {
    background-color: rgba(184,184,184,0.08);
}
QToolButton#transportButton:checked {
    color: %7;
}
QSlider::groove:horizontal {
    height: 4px;
    border-radius: 2px;
    background: rgba(184,184,184,0.14);
}
QSlider::sub-page:horizontal {
    background: %7;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: %4;
    border: none;
    width: 12px;
    height: 12px;
    margin: -4px 0;
    border-radius: 6px;
}
QSlider::handle:horizontal:hover {
    background: %7;
}
QSlider::groove:vertical {
    width: 4px;
    border-radius: 2px;
    background: rgba(184,184,184,0.14);
}
QSlider::sub-page:vertical {
    background: %7;
    border-radius: 2px;
}
QSlider::handle:vertical {
    background: %4;
    border: none;
    width: 12px;
    height: 12px;
    margin: 0 -4px;
    border-radius: 6px;
}
QMenu {
    background-color: %6;
    border: 1px solid %3;
    border-radius: 10px;
    padding: 6px;
}
QMenu::item {
    padding: 7px 28px 7px 12px;
    border-radius: 6px;
    color: %4;
}
QMenu::item:selected {
    background-color: rgba(184,184,184,0.10);
}
QMenu::separator {
    height: 1px;
    background: %3;
    margin: 6px 8px;
}
QDialog {
    background-color: %1;
}
QGroupBox {
    border: 1px solid %3;
    border-radius: 10px;
    margin-top: 12px;
    padding-top: 8px;
    color: %5;
    font-weight: 600;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 4px;
}
QListWidget#browseList, QListWidget#playlistNav, QListWidget#settingsList {
    background: transparent;
    border: none;
    outline: none;
}
QListWidget#browseList::item, QListWidget#playlistNav::item {
    padding: 8px 12px;
    border-radius: 8px;
    margin: 2px;
}
QListWidget#browseList::item:hover, QListWidget#playlistNav::item:hover {
    background-color: rgba(184,184,184,0.05);
}
QListWidget#browseList::item:selected, QListWidget#playlistNav::item:selected {
    background-color: rgba(184,184,184,0.10);
}
QScrollArea {
    background: transparent;
    border: none;
}
QLabel#pageTitle {
    font-size: 22px;
    font-weight: 700;
    color: %4;
}
QLabel#pageSubtitle {
    font-size: 12px;
    color: %5;
}
QLabel#sectionHeader {
    font-size: 13px;
    font-weight: 700;
    color: %4;
}
QWidget#playbackBar {
    background-color: %2;
    border-top: 1px solid %3;
}
QLabel#nowPlayingTitle {
    font-size: 15px;
    font-weight: 600;
    color: %4;
}
QLabel#nowPlayingArtist {
    font-size: 12px;
    color: %5;
}
QLabel#bigTitle {
    font-size: 24px;
    font-weight: 700;
    color: %4;
}
QLabel#metaLabel {
    font-size: 13px;
    color: %5;
}
QLabel#metaValue {
    font-size: 13px;
    color: %4;
}
QLabel#placeholder {
    color: %5;
}
QStatusBar {
    background: %2;
    color: %5;
    border-top: 1px solid %3;
}
QProgressBar {
    background-color: %6;
    border: none;
    border-radius: 4px;
    height: 6px;
    text-align: center;
}
QProgressBar::chunk {
    background-color: %7;
    border-radius: 4px;
}
QCheckBox {
    spacing: 8px;
    color: %4;
}
QCheckBox::indicator {
    width: 16px;
    height: 16px;
    border-radius: 4px;
    border: 1px solid %3;
    background: %6;
}
QCheckBox::indicator:checked {
    background-color: %7;
    border: 1px solid %7;
}
QToolTip {
    background-color: %6;
    color: %4;
    border: 1px solid %3;
    border-radius: 6px;
    padding: 4px 8px;
}
QListWidget#albumGrid {
    background: transparent;
    border: none;
    outline: none;
}
QListWidget#albumGrid::item {
    border-radius: 10px;
    margin: 4px;
    padding: 10px;
}
QListWidget#albumGrid::item:hover {
    background-color: rgba(184,184,184,0.05);
}
QListWidget#albumGrid::item:selected {
    background-color: rgba(184,184,184,0.10);
}
)").arg(m_base.name(QColor::HexRgb), m_surface.name(QColor::HexRgb), m_border.name(QColor::HexRgb),
          m_textPrimary.name(QColor::HexRgb), m_textSecondary.name(QColor::HexRgb),
          m_surfaceAlt.name(QColor::HexRgb), accentCss);
}

} // namespace phonio
