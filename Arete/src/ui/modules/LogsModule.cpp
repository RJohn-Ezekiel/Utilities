#include "ui/modules/LogsModule.h"
#include "app/AppContext.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace arete::ui {

LogsModule::LogsModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
    , m_model(arete::logging::Logger::instance().model())
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(48, 32, 48, 32);
    outer->setSpacing(16);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("Logs"), this);
    title->setStyleSheet(QStringLiteral("font-size: 22px; color: #E2E2E2; letter-spacing: 1px;"));
    header->addWidget(title);
    header->addStretch(1);

    auto* clearButton = new QPushButton(QStringLiteral("Clear"), this);
    clearButton->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #242424; color: #C4C4C4;"
                       " border: 1px solid #353535; border-radius: 8px; padding: 6px 16px; font-size: 13px; }"
                       "QPushButton:hover { border-color: #4A4A4A; }"));
    connect(clearButton, &QPushButton::clicked, this, [this] {
        arete::logging::Logger::instance().clear();
        rebuildList();
    });
    header->addWidget(clearButton);
    outer->addLayout(header);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; }"));

    m_listHost = new QWidget;
    m_listLayout = new QVBoxLayout(m_listHost);
    m_listLayout->setContentsMargins(0, 0, 8, 0);
    m_listLayout->setSpacing(2);
    m_listLayout->setAlignment(Qt::AlignTop);
    scroll->setWidget(m_listHost);
    outer->addWidget(scroll, 1);

    m_emptyLabel = new QLabel(QStringLiteral("No log entries."), m_listHost);
    m_emptyLabel->setStyleSheet(QStringLiteral("color: #707070; font-size: 14px;"));
    m_listLayout->addWidget(m_emptyLabel);

    // Queued: the model emits signals while holding its mutex, so the
    // handler must not call back into the model synchronously.
    connect(m_model, &arete::logging::LogModel::entriesChanged,
            this, &LogsModule::rebuildList, Qt::QueuedConnection);
}

void LogsModule::rebuildList()
{
    while (auto* item = m_listLayout->takeAt(0)) {
        if (auto* widget = item->widget()) widget->deleteLater();
        delete item;
    }

    const auto entries = m_model->entries();
    if (entries.isEmpty()) {
        m_listLayout->addWidget(m_emptyLabel);
        return;
    }

    for (const auto& e : entries) {
        auto* row = new QWidget(m_listHost);
        row->setStyleSheet(QStringLiteral("background-color: #1C1C1C; border-radius: 6px;"));
        auto* layout = new QHBoxLayout(row);
        layout->setContentsMargins(12, 6, 12, 6);
        layout->setSpacing(10);

        auto* stamp = new QLabel(e.timestamp.toString(QStringLiteral("HH:mm:ss")), row);
        stamp->setStyleSheet(QStringLiteral("color: #6A6A6A; font-size: 12px;"));
        layout->addWidget(stamp);

        auto* level = new QLabel(levelName(e.level), row);
        level->setStyleSheet(QStringLiteral("color: %1; font-size: 11px; font-weight: 600;")
                                 .arg(levelColor(e.level)));
        level->setFixedWidth(52);
        layout->addWidget(level);

        if (!e.category.isEmpty()) {
            auto* category = new QLabel(e.category, row);
            category->setStyleSheet(QStringLiteral("color: #7A9BC7; font-size: 12px;"));
            category->setFixedWidth(110);
            layout->addWidget(category);
        }

        auto* message = new QLabel(e.message, row);
        message->setStyleSheet(QStringLiteral("color: #B0B0B0; font-size: 12px;"));
        message->setWordWrap(true);
        layout->addWidget(message, 1);

        m_listLayout->addWidget(row);
    }
    m_listLayout->addStretch(1);
}

QString LogsModule::levelName(arete::logging::Level level) const
{
    switch (level) {
    case arete::logging::Level::Trace: return QStringLiteral("TRACE");
    case arete::logging::Level::Debug: return QStringLiteral("DEBUG");
    case arete::logging::Level::Info: return QStringLiteral("INFO");
    case arete::logging::Level::Warning: return QStringLiteral("WARN");
    case arete::logging::Level::Error: return QStringLiteral("ERROR");
    case arete::logging::Level::Critical: return QStringLiteral("FATAL");
    }
    return QStringLiteral("INFO");
}

QString LogsModule::levelColor(arete::logging::Level level) const
{
    switch (level) {
    case arete::logging::Level::Warning: return QStringLiteral("#D9A05B");
    case arete::logging::Level::Error:
    case arete::logging::Level::Critical: return QStringLiteral("#D96B6B");
    case arete::logging::Level::Trace:
    case arete::logging::Level::Debug: return QStringLiteral("#8A8A8A");
    case arete::logging::Level::Info: return QStringLiteral("#7FBF7F");
    }
    return QStringLiteral("#7FBF7F");
}

void LogsModule::onActivated()
{
}

void LogsModule::refreshView()
{
    rebuildList();
}

} // namespace arete::ui