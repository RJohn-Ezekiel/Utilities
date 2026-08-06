#include "ui/modules/KanbanModule.h"
#include "app/AppContext.h"
#include "data/TaskRepository.h"
#include "models/Task.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDateTime>
#include <functional>

namespace arete::ui {

namespace {
const int kTaskRole = Qt::UserRole;
const QLatin1String kTaskMime("application/x-arete-task");
}

// Task list inside a board: carries task ids through drag-and-drop and
// applies the status change on drop.
class KanbanTaskList : public QListWidget
{
public:
    std::function<void(qint64, const QString&)> onDrop;

    explicit KanbanTaskList(const QString& columnName, QWidget* parent = nullptr)
        : QListWidget(parent), m_columnName(columnName)
    {
        setObjectName(columnName);
        setDragEnabled(true);
        setAcceptDrops(true);
        setDragDropMode(QAbstractItemView::DragDrop);
        setDragDropOverwriteMode(false);
        setDropIndicatorShown(true);
        setDefaultDropAction(Qt::MoveAction);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setFrameShape(QFrame::NoFrame);
        setSpacing(4);
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        setStyleSheet(QStringLiteral(
            "QListWidget { background-color: #1C1C1C; border: 1px solid #2A2A2A;"
            " border-radius: 12px; padding: 6px; }"
            "QListWidget::item { padding: 8px; margin: 2px 0; border-radius: 8px;"
            " background-color: #242424; color: #D6D6D6; border: 1px solid #2E2E2E; }"
            "QListWidget::item:selected { background-color: #2E2E2E; }"));
    }

protected:
    QMimeData* mimeData(const QList<QListWidgetItem*>& items) const override
    {
        auto* mime = new QMimeData;
        if (!items.isEmpty()) {
            mime->setData(kTaskMime,
                          QByteArray::number(items.first()->data(kTaskRole).toLongLong()));
        }
        return mime;
    }

    void dropEvent(QDropEvent* event) override
    {
        if (event->mimeData()->hasFormat(kTaskMime)) {
            const qint64 id = event->mimeData()->data(kTaskMime).toLongLong();
            event->acceptProposedAction();
            if (onDrop) onDrop(id, m_columnName);
        } else {
            QListWidget::dropEvent(event);
        }
    }

    void dragEnterEvent(QDragEnterEvent* event) override
    {
        if (event->mimeData()->hasFormat(kTaskMime)) {
            event->acceptProposedAction();
        } else {
            QListWidget::dragEnterEvent(event);
        }
    }

    void dragMoveEvent(QDragMoveEvent* event) override
    {
        if (event->mimeData()->hasFormat(kTaskMime)) {
            event->acceptProposedAction();
        } else {
            QListWidget::dragMoveEvent(event);
        }
    }

private:
    QString m_columnName;
};

// One board: titled header with a task count and an Add button, plus the
// draggable task list underneath.
class KanbanColumn : public QWidget
{
public:
    QString name;
    std::function<void(const QString&)> onAdd;
    std::function<void(qint64)> onRename;
    std::function<void(qint64)> onDelete;

    explicit KanbanColumn(const QString& columnName, QWidget* parent = nullptr)
        : QWidget(parent), name(columnName)
    {
        auto* v = new QVBoxLayout(this);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(8);

        auto* header = new QHBoxLayout;
        header->setSpacing(6);
        m_title = new QLabel(columnName, this);
        m_title->setStyleSheet(QStringLiteral(
            "QLabel { color: #C4C4C4; font-size: 14px; letter-spacing: 1px; }"));
        header->addWidget(m_title);
        header->addStretch(1);

        m_add = new QPushButton(QStringLiteral("+"), this);
        m_add->setToolTip(QStringLiteral("Add a task to %1").arg(columnName));
        m_add->setFixedSize(24, 24);
        m_add->setStyleSheet(QStringLiteral(
            "QPushButton { background-color: #242424; color: #C4C4C4; border: 1px solid #353535;"
            " border-radius: 12px; font-size: 15px; }"
            "QPushButton:hover { border-color: #4A4A4A; }"));
        connect(m_add, &QPushButton::clicked, this, [this] { if (onAdd) onAdd(name); });
        header->addWidget(m_add);

        v->addLayout(header);

        m_list = new KanbanTaskList(columnName, this);
        connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
            if (onRename && item) onRename(item->data(kTaskRole).toLongLong());
        });
        connect(m_list, &QListWidget::customContextMenuRequested, this,
                [this](const QPoint& pos) {
            QListWidgetItem* item = m_list->itemAt(pos);
            if (!item) return;
            QMenu menu(m_list);
            QAction* renameAction = menu.addAction(QStringLiteral("Rename"));
            QAction* deleteAction = menu.addAction(QStringLiteral("Delete"));
            QAction* chosen = menu.exec(m_list->viewport()->mapToGlobal(pos));
            if (!chosen) return;
            const qint64 id = item->data(kTaskRole).toLongLong();
            if (chosen == renameAction && onRename) onRename(id);
            else if (chosen == deleteAction && onDelete) onDelete(id);
        });
        m_list->setContextMenuPolicy(Qt::CustomContextMenu);

        v->addWidget(m_list, 1);
    }

    void setCount(int count)
    {
        m_title->setText(QStringLiteral("%1  %2").arg(name).arg(count));
    }

    KanbanTaskList* list() const { return m_list; }

private:
    QLabel* m_title;
    QPushButton* m_add;
    KanbanTaskList* m_list;
};

KanbanModule::KanbanModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("Kanban"), this);
    title->setStyleSheet(QStringLiteral("font-size: 22px; color: #E2E2E2; letter-spacing: 1px;"));
    layout->addWidget(title);

    auto moveTask = [this, context](qint64 taskId, const QString& columnName) {
        models::Task task = context->tasks()->byId(taskId);
        if (task.id < 0) return;
        if (columnName == QLatin1String("Done")) {
            task.status = models::TaskStatus::Completed;
            task.completedAt = QDateTime::currentDateTime();
        } else if (columnName == QLatin1String("Today")) {
            task.status = models::TaskStatus::Active;
            task.dueAt = QDateTime(QDate::currentDate(), QTime(17, 0));
            task.completedAt = QDateTime();
        } else {
            task.status = models::TaskStatus::Active;
            task.dueAt = QDateTime();
            task.completedAt = QDateTime();
        }
        context->tasks()->update(task);
        rebuild();
    };

    auto* board = new QHBoxLayout;
    board->setSpacing(12);
    for (const QString& name : {QStringLiteral("Active"), QStringLiteral("Today"), QStringLiteral("Done")}) {
        auto* column = new KanbanColumn(name, this);
        column->list()->onDrop = moveTask;
        column->onAdd = [this](const QString& columnName) { addTask(columnName); };
        column->onRename = [this](qint64 taskId) { renameTask(taskId); };
        column->onDelete = [this](qint64 taskId) { deleteTask(taskId); };
        m_columns.insert(name, column);
        board->addWidget(column, 1);
    }
    layout->addLayout(board, 1);
}

void KanbanModule::addTask(const QString& columnName)
{
    bool ok = false;
    const QString text = QInputDialog::getText(this, QStringLiteral("Add task"),
                                               QStringLiteral("Task title:"),
                                               QLineEdit::Normal, QString(), &ok);
    if (!ok || text.trimmed().isEmpty()) return;

    models::Task task;
    task.title = text.trimmed();
    if (columnName == QLatin1String("Done")) {
        task.status = models::TaskStatus::Completed;
        task.completedAt = QDateTime::currentDateTime();
    } else if (columnName == QLatin1String("Today")) {
        task.status = models::TaskStatus::Active;
        task.dueAt = QDateTime(QDate::currentDate(), QTime(17, 0));
    } else {
        task.status = models::TaskStatus::Active;
    }
    context()->tasks()->insert(task);
    rebuild();
}

void KanbanModule::renameTask(qint64 taskId)
{
    models::Task task = context()->tasks()->byId(taskId);
    if (task.id < 0) return;
    bool ok = false;
    const QString text = QInputDialog::getText(this, QStringLiteral("Rename task"),
                                               QStringLiteral("Task title:"),
                                               QLineEdit::Normal, task.title, &ok);
    if (!ok || text.trimmed().isEmpty() || text.trimmed() == task.title) return;
    task.title = text.trimmed();
    context()->tasks()->update(task);
    rebuild();
}

void KanbanModule::deleteTask(qint64 taskId)
{
    models::Task task = context()->tasks()->byId(taskId);
    if (task.id < 0) return;
    const auto choice = QMessageBox::question(
        this, QStringLiteral("Delete task"),
        QStringLiteral("Delete \"%1\"?").arg(task.title),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (choice != QMessageBox::Yes) return;
    context()->tasks()->remove(taskId);
    rebuild();
}

void KanbanModule::rebuild()
{
    for (KanbanColumn* column : m_columns) {
        column->list()->clear();
    }

    const auto tasks = context()->tasks()->all();
    for (const models::Task& t : tasks) {
        const QString columnName = t.status == models::TaskStatus::Completed
            ? QStringLiteral("Done")
            : (t.isDueToday() ? QStringLiteral("Today") : QStringLiteral("Active"));
        KanbanColumn* column = m_columns.value(columnName);
        if (!column) continue;

        auto* item = new QListWidgetItem(t.title, column->list());
        item->setData(kTaskRole, t.id);
        if (t.isOverdue()) {
            item->setForeground(QColor(QStringLiteral("#909090")));
        }
    }
    for (KanbanColumn* column : m_columns) {
        column->setCount(column->list()->count());
    }
}

void KanbanModule::onActivated()
{
    rebuild();
}

void KanbanModule::refreshView()
{
    rebuild();
}

} // namespace arete::ui