#include "ui/modules/AlertsModule.h"
#include "app/AppContext.h"
#include "arete/notifications/NotificationCenter.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace arete::ui {

AlertsModule::AlertsModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
    , m_model(arete::notifications::NotificationCenter::instance().model())
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(48, 32, 48, 32);
    outer->setSpacing(16);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("Alerts"), this);
    title->setStyleSheet(QStringLiteral("font-size: 22px; color: #E2E2E2; letter-spacing: 1px;"));
    header->addWidget(title);
    header->addStretch(1);

    auto* clearButton = new QPushButton(QStringLiteral("Clear"), this);
    clearButton->setStyleSheet(
        QStringLiteral("QPushButton { background-color: #242424; color: #C4C4C4;"
                       " border: 1px solid #353535; border-radius: 8px; padding: 6px 16px; font-size: 13px; }"
                       "QPushButton:hover { border-color: #4A4A4A; }"));
    connect(clearButton, &QPushButton::clicked, this, [this] {
        arete::notifications::NotificationCenter::instance().dismissAll();
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
    m_listLayout->setSpacing(8);
    m_listLayout->setAlignment(Qt::AlignTop);
    scroll->setWidget(m_listHost);
    outer->addWidget(scroll, 1);

    m_emptyLabel = new QLabel(QStringLiteral("No alerts yet."), m_listHost);
    m_emptyLabel->setStyleSheet(QStringLiteral("color: #707070; font-size: 14px;"));
    m_listLayout->addWidget(m_emptyLabel);

    // Queued: the model emits signals while holding its mutex, so the
    // handler must not call back into the model synchronously.
    connect(m_model, &arete::notifications::NotificationModel::notificationsChanged,
            this, &AlertsModule::rebuildList, Qt::QueuedConnection);
}

void AlertsModule::rebuildList()
{
    while (auto* item = m_listLayout->takeAt(0)) {
        if (auto* widget = item->widget()) widget->deleteLater();
        delete item;
    }

    const auto notifications = m_model->notifications();
    if (notifications.isEmpty()) {
        m_listLayout->addWidget(m_emptyLabel);
        return;
    }

    for (const auto& n : notifications) {
        auto* card = new QWidget(m_listHost);
        card->setStyleSheet(QStringLiteral("background-color: #1C1C1C; border-radius: 10px;"));
        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 12, 16, 12);
        cardLayout->setSpacing(4);

        auto* head = new QHBoxLayout;
        auto* typeLabel = new QLabel(typeName(n.type), card);
        typeLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 11px; letter-spacing: 1px;")
                                     .arg(typeColor(n.type)));
        head->addWidget(typeLabel);
        head->addStretch(1);
        auto* timeLabel = new QLabel(
            n.timestamp.toString(QStringLiteral("MMM d · HH:mm")), card);
        timeLabel->setStyleSheet(QStringLiteral("color: #6A6A6A; font-size: 12px;"));
        head->addWidget(timeLabel);
        cardLayout->addLayout(head);

        auto* titleLabel = new QLabel(n.title, card);
        titleLabel->setStyleSheet(QStringLiteral("color: #D8D8D8; font-size: 14px; font-weight: 600;"));
        titleLabel->setWordWrap(true);
        cardLayout->addWidget(titleLabel);

        if (!n.message.isEmpty()) {
            auto* messageLabel = new QLabel(n.message, card);
            messageLabel->setStyleSheet(QStringLiteral("color: #9A9A9A; font-size: 13px;"));
            messageLabel->setWordWrap(true);
            cardLayout->addWidget(messageLabel);
        }

        m_listLayout->addWidget(card);
    }
    m_listLayout->addStretch(1);
}

QString AlertsModule::typeName(arete::notifications::Type type) const
{
    switch (type) {
    case arete::notifications::Type::Info: return QStringLiteral("INFO");
    case arete::notifications::Type::Success: return QStringLiteral("OK");
    case arete::notifications::Type::Warning: return QStringLiteral("WARNING");
    case arete::notifications::Type::Error: return QStringLiteral("ERROR");
    case arete::notifications::Type::Progress: return QStringLiteral("PROGRESS");
    }
    return QStringLiteral("INFO");
}

QString AlertsModule::typeColor(arete::notifications::Type type) const
{
    switch (type) {
    case arete::notifications::Type::Success: return QStringLiteral("#7FBF7F");
    case arete::notifications::Type::Warning: return QStringLiteral("#D9A05B");
    case arete::notifications::Type::Error: return QStringLiteral("#D96B6B");
    case arete::notifications::Type::Info:
    case arete::notifications::Type::Progress: return QStringLiteral("#7A9BC7");
    }
    return QStringLiteral("#7A9BC7");
}

void AlertsModule::onActivated()
{
}

void AlertsModule::refreshView()
{
    rebuildList();
}

} // namespace arete::ui