#pragma once

#include "core/Module.h"

#include <QWidget>
#include <QHash>

class QListWidget;

namespace arete {
namespace app { class AppContext; }

namespace ui {

class KanbanColumn;

// Kanban tab: three boards — Active / Today / Done — with drag-and-drop
// between columns, per-column Add buttons, double-click to rename and a
// right-click menu (rename / delete).
class KanbanModule : public core::Module
{
    Q_OBJECT

public:
    explicit KanbanModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("kanban"); }
    QString moduleTitle() const override { return QStringLiteral("Kanban"); }
    QString moduleIcon() const override { return QString(); }
    void onActivated() override;
    void refreshView() override;

private:
    void rebuild();
    void addTask(const QString& columnName);
    void renameTask(qint64 taskId);
    void deleteTask(qint64 taskId);

    QHash<QString, KanbanColumn*> m_columns;
};

} // namespace ui
} // namespace arete