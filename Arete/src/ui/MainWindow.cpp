#include "ui/MainWindow.h"
#include "app/AppContext.h"
#include "core/Module.h"
#include "core/ModuleRegistry.h"
#include "core/Icons.h"
#include "ui/TopBarWidget.h"
#include "ui/common/EmptyStateWidget.h"
#include "ui/dialogs/QuickCaptureDialog.h"
#include "ui/dialogs/TaskCompleteDialog.h"
#include "ui/dialogs/UniversalSearchDialog.h"
#include "ui/modules/HomeModule.h"
#include "ui/modules/TodayModule.h"
#include "ui/modules/ProjectsModule.h"
#include "ui/modules/KanbanModule.h"
#include "ui/modules/CalendarModule.h"
#include "ui/modules/ChronosModule.h"
#include "ui/modules/CodexModule.h"
#include "ui/modules/LogosModule.h"
#include "ui/modules/PhonioModule.h"
#include "ui/modules/JournalModule.h"
#include "ui/modules/HabitsModule.h"
#include "ui/modules/WellbeingModule.h"
#include "ui/modules/StatisticsModule.h"
#include "ui/modules/AlertsModule.h"
#include "ui/modules/LogsModule.h"
#include "ui/modules/OpsModule.h"
#include "ui/modules/HelpModule.h"
#include "ui/modules/ModulesPage.h"
#include "ui/modules/SettingsModule.h"
#include "services/CommandRunner.h"
#include "services/SearchService.h"
#include "services/TimerService.h"
#include "services/VerseService.h"
#include "data/TaskRepository.h"
#include "data/JournalRepository.h"
#include "models/Task.h"
#include "models/JournalEntry.h"
#include "arete/settings/SettingsManager.h"
#include "arete/widgets/CommandPalette.h"
#include "arete/widgets/NotificationsPanel.h"
#include "arete/notifications/NotificationCenter.h"
#include "arete/logging/Logger.h"
#include "core/vault.h"

#include <QTabBar>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QShortcut>
#include <QKeySequence>
#include <QCloseEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QDate>

using arete::logging::Logger;

namespace arete::ui {

namespace {
// Core tabs, in a fixed order: they cannot be removed or reordered.
// Home first, Modules second-to-last, Settings last; Chronos sits after
// Home because it is the core work engine.
const QStringList kCoreOrder = {
    QStringLiteral("home"), QStringLiteral("chronos"), QStringLiteral("modules"),
    QStringLiteral("settings"),
};

// Optional modules are appended after the core block in this fallback order;
// the saved tab order takes precedence once the user rearranges them.
const QStringList kDefaultOptionalOrder = {
    QStringLiteral("today"), QStringLiteral("projects"), QStringLiteral("kanban"),
    QStringLiteral("calendar"), QStringLiteral("codex"), QStringLiteral("logos"),
    QStringLiteral("phonio"), QStringLiteral("journal"), QStringLiteral("habits"),
    QStringLiteral("wellbeing"), QStringLiteral("statistics"), QStringLiteral("alerts"),
    QStringLiteral("logs"), QStringLiteral("ops"), QStringLiteral("help"),
};
}

MainWindow::MainWindow(app::AppContext* context, QWidget* parent)
    : QMainWindow(parent), m_context(context)
{
    setWindowTitle(QStringLiteral("Arete"));
    setMinimumSize(1000, 680);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_topBar = new TopBarWidget(context, central);
    layout->addWidget(m_topBar);

    m_tabBar = new QTabBar(central);
    m_tabBar->setMovable(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setTabsClosable(false);
    m_tabBar->setDocumentMode(true);
    m_tabBar->setStyleSheet(QStringLiteral(
        "QTabBar::tab { background: transparent; color: #7A7A7A; padding: 9px 18px;"
        " border-bottom: 2px solid transparent; font-size: 12px; }"
        "QTabBar::tab:selected { color: #D6D6D6; border-bottom: 2px solid #D0D0D0; }"
        "QTabBar::tab:hover { color: #A8A8A8; }"
    ));
    layout->addWidget(m_tabBar);

    m_stack = new QStackedWidget(central);
    layout->addWidget(m_stack, 1);

    setCentralWidget(central);

    buildModules();
    buildTabBar();
    setupShortcuts();
    setupConnections();

    Logger::instance().info(QStringLiteral("MainWindow ready (%1 modules)").arg(m_modules.size()),
                            QStringLiteral("ui"));
}

MainWindow::~MainWindow()
{
    saveSession();
}

// ── Module construction ──────────────────────────────────────────────────

void MainWindow::buildModules()
{
    using core::ModuleInfo;
    auto reg = [this](const QString& id, const QString& title, const QString& icon,
                      bool pinned, bool lazy, bool optional,
                      std::function<core::Module*(app::AppContext*)> factory) {
        m_context->modules()->registerModule({id, title, icon, pinned, lazy, optional, {}, {}, false,
                                              std::move(factory)});
    };

    // ── Core tabs (permanent, fixed order) ──
    reg(QStringLiteral("home"), QStringLiteral("Home"), core::icons::Home, true, false, false,
        [](app::AppContext* ctx) { return new HomeModule(ctx); });
    reg(QStringLiteral("modules"), QStringLiteral("Modules"), core::icons::Home, true, false, false,
        [](app::AppContext* ctx) { return new ModulesModule(ctx); });
    reg(QStringLiteral("chronos"), QStringLiteral("Chronos"), core::icons::Chronos, true, true, false,
        [](app::AppContext* ctx) { return new ChronosModule(ctx); });
    reg(QStringLiteral("settings"), QStringLiteral("Settings"), core::icons::Settings, true, false, false,
        [](app::AppContext* ctx) { return new SettingsModule(ctx); });

    // ── Optional modules (toggled from the Modules page) ──
    reg(QStringLiteral("today"), QStringLiteral("Today"), core::icons::Today, false, false, true,
        [](app::AppContext* ctx) { return new TodayModule(ctx); });
    reg(QStringLiteral("projects"), QStringLiteral("Projects"), core::icons::Projects, false, false, true,
        [](app::AppContext* ctx) { return new ProjectsModule(ctx); });
    reg(QStringLiteral("kanban"), QStringLiteral("Kanban"), core::icons::Kanban, false, false, true,
        [](app::AppContext* ctx) { return new KanbanModule(ctx); });
    reg(QStringLiteral("calendar"), QStringLiteral("Calendar"), core::icons::Calendar, false, false, true,
        [](app::AppContext* ctx) { return new CalendarModule(ctx); });
    reg(QStringLiteral("codex"), QStringLiteral("Codex"), core::icons::Codex, false, true, true,
        [](app::AppContext* ctx) { return new CodexModule(ctx); });
    reg(QStringLiteral("logos"), QStringLiteral("Logos"), core::icons::Logos, false, true, true,
        [](app::AppContext* ctx) { return new LogosModule(ctx); });
    reg(QStringLiteral("phonio"), QStringLiteral("Phonio"), core::icons::Phonio, false, true, true,
        [](app::AppContext* ctx) { return new PhonioModule(ctx); });
    reg(QStringLiteral("journal"), QStringLiteral("Journal"), core::icons::Journal, false, false, true,
        [](app::AppContext* ctx) { return new JournalModule(ctx); });
    reg(QStringLiteral("habits"), QStringLiteral("Habits"), core::icons::Habits, false, false, true,
        [](app::AppContext* ctx) { return new HabitsModule(ctx); });
    reg(QStringLiteral("wellbeing"), QStringLiteral("Wellbeing"), core::icons::Wellbeing, false, false, true,
        [](app::AppContext* ctx) { return new WellbeingModule(ctx); });
    reg(QStringLiteral("statistics"), QStringLiteral("Statistics"), core::icons::Statistics, false, false, true,
        [](app::AppContext* ctx) { return new StatisticsModule(ctx); });
    reg(QStringLiteral("alerts"), QStringLiteral("Alerts"), core::icons::Alerts, false, false, true,
        [](app::AppContext* ctx) { return new AlertsModule(ctx); });
    reg(QStringLiteral("logs"), QStringLiteral("Logs"), core::icons::Logs, false, false, true,
        [](app::AppContext* ctx) { return new LogsModule(ctx); });
    reg(QStringLiteral("ops"), QStringLiteral("Ops"), core::icons::Ops, false, false, true,
        [](app::AppContext* ctx) { return new OpsModule(ctx); });
    reg(QStringLiteral("help"), QStringLiteral("Help"), core::icons::Help, false, false, true,
        [](app::AppContext* ctx) { return new HelpModule(ctx); });

    // Live rebuild when the enabled set changes from the Modules page.
    connect(m_context->modules(), &core::ModuleRegistry::modulesChanged,
            this, &MainWindow::refreshModuleSet, Qt::QueuedConnection);

    m_order = computeOrder();

    for (const QString& id : m_order) {
        if (core::Module* module = m_context->modules()->create(id)) {
            m_modules.append(module);
            m_stack->addWidget(module);
        }
    }
}

QStringList MainWindow::computeOrder()
{
    auto* registry = m_context->modules();

    // Core block first, in its fixed order; it can never be moved or removed.
    QStringList order = kCoreOrder;

    // Restore saved order (comma list in newer sessions, JSON array older).
    QStringList saved;
    const QString rawOrder = m_context->settings()->get(QStringLiteral("ui/tabOrder"), QString());
    if (rawOrder.contains(QLatin1Char(','))) {
        saved = rawOrder.split(QLatin1Char(','), Qt::SkipEmptyParts);
    } else {
        const QJsonArray arr = QJsonDocument::fromJson(rawOrder.toUtf8()).array();
        for (const QJsonValue& v : arr) {
            saved.append(v.toString());
        }
    }
    for (const QString& id : saved) {
        if (registry->contains(id) && registry->isEnabled(id) && !order.contains(id)) {
            order.append(id);
        }
    }
    for (const QString& id : kDefaultOptionalOrder) {
        if (registry->contains(id) && registry->isEnabled(id) && !order.contains(id)) {
            order.append(id);
        }
    }
    return order;
}

void MainWindow::refreshModuleSet()
{
    const int current = m_tabBar->currentIndex();
    const QString currentId = (current >= 0 && current < m_modules.size())
        ? m_modules.at(current)->moduleId() : QStringLiteral("home");

    m_order = computeOrder();

    for (core::Module* module : m_modules) {
        m_stack->removeWidget(module);
        module->deleteLater();
    }
    m_modules.clear();

    for (const QString& id : m_order) {
        if (core::Module* module = m_context->modules()->create(id)) {
            m_modules.append(module);
            m_stack->addWidget(module);
        }
    }

    m_context->settings()->set(QStringLiteral("ui/tabOrder"), m_order.join(QLatin1Char(',')));
    m_context->settings()->sync();
    buildTabBar();
    activateModule(m_context->modules()->isEnabled(currentId) ? currentId
                                                              : QStringLiteral("home"));
}

void MainWindow::buildTabBar()
{
    m_tabBar->blockSignals(true);
    while (m_tabBar->count() > 0) {
        m_tabBar->removeTab(0);
    }
    for (core::Module* module : m_modules) {
        const QString icon = module->moduleIcon();
        const int tabIndex = m_tabBar->addTab(icon.isEmpty() ? module->moduleTitle()
                                                             : icon + QStringLiteral("  ") + module->moduleTitle());
        m_tabBar->setTabData(tabIndex, module->moduleId());
    }
    m_tabBar->blockSignals(false);

    // Rebuilds happen when the module set or order changes; drop stale connects.
    m_tabBar->disconnect(this);

    // The four core tabs (Home, Modules, Chronos, Settings) cannot be moved
    // or removed; dragging only rearranges the optional modules after them.
    // The tab bar has already applied the user's drag internally (it emits
    // tabMoved live during the motion), so defer the rebuild to after the
    // drag settles, then read the real arrangement from the tab data.
    connect(m_tabBar, &QTabBar::tabMoved, this, [this](int, int) {
        const int current = m_tabBar->currentIndex();
        const QString currentId = current >= 0 && current < m_tabBar->count()
            ? m_tabBar->tabData(current).toString() : QStringLiteral("home");
        QTimer::singleShot(0, this, [this, currentId] {
            QStringList order;
            for (int i = 0; i < m_tabBar->count(); ++i) {
                order.append(m_tabBar->tabData(i).toString());
            }
            reorderTo(order);
            if (!currentId.isEmpty()) activateModule(currentId);
        });
    });

    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        if (index < 0 || index >= m_modules.size()) return;
        m_stack->setCurrentIndex(index);
        m_modules.at(index)->activate();
    });
}

// ── Navigation ───────────────────────────────────────────────────────────

bool MainWindow::activateModule(const QString& id)
{
    const int index = m_order.indexOf(id);
    if (index < 0) return false;
    m_tabBar->setCurrentIndex(index);
    m_stack->setCurrentIndex(index);
    m_modules.at(index)->activate();
    return true;
}

void MainWindow::sendCommandToModule(const QString& id, const QString& command)
{
    const int index = m_order.indexOf(id);
    if (index < 0) return;
    m_modules.at(index)->handleCommand(command);
}

void MainWindow::showModule(const QString& id)
{
    activateModule(id);
}

void MainWindow::selectTab(int index)
{
    if (index >= 0 && index < m_tabBar->count()) {
        m_tabBar->setCurrentIndex(index);
    }
}

QWidget* MainWindow::moduleWidgetAt(int stackIndex) const
{
    return stackIndex >= 0 && stackIndex < m_stack->count() ? m_stack->widget(stackIndex) : nullptr;
}

void MainWindow::refreshTabLabels()
{
    for (int i = 0; i < m_tabBar->count(); ++i) {
        if (i < m_modules.size()) {
            const QString icon = m_modules.at(i)->moduleIcon();
            m_tabBar->setTabText(i, icon.isEmpty()
                                        ? m_modules.at(i)->moduleTitle()
                                        : icon + QStringLiteral("  ") + m_modules.at(i)->moduleTitle());
        }
    }
}

void MainWindow::reorderTo(const QStringList& ids)
{
    // Core block is fixed; only enabled optional modules follow, in the
    // order the user dragged them to. Disabled modules never appear.
    QStringList order = kCoreOrder;
    for (const QString& id : ids) {
        if (m_context->modules()->contains(id) && m_context->modules()->isEnabled(id)
            && !order.contains(id)) {
            order.append(id);
        }
    }
    // Any enabled optional modules missing from the dragged order follow at
    // the end instead of being dropped.
    for (const QString& id : kDefaultOptionalOrder) {
        if (m_context->modules()->contains(id) && m_context->modules()->isEnabled(id)
            && !order.contains(id)) {
            order.append(id);
        }
    }

    // Apply to the stack.
    QVector<core::Module*> newModules;
    for (const QString& id : order) {
        for (int i = 0; i < m_modules.size(); ++i) {
            if (m_modules.at(i)->moduleId() == id) {
                newModules.append(m_modules.at(i));
                m_stack->removeWidget(m_modules.at(i));
                break;
            }
        }
    }
    for (core::Module* module : newModules) {
        m_stack->addWidget(module);
    }
    m_modules = newModules;
    m_order = order;
    m_context->settings()->set(QStringLiteral("ui/tabOrder"), m_order.join(QLatin1Char(',')));
    m_context->settings()->sync();
    buildTabBar();
}

// ── Global actions ───────────────────────────────────────────────────────

void MainWindow::openCommandPalette()
{
    arete::widgets::CommandPalette palette(this);

    QVector<arete::widgets::Command> commands;
    // Recent history first.
    for (const QString& h : m_context->commands()->history()) {
        commands.append({QStringLiteral("history-") + h, QStringLiteral("history-") + h,
                         QStringLiteral("re-run"), QStringLiteral("History"),
                         [this, h] { m_context->commands()->run(h); }});
    }
    commands += m_context->commands()->paletteCommands();
    palette.setCommands(commands);
    palette.setSearchProvider([this](const QString& query) -> QVector<arete::widgets::Command> {
        QVector<arete::widgets::Command> out;
        const QString trimmed = query.trimmed();
        if (!trimmed.isEmpty() && trimmed != QLatin1String("play")
            && trimmed != QLatin1String("pause") && trimmed != QLatin1String("next")
            && trimmed != QLatin1String("previous") && trimmed != QLatin1String("stop")) {
            out.append({QStringLiteral("run-free"), QStringLiteral("Run: ") + trimmed,
                        QStringLiteral("command line"), QStringLiteral("Commands"),
                        [this, trimmed] { m_context->commands()->run(trimmed); }});
        }
        return out;
    });
    palette.execPalette();
}

void MainWindow::openUniversalSearch(const QString& initialQuery)
{
    UniversalSearchDialog dialog(this);
    dialog.setCommands(m_context->commands()->paletteCommands());
    dialog.setSearchProvider([this](const QString& query) -> QVector<arete::widgets::Command> {
        QVector<arete::widgets::Command> out;
        const auto hits = m_context->search()->search(query);
        for (const auto& hit : hits) {
            out.append({QStringLiteral("hit-") + hit.title, hit.title, hit.detail, hit.group,
                        hit.action});
        }
        const QString trimmed = query.trimmed();
        if (!trimmed.isEmpty()) {
            out.append({QStringLiteral("run-free"), QStringLiteral("Run: ") + trimmed,
                        QStringLiteral("command line"), QStringLiteral("Commands"),
                        [this, trimmed] { m_context->commands()->run(trimmed); }});
        }
        return out;
    });
    if (!initialQuery.isEmpty()) {
        dialog.setInitialQuery(initialQuery);
    }
    dialog.execPalette();
}

void MainWindow::openQuickCapture(const QString& kind)
{
    QuickCaptureDialog dialog(m_context, QuickCaptureDialog::kindFromString(kind), this);
    dialog.exec();
}

void MainWindow::captureNote(const QString& text)
{
    const QString vaultRoot = m_context->settings()->get(QStringLiteral("codex/vaultPath"), QString());
    if (vaultRoot.isEmpty()) {
        notify(QStringLiteral("No Codex vault"),
               QStringLiteral("Open the Codex tab once to create a vault."));
        return;
    }
    codex::VaultManager manager;
    manager.ensureVault(vaultRoot.toStdString());
    const QString title = text.left(40).simplified();
    if (manager.createNote(title.toStdString())) {
        notify(QStringLiteral("Note created"), title);
    }
}

void MainWindow::showTaskCompletion(qint64 taskId, const QString& taskTitle)
{
    TaskCompleteDialog dialog(m_context, this);
    dialog.setCompletedTask(taskTitle);
    if (dialog.exec() != QDialog::Accepted) return;

    switch (dialog.chosenAction()) {
    case TaskCompleteDialog::Action::StartNext:
        activateModule(QStringLiteral("today"));
        break;
    case TaskCompleteDialog::Action::ShortBreak:
        m_context->timer()->startShortBreak();
        break;
    case TaskCompleteDialog::Action::LongBreak:
        m_context->timer()->startLongBreak();
        break;
    case TaskCompleteDialog::Action::Reschedule: {
        models::Task task = m_context->tasks()->byId(taskId);
        if (task.id > 0) {
            task.dueAt = QDateTime(dialog.rescheduleDate(), QTime(17, 0));
            task.status = models::TaskStatus::Active;
            task.completedAt = QDateTime();
            m_context->tasks()->update(task);
            notify(QStringLiteral("Rescheduled"), QStringLiteral("Task moved to %1.")
                       .arg(dialog.rescheduleDate().toString(Qt::ISODate)));
        }
        break;
    }
    case TaskCompleteDialog::Action::Journal: {
        models::JournalEntry entry = m_context->journal()->forDate(QDate::currentDate());
        entry.date = QDate::currentDate();
        if (!entry.reflection.isEmpty()) entry.reflection += QStringLiteral("\n");
        entry.reflection += QStringLiteral("Completed: ") + taskTitle;
        m_context->journal()->upsert(entry);
        activateModule(QStringLiteral("journal"));
        break;
    }
    case TaskCompleteDialog::Action::Dismiss:
        break;
    }
}

void MainWindow::notify(const QString& title, const QString& message, int timeoutMs)
{
    arete::notifications::NotificationCenter::instance().showInfo(title, message, timeoutMs);
}

void MainWindow::highlightTask(qint64 taskId)
{
    activateModule(QStringLiteral("today"));
    for (core::Module* module : m_modules) {
        if (module->moduleId() == QLatin1String("today")) {
            QMetaObject::invokeMethod(module, "highlightTask", Q_ARG(qint64, taskId));
            break;
        }
    }
}

void MainWindow::highlightProject(qint64 projectId)
{
    activateModule(QStringLiteral("projects"));
    for (core::Module* module : m_modules) {
        if (module->moduleId() == QLatin1String("projects")) {
            QMetaObject::invokeMethod(module, "highlightProject", Q_ARG(qint64, projectId));
            break;
        }
    }
}

void MainWindow::openJournalDate(const QDate& date)
{
    activateModule(QStringLiteral("journal"));
    for (core::Module* module : m_modules) {
        if (module->moduleId() == QLatin1String("journal")) {
            QMetaObject::invokeMethod(module, "openDate", Q_ARG(QDate, date));
            break;
        }
    }
}

// ── Shortcuts ────────────────────────────────────────────────────────────

void MainWindow::setupShortcuts()
{
    const auto add = [this](const QKeySequence& seq, std::function<void()> fn) {
        auto* sc = new QShortcut(seq, this, nullptr, nullptr, Qt::WindowShortcut);
        connect(sc, &QShortcut::activated, this, [fn] { fn(); });
    };

    add(QKeySequence(QStringLiteral("Ctrl+P")), [this] { openCommandPalette(); });
    add(QKeySequence(QStringLiteral("Ctrl+K")), [this] { openUniversalSearch(); });
    add(QKeySequence(QStringLiteral("Ctrl+T")), [this] { openQuickCapture(QStringLiteral("task")); });
    add(QKeySequence(QStringLiteral("Ctrl+Shift+A")), [this] { openQuickCapture(QStringLiteral("task")); });
    add(QKeySequence(QStringLiteral("Ctrl+J")), [this] { activateModule(QStringLiteral("journal")); });

    for (int i = 0; i < 9; ++i) {
        const int index = i;
        add(QKeySequence(Qt::ALT | (Qt::Key_1 + i)), [this, index] { selectTab(index); });
    }

    add(QKeySequence(Qt::CTRL | Qt::Key_Tab), [this] {
        const int next = (m_tabBar->currentIndex() + 1) % m_tabBar->count();
        selectTab(next);
    });
    add(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), [this] {
        const int count = m_tabBar->count();
        const int prev = (m_tabBar->currentIndex() - 1 + count) % count;
        selectTab(prev);
    });

    add(QKeySequence(QStringLiteral("F1")), [this] {
        notify(QStringLiteral("Shortcuts"),
               QStringLiteral("Ctrl+P palette · Ctrl+K search · Ctrl+T task · Ctrl+J journal"));
    });
}

void MainWindow::setupConnections()
{
    connect(m_topBar, &TopBarWidget::notificationsClicked, this, [this] {
        if (!m_notificationsPopup) {
            m_notificationsPopup = new QWidget(this, Qt::Popup);
            m_notificationsPopup->setFixedSize(360, 420);
            auto* layout = new QVBoxLayout(m_notificationsPopup);
            layout->setContentsMargins(0, 0, 0, 0);
            auto* panel = new arete::widgets::NotificationsPanel(m_notificationsPopup);
            panel->setNotificationCenter(&arete::notifications::NotificationCenter::instance());
            layout->addWidget(panel);
        }
        const QPoint global = m_topBar->mapToGlobal(m_topBar->rect().topRight());
        m_notificationsPopup->move(global.x() - m_notificationsPopup->width(), global.y() + 8);
        m_notificationsPopup->show();
    });
}

// ── Session ──────────────────────────────────────────────────────────────

void MainWindow::restoreSession()
{
    const QByteArray geometry = m_context->settings()
        ->get(QStringLiteral("window/geometry"), QByteArray());
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }

    const QStringList savedOrder = m_context->settings()
        ->get(QStringLiteral("ui/tabOrder"), QString()).split(QLatin1Char(','), Qt::SkipEmptyParts);
    if (!savedOrder.isEmpty() && savedOrder.size() >= 2) {
        reorderTo(savedOrder);
    }

    const QString selected = m_context->settings()->get(QStringLiteral("ui/selectedTab"),
                                                        QStringLiteral("home"));
    const int index = m_order.indexOf(selected);
    if (index >= 0) {
        m_tabBar->setCurrentIndex(index);
        m_stack->setCurrentIndex(index);
        m_modules.at(index)->activate();
    } else {
        m_modules.at(0)->activate();
    }
}

void MainWindow::saveSession()
{
    m_context->settings()->set(QStringLiteral("window/geometry"), saveGeometry());
    m_context->settings()->set(QStringLiteral("ui/tabOrder"), m_order.join(QLatin1Char(',')));
    const int index = m_tabBar->currentIndex();
    if (index >= 0 && index < m_modules.size()) {
        m_context->settings()->set(QStringLiteral("ui/selectedTab"), m_modules.at(index)->moduleId());
    }
    m_context->settings()->sync();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveSession();
    QMainWindow::closeEvent(event);
}

} // namespace arete::ui
