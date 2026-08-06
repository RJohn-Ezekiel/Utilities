#pragma once

#include "core/Module.h"

#include <QWidget>
#include <QDate>
#include <QVector>

class QLabel;
class QGridLayout;
class QPushButton;

namespace arete {
namespace app { class AppContext; }
namespace models { struct CalendarEvent; }

namespace ui {

// Calendar tab: a month grid with event counts per day, day selection and
// a side panel with that day's time blocks.
class CalendarModule : public core::Module
{
    Q_OBJECT

public:
    explicit CalendarModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("calendar"); }
    QString moduleTitle() const override { return QStringLiteral("Calendar"); }
    QString moduleIcon() const override { return QString(); }
    void onActivated() override;
    void refreshView() override;
    void handleCommand(const QString& command) override;

public slots:
    void openDate(const QDate& date);

private:
    void rebuildGrid();
    void refreshSidePanel();
    void showEventEditor(const models::CalendarEvent* existing = nullptr,
                         const QDate& prefill = QDate());

    QDate m_cursor = QDate::currentDate();
    QDate m_selected = QDate::currentDate();
    QLabel* m_monthLabel;
    QGridLayout* m_grid;
    QLabel* m_dayLabel;
    QLabel* m_dayList;
};

} // namespace ui
} // namespace arete
