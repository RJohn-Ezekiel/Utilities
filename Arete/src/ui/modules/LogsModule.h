#pragma once

#include "core/Module.h"
#include "arete/logging/Logger.h"

#include <QWidget>

class QLabel;
class QVBoxLayout;
class QPushButton;

namespace arete {
namespace app { class AppContext; }

namespace ui {

// Logs tab: everything Arete and Qt print (INFO/WARNING/ERROR), routed here
// instead of the terminal. Live-updating, with a Clear control.
class LogsModule : public core::Module
{
    Q_OBJECT

public:
    explicit LogsModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("logs"); }
    QString moduleTitle() const override { return QStringLiteral("Logs"); }
    QString moduleIcon() const override { return QString(); }
    void onActivated() override;
    void refreshView() override;

private:
    void rebuildList();
    QString levelName(arete::logging::Level level) const;
    QString levelColor(arete::logging::Level level) const;

    arete::logging::LogModel* m_model;
    QWidget* m_listHost;
    QVBoxLayout* m_listLayout;
    QLabel* m_emptyLabel;
};

} // namespace ui
} // namespace arete