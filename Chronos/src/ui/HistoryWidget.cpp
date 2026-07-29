#include "HistoryWidget.h"
#include "Theme.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QDate>

namespace chronos {

HistoryWidget::HistoryWidget(StorageManager* storage, QWidget* parent)
    : QWidget(parent)
    , m_storage(storage)
{
    setupUi();
}

void HistoryWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* header = new QLabel(QStringLiteral("Session History"), this);
    header->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 18px; font-weight: bold;"
    ).arg(Theme::PrimaryText.name()));
    root->addWidget(header);

    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Date"),
        QStringLiteral("Type"),
        QStringLiteral("Duration"),
        QStringLiteral("Task"),
        QStringLiteral("Note")
    });

    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->hide();
    m_table->setShowGrid(false);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setColumnWidth(3, 120);
    m_table->setStyleSheet(QStringLiteral(
        "QTableWidget {"
        "  background: transparent;"
        "  color: %1;"
        "  border: 1px solid %2;"
        "  font-size: 12px;"
        "  gridline-color: transparent;"
        "}"
        "QTableWidget::item {"
        "  padding: 6px 10px;"
        "  border-bottom: 1px solid %2;"
        "}"
        "QHeaderView::section {"
        "  background: %3;"
        "  color: %4;"
        "  border: none;"
        "  border-bottom: 1px solid %2;"
        "  padding: 6px 10px;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "}"
    ).arg(Theme::PrimaryText.name())
     .arg(Theme::Border.name())
     .arg(Theme::Toolbar.name())
     .arg(Theme::SecondaryText.name()));

    root->addWidget(m_table, 1);

    refresh();
}

void HistoryWidget::refresh()
{
    m_table->setRowCount(0);
    auto sessions = m_storage->loadSessions();

    // Sort by start time descending
    std::sort(sessions.begin(), sessions.end(), [](const Session& a, const Session& b) {
        return a.startTime > b.startTime;
    });

    for (const auto& session : sessions) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        auto typeStr = [](SessionType t) -> QString {
            switch (t) {
            case SessionType::Focus: return QStringLiteral("Focus");
            case SessionType::ShortBreak: return QStringLiteral("Short Break");
            case SessionType::LongBreak: return QStringLiteral("Long Break");
            }
            return {};
        };

        int mins = session.durationSeconds / 60;
        int secs = session.durationSeconds % 60;
        QString durStr = QStringLiteral("%1:%2")
                             .arg(mins, 2, 10, QChar('0'))
                             .arg(secs, 2, 10, QChar('0'));

        m_table->setItem(row, 0, new QTableWidgetItem(
            session.startTime.toString(QStringLiteral("MMM d, h:mm AP"))));
        m_table->setItem(row, 1, new QTableWidgetItem(typeStr(session.type)));
        m_table->setItem(row, 2, new QTableWidgetItem(durStr));
        m_table->setItem(row, 3, new QTableWidgetItem(
            session.taskId.isEmpty() ? QStringLiteral("--") : QStringLiteral("Linked")));
        m_table->setItem(row, 4, new QTableWidgetItem(
            session.note.isEmpty() ? QString() : session.note));
    }
}

} // namespace chronos
