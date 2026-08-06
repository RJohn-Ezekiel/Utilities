#include "TaskListWidget.h"
#include "TaskWidget.h"
#include "services/TaskService.h"
#include "Theme.h"

#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QScrollBar>

namespace chronos {

TaskListWidget::TaskListWidget(TaskService* taskService, QWidget* parent)
    : QWidget(parent)
    , m_taskService(taskService)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Header ──
    auto* header = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(16, 12, 16, 8);

    auto* titleLabel = new QLabel(QStringLiteral("Tasks"), this);
    titleLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 18px; font-weight: bold;"
    ).arg(Theme::PrimaryText.name()));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    m_addBtn = new QPushButton(QStringLiteral("+ Add Task"), this);
    m_addBtn->setFixedHeight(30);
    m_addBtn->setCursor(Qt::PointingHandCursor);
    m_addBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  padding: 4px 16px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover { background: %4; }"
    ).arg(Theme::Card.name())
     .arg(Theme::Accent.name())
     .arg(Theme::Border.name())
     .arg(Theme::Hover.name()));
    connect(m_addBtn, &QPushButton::clicked, this, &TaskListWidget::onAddTask);
    headerLayout->addWidget(m_addBtn);

    root->addWidget(header);

    // ── Scroll area ──
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; }"
        "QScrollBar:vertical {"
        "  background: %1;"
        "  width: 8px;"
        "  margin: 0;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: %2;"
        "  min-height: 30px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0;"
        "}"
    ).arg(Theme::Panel.name()).arg(Theme::Border.name()));

    m_scrollContent = new QWidget(this);
    m_taskLayout = new QVBoxLayout(m_scrollContent);
    m_taskLayout->setContentsMargins(0, 0, 0, 0);
    m_taskLayout->setSpacing(0);
    m_taskLayout->addStretch();

    m_scrollArea->setWidget(m_scrollContent);
    root->addWidget(m_scrollArea, 1);

    // ── Connections ──
    connect(m_taskService, &TaskService::tasksChanged,
            this, &TaskListWidget::onTasksChanged);

    onTasksChanged();
}

void TaskListWidget::onTasksChanged()
{
    rebuildTaskWidgets();
}

void TaskListWidget::rebuildTaskWidgets()
{
    // Remove old widgets
    for (auto* w : m_taskWidgets) {
        m_taskLayout->removeWidget(w);
        w->deleteLater();
    }
    m_taskWidgets.clear();

    // Remove stretch (will re-add at end)
    auto* stretchItem = m_taskLayout->takeAt(m_taskLayout->count() - 1);
    delete stretchItem;

    const auto& tasks = m_taskService->tasks();
    int count = tasks.size();

    for (int i = 0; i < count; ++i) {
        auto* widget = new TaskWidget(tasks[i], i == 0, i == count - 1,
                                      m_scrollContent);

        connect(widget, &TaskWidget::startRequested,
                this, &TaskListWidget::onStartTask);
        connect(widget, &TaskWidget::completeToggled,
                this, &TaskListWidget::onCompleteToggled);
        connect(widget, &TaskWidget::editRequested,
                this, &TaskListWidget::onEditTask);
        connect(widget, &TaskWidget::deleteRequested,
                this, &TaskListWidget::onDeleteTask);
        connect(widget, &TaskWidget::moveUpRequested,
                this, &TaskListWidget::onMoveUp);
        connect(widget, &TaskWidget::moveDownRequested,
                this, &TaskListWidget::onMoveDown);

        m_taskLayout->addWidget(widget);
        m_taskWidgets.append(widget);
    }

    m_taskLayout->addStretch();
}

// ── Actions ──

void TaskListWidget::onAddTask()
{
    QString title;
    QString description;
    int estimatedSessions = 1;

    if (!editTaskDialog(this, title, description, estimatedSessions)) {
        return;
    }
    if (title.trimmed().isEmpty()) return;

    m_taskService->addTask(title.trimmed(), description.trimmed(), estimatedSessions);
}

void TaskListWidget::onStartTask(const QString& taskId)
{
    emit startFocusRequested(taskId);
}

void TaskListWidget::onCompleteToggled(const QString& taskId, bool completed)
{
    if (completed) {
        m_taskService->completeTask(taskId);
    } else {
        m_taskService->uncompleteTask(taskId);
    }
}

void TaskListWidget::onEditTask(const QString& taskId)
{
    auto task = m_taskService->task(taskId);
    if (task.id.isEmpty()) return;

    QString title = task.title;
    QString description = task.description;
    int estimatedSessions = task.estimatedSessions;

    if (!editTaskDialog(this, title, description, estimatedSessions)) {
        return;
    }

    m_taskService->editTask(taskId, title.trimmed(), description.trimmed(),
                            estimatedSessions);
}

void TaskListWidget::onDeleteTask(const QString& taskId)
{
    m_taskService->deleteTask(taskId);
}

void TaskListWidget::onMoveUp(const QString& taskId)
{
    m_taskService->moveTaskUp(taskId);
}

void TaskListWidget::onMoveDown(const QString& taskId)
{
    m_taskService->moveTaskDown(taskId);
}

// ── Dialog ──

bool TaskListWidget::editTaskDialog(QWidget* parent, QString& title,
                                    QString& description, int& estimatedSessions)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title.isEmpty()
        ? QStringLiteral("New Task")
        : QStringLiteral("Edit Task"));
    dialog.setMinimumWidth(400);

    auto* layout = new QVBoxLayout(&dialog);

    auto* form = new QFormLayout();

    auto* titleEdit = new QLineEdit(title, &dialog);
    titleEdit->setPlaceholderText(QStringLiteral("Task title"));
    form->addRow(QStringLiteral("Title:"), titleEdit);

    auto* descEdit = new QLineEdit(description, &dialog);
    descEdit->setPlaceholderText(QStringLiteral("Optional description"));
    form->addRow(QStringLiteral("Description:"), descEdit);

    auto* sessionsSpin = new QSpinBox(&dialog);
    sessionsSpin->setRange(1, 99);
    sessionsSpin->setValue(estimatedSessions);
    form->addRow(QStringLiteral("Estimated sessions:"), sessionsSpin);

    layout->addLayout(form);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    // Style the dialog
    dialog.setStyleSheet(QStringLiteral(
        "QDialog { background: %1; color: %2; }"
        "QLineEdit, QSpinBox {"
        "  background: %3;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  padding: 4px 8px;"
        "  font-size: 13px;"
        "}"
        "QLabel { color: %5; }"
        "QPushButton {"
        "  background: %3;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  padding: 6px 16px;"
        "}"
        "QPushButton:hover { background: %6; }"
    ).arg(Theme::Background.name())
     .arg(Theme::PrimaryText.name())
     .arg(Theme::Card.name())
     .arg(Theme::Border.name())
     .arg(Theme::SecondaryText.name())
     .arg(Theme::Hover.name()));

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    title = titleEdit->text();
    description = descEdit->text();
    estimatedSessions = sessionsSpin->value();
    return true;
}

} // namespace chronos
