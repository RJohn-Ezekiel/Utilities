#pragma once

#include <QMainWindow>
#include <QVector>
#include <QString>
#include <QDate>

class QTabBar;
class QStackedWidget;
class QLabel;
class QCloseEvent;
class QShowEvent;

namespace arete {
namespace app { class AppContext; }
namespace core { class Module; }

namespace ui {

class TopBarWidget;

// The Arete shell: top bar, draggable tab strip, module stack. Owns the
// global shortcuts, palette, universal search, quick capture and the
// task-completion flow.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(app::AppContext* context, QWidget* parent = nullptr);
    ~MainWindow() override;

    [[nodiscard]] app::AppContext* context() const { return m_context; }

    void restoreSession();
    void saveSession();

    // ── Navigation ──
    bool activateModule(const QString& id);
    void sendCommandToModule(const QString& id, const QString& command);
    void showModule(const QString& id);

    // ── Global actions ──
    void openCommandPalette();
    void openUniversalSearch(const QString& initialQuery = {});
    void openQuickCapture(const QString& kind = QStringLiteral("task"));
    void captureNote(const QString& text);
    void showTaskCompletion(qint64 taskId, const QString& taskTitle);

    // ── Cross-module highlighting (from universal search) ──
    void highlightTask(qint64 taskId);
    void highlightProject(qint64 projectId);
    void openJournalDate(const QDate& date);

    // Notifications.
    void notify(const QString& title, const QString& message, int timeoutMs = 5000);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildModules();
    void buildTabBar();
    void setupShortcuts();
    void setupConnections();
    QStringList computeOrder();
    void refreshModuleSet();
    void reorderTo(const QStringList& ids);
    void selectTab(int index);
    void refreshTabLabels();
    QWidget* moduleWidgetAt(int stackIndex) const;

    app::AppContext* m_context;
    TopBarWidget* m_topBar = nullptr;
    QTabBar* m_tabBar = nullptr;
    QStackedWidget* m_stack = nullptr;
    QVector<core::Module*> m_modules;         // parallel to stack
    QStringList m_order;                      // module ids, tab order
    QWidget* m_notificationsPopup = nullptr;
};

} // namespace ui
} // namespace arete
