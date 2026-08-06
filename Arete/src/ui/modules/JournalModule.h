#pragma once

#include "core/Module.h"

#include <QWidget>
#include <QDate>

class QDateEdit;
class QTextEdit;
class QLabel;

namespace arete {
namespace app { class AppContext; }

namespace ui {

// Journal tab: a date navigator plus six fields (reflection, gratitude,
// mistakes, lessons, scripture, tomorrow) saved to the day's entry.
class JournalModule : public core::Module
{
    Q_OBJECT

public:
    explicit JournalModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("journal"); }
    QString moduleTitle() const override { return QStringLiteral("Journal"); }
    QString moduleIcon() const override { return QString(); }
    void onActivated() override;
    void refreshView() override;

public slots:
    void openDate(const QDate& date);

private:
    void load(const QDate& date);
    void saveCurrent();
    void setDateLabel();

    QDate m_date = QDate::currentDate();
    QDateEdit* m_dateEdit;
    QLabel* m_dateLabel;
    QLabel* m_streak;
    QTextEdit* m_reflection;
    QTextEdit* m_gratitude;
    QTextEdit* m_mistakes;
    QTextEdit* m_lessons;
    QTextEdit* m_scripture;
    QTextEdit* m_tomorrow;
};

} // namespace ui
} // namespace arete
