#pragma once

#include "core/Module.h"

#include <QWidget>
#include <QVector>

class QLabel;
class QPushButton;
class QListWidget;
class QListWidgetItem;

namespace arete {
namespace app { class AppContext; }
namespace models { struct Habit; }

namespace ui {

// Habits tab (Crona-inspired): daily habit completion, weekly targets,
// streaks, and a seven-day history strip per habit.
class HabitsModule : public core::Module
{
    Q_OBJECT

public:
    explicit HabitsModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("habits"); }
    QString moduleTitle() const override { return QStringLiteral("Habits"); }
    QString moduleIcon() const override { return QString(); }
    void onActivated() override;
    void refreshView() override;
    void handleCommand(const QString& command) override;

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onItemActivated(QListWidgetItem* item);

private:
    void rebuild();
    void showEditor(const models::Habit* habit);

    QLabel* m_summaryLabel;
    QListWidget* m_list;
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
};

} // namespace ui
} // namespace arete