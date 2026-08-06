#include "ui/common/TaskRowWidget.h"

#include <QHBoxLayout>
#include <QMenu>
#include <QMouseEvent>

namespace arete::ui::common {

namespace {
// Plain-text priority markers (no glyphs).
inline const QString kLowGlyph = QStringLiteral("P3");
inline const QString kMediumGlyph = QStringLiteral("P2");
inline const QString kHighGlyph = QStringLiteral("P1");
} // namespace

TaskRowWidget::TaskRowWidget(const models::Task& task, QWidget* parent)
    : QWidget(parent), m_task(task)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setStyleSheet(QStringLiteral(
        "QWidget { background: transparent; }"
        "QLabel { background: transparent; }"));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(10);

    m_check = new TaskToggle(this);
    connect(m_check, &TaskToggle::toggled, this, [this](bool checked) {
        if (m_task.status == models::TaskStatus::Active && checked) {
            emit toggled(m_task.id, true);
        }
    });

    m_title = new QLabel(this);
    m_title->setWordWrap(false);
    m_title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_meta = new QLabel(this);
    m_meta->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    layout->addWidget(m_check);
    layout->addWidget(m_title, 1);
    layout->addWidget(m_meta);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu(this);
        auto* focus = menu.addAction(QStringLiteral("Start Focus"));
        auto* edit = menu.addAction(QStringLiteral("Edit"));
        QAction* chosen = menu.exec(mapToGlobal(pos));
        if (chosen == focus) emit focusRequested(m_task.id, m_task.title);
        else if (chosen == edit) emit editRequested(m_task.id);
    });

    refreshLabels();
}

void TaskRowWidget::setTask(const models::Task& task)
{
    m_task = task;
    refreshLabels();
}

void TaskRowWidget::setCheckable(bool checkable)
{
    m_checkable = checkable;
    m_check->setVisible(checkable);
}

void TaskRowWidget::refreshLabels()
{
    const bool done = m_task.status == models::TaskStatus::Completed;
    m_check->setChecked(done);
    m_check->setEnabled(!done && m_checkable);

    QString title = m_task.title;
    if (m_task.isOverdue()) {
        title += QStringLiteral("  !");
    }
    m_title->setText(title);
    m_title->setStyleSheet(done ? QStringLiteral("color: #5A5A5A; text-decoration: line-through;")
                                : QStringLiteral("color: #C4C4C4;"));

    QString meta;
    if (m_task.priority != models::Priority::Medium) {
        meta += (m_task.priority == models::Priority::High) ? kHighGlyph : kLowGlyph;
        meta += QStringLiteral("  ");
    }
    if (m_task.dueAt.isValid()) {
        meta += m_task.dueAt.toString(QStringLiteral("MMM d"));
    }
    if (!m_task.checklist.isEmpty()) {
        meta += (meta.isEmpty() ? QString() : QStringLiteral("  "))
            + QStringLiteral("[%1/%2]").arg(m_task.checkedItems.size()).arg(m_task.checklist.size());
    }
    m_meta->setText(meta);
    m_meta->setStyleSheet(m_task.isOverdue() ? QStringLiteral("color: #C4A050;")
                                             : QStringLiteral("color: #7A7A7A;"));
}

void TaskRowWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit editRequested(m_task.id);
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

} // namespace arete::ui::common
