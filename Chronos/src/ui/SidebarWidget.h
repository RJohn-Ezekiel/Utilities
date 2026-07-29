#pragma once

#include <QWidget>
#include <QPushButton>
#include <QList>

namespace chronos {

class SidebarWidget : public QWidget {
    Q_OBJECT

public:
    enum Page {
        Dashboard = 0,
        Tasks,
        History,
        Statistics,
        Settings,
        PageCount
    };

    explicit SidebarWidget(QWidget* parent = nullptr);

    void setActivePage(Page page);

signals:
    void pageSelected(SidebarWidget::Page page);

private:
    QPushButton* createNavButton(const QString& text, Page page);
    void updateButtonStates();

    QList<QPushButton*> m_buttons;
    Page m_activePage = Dashboard;
};

} // namespace chronos
