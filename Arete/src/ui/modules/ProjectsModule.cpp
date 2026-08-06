#include "ui/modules/ProjectsModule.h"
#include "app/AppContext.h"
#include "data/ProjectRepository.h"
#include "data/TaskRepository.h"
#include "models/Project.h"
#include "models/Task.h"
#include "ui/common/SectionLabel.h"
#include "ui/common/EmptyStateWidget.h"
#include "core/Icons.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QProgressBar>
#include <QTimer>
#include <QBrush>
#include <QDateTime>
#include <QPushButton>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QMessageBox>

namespace arete::ui {

ProjectsModule::ProjectsModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(48, 32, 48, 32);
    layout->setSpacing(16);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("Projects"), this);
    title->setStyleSheet(QStringLiteral("font-size: 22px; color: #E2E2E2; letter-spacing: 1px;"));
    header->addWidget(title);
    header->addStretch(1);

    const auto buttonStyle = QStringLiteral(
        "QPushButton { font-size: 12px; letter-spacing: 1px; color: #D6D6D6;"
        " background-color: #242424; border: 1px solid #353535; border-radius: 8px;"
        " padding: 6px 14px; }"
        "QPushButton:hover { background-color: #2A2A2A; }");
    m_addButton = new QPushButton(QStringLiteral("+  NEW PROJECT"), this);
    m_editButton = new QPushButton(QStringLiteral("Edit"), this);
    m_deleteButton = new QPushButton(QStringLiteral("Delete"), this);
    for (QPushButton* b : {m_addButton, m_editButton, m_deleteButton}) {
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(buttonStyle);
    }
    connect(m_addButton, &QPushButton::clicked, this, [this] { showEditor(nullptr); });
    connect(m_editButton, &QPushButton::clicked, this, [this, context] {
        const int index = m_list->currentRow();
        if (index < 0) return;
        const auto projects = context->projects()->all();
        if (index < projects.size()) showEditor(&projects.at(index));
    });
    connect(m_deleteButton, &QPushButton::clicked, this, [this, context] {
        const int index = m_list->currentRow();
        if (index < 0) return;
        const auto projects = context->projects()->all();
        if (index >= projects.size()) return;
        const auto answer = QMessageBox::question(
            this, QStringLiteral("Delete project"),
            QStringLiteral("Remove \u201C%1\u201D? Its tasks stay in the list.")
                .arg(projects.at(index).name),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer == QMessageBox::Yes) {
            context->projects()->remove(projects.at(index).id);
            refresh();
        }
    });
    m_editButton->setEnabled(false);
    m_deleteButton->setEnabled(false);
    header->addWidget(m_editButton);
    header->addWidget(m_deleteButton);
    header->addWidget(m_addButton);
    layout->addLayout(header);

    m_summary = new QLabel(this);
    m_summary->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 13px;"));
    layout->addWidget(m_summary);

    m_list = new QListWidget(this);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setFocusPolicy(Qt::ClickFocus);
    m_list->setStyleSheet(QStringLiteral("QListWidget { background: transparent; outline: none; }"
                                         "QListWidget::item { padding: 6px 0; }"
                                         "QListWidget::item:selected { background: #1E1E1E; }"));
    connect(m_list, &QListWidget::itemSelectionChanged, this, [this] {
        const bool has = m_list->currentRow() >= 0;
        m_editButton->setEnabled(has);
        m_deleteButton->setEnabled(has);
    });
    layout->addWidget(m_list, 1);
}

void ProjectsModule::onActivated()
{
    refresh();
}

void ProjectsModule::refreshView()
{
    refresh();
}

void ProjectsModule::refresh()
{
    m_list->clear();

    const auto projects = context()->projects()->all();
    const auto tasks = context()->tasks()->active();
    int done = 0;
    int total = 0;
    for (const models::Task& t : tasks) {
        if (t.status == models::TaskStatus::Completed) ++done;
        ++total;
    }

    if (projects.isEmpty()) {
        m_summary->setText(QStringLiteral("%1 of %2 tasks completed").arg(done).arg(total));
        m_list->addItem(QString());
        m_list->setItemWidget(m_list->item(0),
                              new common::EmptyStateWidget(QStringLiteral(""),
                                                           QStringLiteral("No projects yet. Create one with \u201C+ NEW PROJECT\u201D."),
                                                           this));
        return;
    }

    for (const models::Project& p : projects) {
        auto* widget = new QWidget(this);
        auto* v = new QVBoxLayout(widget);
        v->setContentsMargins(4, 8, 4, 8);
        v->setSpacing(6);

        auto* name = new QLabel(p.name, widget);
        name->setStyleSheet(QStringLiteral("font-size: 15px; color: #D6D6D6;"));

        int pDone = 0;
        int pTotal = 0;
        QDate deadline = p.deadline.date();
        const auto pTasks = context()->tasks()->forProject(p.id);
        for (const models::Task& t : pTasks) {
            if (t.status == models::TaskStatus::Completed) ++pDone;
            ++pTotal;
        }
        auto* meta = new QLabel(widget);
        QString metaText = QStringLiteral("%1 / %2 tasks").arg(pDone).arg(pTotal);
        if (deadline.isValid()) {
            metaText += QStringLiteral("  \u00B7  due %1")
                            .arg(deadline.toString(QStringLiteral("MMM d")));
        }
        meta->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 12px;"));
        meta->setText(metaText);

        auto* bar = new QProgressBar(widget);
        bar->setRange(0, qMax(1, pTotal));
        bar->setValue(pDone);
        bar->setTextVisible(false);
        bar->setFixedHeight(5);
        bar->setStyleSheet(QStringLiteral(
            "QProgressBar { background-color: #242424; border: none; border-radius: 2px; }"
            "QProgressBar::chunk { background-color: #7A7A7A; border-radius: 2px; }"));

        v->addWidget(name);
        v->addWidget(meta);
        v->addWidget(bar);

        m_list->addItem(QString());
        m_list->setItemWidget(m_list->item(m_list->count() - 1), widget);
    }
}

void ProjectsModule::showEditor(const models::Project* project)
{
    QDialog dialog(this);
    dialog.setWindowTitle(project ? QStringLiteral("Edit Project") : QStringLiteral("New Project"));
    auto* form = new QFormLayout(&dialog);

    auto* nameEdit = new QLineEdit(project ? project->name : QString(), &dialog);
    auto* deadlineEdit = new QDateEdit(&dialog);
    deadlineEdit->setCalendarPopup(true);
    deadlineEdit->setDisplayFormat(QStringLiteral("MMM d, yyyy"));
    deadlineEdit->setDateRange(QDate(2000, 1, 1), QDate(2100, 12, 31));
    if (project && project->deadline.isValid()) {
        deadlineEdit->setDate(project->deadline.date());
    }
    const QString style = QStringLiteral(
        "QLineEdit, QDateEdit { background: #1C1C1C; border: 1px solid #2A2A2A;"
        " color: #D6D6D6; border-radius: 6px; padding: 6px 8px; }");
    nameEdit->setStyleSheet(style);
    deadlineEdit->setStyleSheet(style);
    nameEdit->setPlaceholderText(QStringLiteral("Project name"));
    form->addRow(QStringLiteral("Name"), nameEdit);
    form->addRow(QStringLiteral("Deadline"), deadlineEdit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(project ? QStringLiteral("Save")
                                                           : QStringLiteral("Create"));
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted) return;
    const QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) return;

    models::Project p;
    if (project) p = *project;
    p.name = name;
    p.deadline = QDateTime(deadlineEdit->date(), QTime(23, 59));
    if (project) {
        context()->projects()->update(p);
    } else {
        context()->projects()->insert(p);
    }
    refresh();
}

void ProjectsModule::highlightProject(qint64 projectId)
{
    refresh();
    int index = 0;
    for (const models::Project& p : context()->projects()->all()) {
        if (p.id == projectId && index < m_list->count()) {
            auto* item = m_list->item(index);
            m_list->scrollToItem(item, QAbstractItemView::PositionAtCenter);
            item->setBackground(QColor(QStringLiteral("#2E2E2E")));
            QTimer::singleShot(1200, this, [item] { item->setBackground(QBrush()); });
            return;
        }
        ++index;
    }
}

} // namespace arete::ui
