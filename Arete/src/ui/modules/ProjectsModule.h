#pragma once

#include "core/Module.h"

#include <QWidget>
#include <QListWidget>
#include <QLabel>

class QPushButton;

namespace arete {
namespace app { class AppContext; }
namespace models { struct Project; }

namespace ui {

// Projects tab: one row per project with progress (done/total tasks).
// Projects can be created, renamed and removed from here.
class ProjectsModule : public core::Module
{
    Q_OBJECT

public:
    explicit ProjectsModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("projects"); }
    QString moduleTitle() const override { return QStringLiteral("Projects"); }
    QString moduleIcon() const override { return QString(); }
    void onActivated() override;
    void refreshView() override;

public slots:
    void highlightProject(qint64 projectId);

private:
    void refresh();
    void showEditor(const models::Project* project);

    QListWidget* m_list;
    QLabel* m_summary;
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
};

} // namespace ui
} // namespace arete
