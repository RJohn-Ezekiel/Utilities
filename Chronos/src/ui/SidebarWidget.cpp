#include "SidebarWidget.h"
#include "Theme.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QStyle>

namespace chronos {

SidebarWidget::SidebarWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(180);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addSpacing(8);

    auto addBtn = [&](const QString& text, Page page) {
        auto* btn = createNavButton(text, page);
        m_buttons.append(btn);
        layout->addWidget(btn);
    };

    addBtn(QStringLiteral("Dashboard"), Dashboard);
    addBtn(QStringLiteral("Tasks"), Tasks);
    addBtn(QStringLiteral("History"), History);
    addBtn(QStringLiteral("Statistics"), Statistics);
    addBtn(QStringLiteral("Settings"), Settings);

    layout->addStretch();
    updateButtonStates();
}

QPushButton* SidebarWidget::createNavButton(const QString& text, Page page)
{
    auto* btn = new QPushButton(text, this);
    btn->setCheckable(false);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setMinimumHeight(36);
    btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: transparent;"
        "  color: %1;"
        "  border: none;"
        "  border-left: 3px solid transparent;"
        "  text-align: left;"
        "  padding: 6px 16px 6px 20px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "  background: %2;"
        "  color: %3;"
        "}"
        "QPushButton[active=\"true\"] {"
        "  background: %4;"
        "  border-left: 3px solid %5;"
        "  color: %3;"
        "}"
    )
        .arg(Theme::SecondaryText.name())
        .arg(Theme::Hover.name())
        .arg(Theme::PrimaryText.name())
        .arg(Theme::Selection.name())
        .arg(Theme::Accent.name()));

    connect(btn, &QPushButton::clicked, this, [this, page]() {
        setActivePage(page);
        emit pageSelected(page);
    });

    return btn;
}

void SidebarWidget::setActivePage(Page page)
{
    if (m_activePage == page) return;
    m_activePage = page;
    updateButtonStates();
}

void SidebarWidget::updateButtonStates()
{
    for (int i = 0; i < m_buttons.size(); ++i) {
        bool active = (i == static_cast<int>(m_activePage));
        m_buttons[i]->setProperty("active", active);
        m_buttons[i]->style()->unpolish(m_buttons[i]);
        m_buttons[i]->style()->polish(m_buttons[i]);
    }
}

} // namespace chronos
