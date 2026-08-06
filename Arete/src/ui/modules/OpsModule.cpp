#include "ui/modules/OpsModule.h"
#include "app/AppContext.h"
#include "data/TaskRepository.h"
#include "models/Task.h"
#include "vendor/chronos/storage/StorageManager.h"
#include "vendor/chronos/models/Session.h"

#include <QDateTime>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace arete::ui {

namespace {

struct ActivityRow {
    QDateTime timestamp;
    QString entity;
    QString action;
    QString target;
};

} // namespace

OpsModule::OpsModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(48, 32, 48, 32);
    m_layout->setSpacing(16);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("Ops"), this);
    title->setStyleSheet(QStringLiteral("font-size: 22px; color: #E2E2E2; letter-spacing: 1px;"));
    header->addWidget(title);

    auto* subtitle = new QLabel(QStringLiteral("Recent activity"), this);
    subtitle->setStyleSheet(QStringLiteral("color: #707070; font-size: 13px;"));
    header->addWidget(subtitle);
    header->addStretch(1);

    auto* refreshButton = new QPushButton(QStringLiteral("Refresh"), this);
    refreshButton->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #242424; color: #C4C4C4;"
                       " border: 1px solid #353535; border-radius: 8px; padding: 6px 16px; font-size: 13px; }"
                       "QPushButton:hover { border-color: #4A4A4A; }"));
    connect(refreshButton, &QPushButton::clicked, this, &OpsModule::rebuildTable);
    header->addWidget(refreshButton);
    m_layout->addLayout(header);

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("Time"), QStringLiteral("Entity"), QStringLiteral("Action"),
         QStringLiteral("Target")});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    m_table->setStyleSheet(QStringLiteral(
        "QTableWidget { background: transparent; border: none; color: #C4C4C4; font-size: 13px; }"
        "QTableWidget::item { padding: 6px 8px; border-bottom: 1px solid #232323; }"
        "QTableWidget::item:alternate { background: #171717; }"
        "QHeaderView::section { background: transparent; border: none; color: #6A6A6A;"
        " font-size: 11px; letter-spacing: 1px; padding: 6px 8px; }"));
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_layout->addWidget(m_table, 1);

    m_emptyLabel = new QLabel(QStringLiteral("No activity yet. Complete tasks or run focus sessions."), this);
    m_emptyLabel->setStyleSheet(QStringLiteral("color: #707070; font-size: 14px;"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_emptyLabel, 1);

    connect(context->tasks(), &data::TaskRepository::changed,
            this, &OpsModule::rebuildTable);
    rebuildTable();
}

void OpsModule::onActivated()
{
    rebuildTable();
}

void OpsModule::refreshView()
{
    rebuildTable();
}

void OpsModule::rebuildTable()
{
    QVector<ActivityRow> rows;

    const auto sessions = context()->chronosStorage()->loadSessions();
    for (const chronos::Session& session : sessions) {
        if (!session.startTime.isValid()) continue;
        ActivityRow row;
        row.timestamp = session.endTime.isValid() ? session.endTime : session.startTime;
        row.entity = session.type == chronos::SessionType::Focus
            ? QStringLiteral("Focus") : QStringLiteral("Break");
        row.action = session.completed ? QStringLiteral("completed")
                                       : QStringLiteral("aborted");
        row.target = session.note.isEmpty() ? QStringLiteral("(untitled)") : session.note;
        rows.append(row);
    }

    const auto tasks = context()->tasks()->all();
    for (const models::Task& task : tasks) {
        if (task.status == models::TaskStatus::Completed && task.completedAt.isValid()) {
            ActivityRow row;
            row.timestamp = task.completedAt;
            row.entity = QStringLiteral("Task");
            row.action = QStringLiteral("completed");
            row.target = task.title;
            rows.append(row);
        }
    }

    std::sort(rows.begin(), rows.end(),
              [](const ActivityRow& a, const ActivityRow& b) { return a.timestamp > b.timestamp; });

    m_table->setVisible(!rows.isEmpty());
    m_emptyLabel->setVisible(rows.isEmpty());

    m_table->setRowCount(0);
    const int count = qMin(rows.size(), 80);
    m_table->setRowCount(count);
    for (int i = 0; i < count; ++i) {
        const ActivityRow& row = rows.at(i);
        const auto make = [](const QString& text, const QString& color) {
            auto* item = new QTableWidgetItem(text);
            item->setForeground(QColor(color));
            return item;
        };
        m_table->setItem(i, 0, make(row.timestamp.toString(QStringLiteral("MMM d · HH:mm")),
                                    QStringLiteral("#6A6A6A")));
        m_table->setItem(i, 1, make(row.entity, QStringLiteral("#7A9BC7")));
        m_table->setItem(i, 2, make(row.action, QStringLiteral("#9A9A9A")));
        m_table->setItem(i, 3, make(row.target, QStringLiteral("#D8D8D8")));
    }
}

} // namespace arete::ui
