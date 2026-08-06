#include "arete/widgets/NotificationsPanel.h"
#include "arete/notifications/NotificationCenter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QToolButton>
#include <QProgressBar>
#include <QScrollArea>
#include <QFrame>
#include <QTimer>
#include <QDateTime>

namespace arete::widgets {

namespace {

QColor typeColor(notifications::Type type)
{
    switch (type) {
        case notifications::Type::Info: return QColor(0xA0, 0xA0, 0xA0);
        case notifications::Type::Success: return QColor(0x6A, 0xA0, 0x6A);
        case notifications::Type::Warning: return QColor(0xC4, 0xA0, 0x50);
        case notifications::Type::Error: return QColor(0xC4, 0x50, 0x50);
        case notifications::Type::Progress: return QColor(0x8A, 0x8A, 0x8A);
    }
    return QColor(0xA0, 0xA0, 0xA0);
}

QString typeLabel(notifications::Type type)
{
    switch (type) {
        case notifications::Type::Info: return QStringLiteral("INFO");
        case notifications::Type::Success: return QStringLiteral("OK");
        case notifications::Type::Warning: return QStringLiteral("WARN");
        case notifications::Type::Error: return QStringLiteral("ERROR");
        case notifications::Type::Progress: return QStringLiteral("PROG");
    }
    return QString();
}

} // namespace

class NotificationsPanel::Private
{
public:
    notifications::NotificationCenter* center = nullptr;
    QWidget* listHost = nullptr;
    QVBoxLayout* listLayout = nullptr;
    QScrollArea* scrollArea = nullptr;
    QComboBox* filter = nullptr;
    QPushButton* clearButton = nullptr;
    QLabel* countLabel = nullptr;
    QTimer* refreshTimer = nullptr;
    int typeFilter = 0;
    QHash<QString, QFrame*> rowWidgets;
};


NotificationsPanel::~NotificationsPanel() = default;

NotificationsPanel::NotificationsPanel(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(8);

    // Header
    auto* header = new QHBoxLayout;
    auto* heading = new QLabel(QStringLiteral("Notifications"), this);
    heading->setProperty("heading", true);
    heading->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 600;"));
    header->addWidget(heading);
    header->addStretch();

    d->filter = new QComboBox(this);
    d->filter->addItem(QStringLiteral("All"));
    d->filter->addItem(QStringLiteral("Info"));
    d->filter->addItem(QStringLiteral("Success"));
    d->filter->addItem(QStringLiteral("Warnings"));
    d->filter->addItem(QStringLiteral("Errors"));
    d->filter->addItem(QStringLiteral("Progress"));
    header->addWidget(d->filter);

    d->clearButton = new QPushButton(QStringLiteral("Clear All"), this);
    d->clearButton->setProperty("destructive", true);
    header->addWidget(d->clearButton);
    rootLayout->addLayout(header);

    // Scrollable list
    d->scrollArea = new QScrollArea(this);
    d->scrollArea->setWidgetResizable(true);
    d->scrollArea->setFrameShape(QFrame::NoFrame);
    d->scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { background-color: transparent; border: none; }"
        "QScrollArea > QWidget > QWidget { background-color: transparent; }"));
    d->listHost = new QWidget;
    d->listLayout = new QVBoxLayout(d->listHost);
    d->listLayout->setContentsMargins(0, 0, 0, 0);
    d->listLayout->setSpacing(6);
    d->listLayout->addStretch();
    d->scrollArea->setWidget(d->listHost);
    rootLayout->addWidget(d->scrollArea, 1);

    // Footer
    auto* footer = new QHBoxLayout;
    d->countLabel = new QLabel(QStringLiteral("0 notifications"), this);
    d->countLabel->setStyleSheet(QStringLiteral("color: #A0A0A0;"));
    footer->addWidget(d->countLabel);
    footer->addStretch();
    rootLayout->addLayout(footer);

    connect(d->filter, &QComboBox::currentIndexChanged, this, [this](int index) {
        d->typeFilter = index;
        refresh();
    });
    connect(d->clearButton, &QPushButton::clicked, this, [this]() {
        clearAll();
    });

    d->refreshTimer = new QTimer(this);
    d->refreshTimer->setInterval(1000);
    connect(d->refreshTimer, &QTimer::timeout, this, [this]() { refresh(); });
    d->refreshTimer->start();

    setNotificationCenter(&notifications::NotificationCenter::instance());
}

void NotificationsPanel::setNotificationCenter(notifications::NotificationCenter* center)
{
    if (center == d->center) {
        refresh();
        return;
    }
    d->center = center;
    if (center) {
        connect(center, &notifications::NotificationCenter::notificationAdded,
                this, [this]() { refresh(); });
        connect(center, &notifications::NotificationCenter::notificationRemoved,
                this, [this]() { refresh(); });
        connect(center, &notifications::NotificationCenter::cleared,
                this, [this]() { refresh(); });
    }
    refresh();
}

int NotificationsPanel::notificationCount() const
{
    return d->center ? d->center->model()->count() : 0;
}

int NotificationsPanel::unreadCount() const
{
    return d->center ? d->center->model()->unreadCount() : 0;
}

void NotificationsPanel::dismiss(const QString& id)
{
    if (d->center) {
        d->center->dismiss(id);
        emit notificationDismissed(id);
    }
}

void NotificationsPanel::clearAll()
{
    if (d->center) {
        d->center->dismissAll();
        emit panelCleared();
    }
}

void NotificationsPanel::setTypeFilter(int index)
{
    d->typeFilter = index;
    if (d->filter) d->filter->setCurrentIndex(index);
    refresh();
}

void NotificationsPanel::refresh()
{
    // Remove old rows
    for (auto it = d->rowWidgets.begin(); it != d->rowWidgets.end(); ++it) {
        if (it.value()) it.value()->deleteLater();
    }
    d->rowWidgets.clear();

    if (!d->center) {
        if (d->countLabel) d->countLabel->setText(QStringLiteral("0 notifications"));
        return;
    }

    const auto notifications = d->center->model()->notifications();
    int visible = 0;
    for (const auto& n : notifications) {
        if (d->typeFilter > 0) {
            const notifications::Type filterType =
                static_cast<notifications::Type>(d->typeFilter - 1);
            if (n.type != filterType) continue;
        }

        auto* frame = new QFrame(d->listHost);
        frame->setObjectName(QStringLiteral("notificationRow"));
        frame->setStyleSheet(QStringLiteral(
            "#notificationRow {"
            "  background-color: #1A1A1A;"
            "  border: 1px solid #353535;"
            "  border-radius: 4px;"
            "}"));
        auto* layout = new QVBoxLayout(frame);
        layout->setContentsMargins(10, 8, 10, 8);
        layout->setSpacing(4);

        // Top row: type + time + dismiss
        auto* topRow = new QHBoxLayout;
        auto* typeBadge = new QLabel(typeLabel(n.type), frame);
        typeBadge->setStyleSheet(QStringLiteral("color: %1; font-size: 11px; font-weight: 600;")
                                     .arg(typeColor(n.type).name()));
        topRow->addWidget(typeBadge);

        if (n.type == notifications::Type::Progress) {
            auto* progress = new QProgressBar(frame);
            progress->setRange(0, 100);
            progress->setValue(n.data.value(QStringLiteral("progress"), 0).toInt());
            progress->setFixedHeight(6);
            progress->setTextVisible(false);
            topRow->addWidget(progress, 1);
        } else {
            topRow->addStretch();
        }

        auto* timeLabel = new QLabel(n.timestamp.time().toString(QStringLiteral("hh:mm")), frame);
        timeLabel->setStyleSheet(QStringLiteral("color: #6E6E6E; font-size: 11px;"));
        topRow->addWidget(timeLabel);

        if (n.dismissible) {
            auto* dismissButton = new QToolButton(frame);
            dismissButton->setText(QStringLiteral("✕"));
            dismissButton->setStyleSheet(QStringLiteral(
                "QToolButton { background: transparent; border: none; color: #6E6E6E; font-size: 12px; }"
                "QToolButton:hover { color: #D0D0D0; }"));
            dismissButton->setToolTip(QStringLiteral("Dismiss"));
            dismissButton->setCursor(Qt::PointingHandCursor);
            const QString id = n.id;
            connect(dismissButton, &QToolButton::clicked, this, [this, id]() { dismiss(id); });
            topRow->addWidget(dismissButton);
        }
        layout->addLayout(topRow);

        // Title
        auto* titleLabel = new QLabel(n.title, frame);
        titleLabel->setStyleSheet(QStringLiteral("color: #C4C4C4; font-weight: 600;"));
        titleLabel->setWordWrap(true);
        layout->addWidget(titleLabel);

        // Message
        if (!n.message.isEmpty()) {
            auto* messageLabel = new QLabel(n.message, frame);
            messageLabel->setStyleSheet(QStringLiteral("color: #A0A0A0;"));
            messageLabel->setWordWrap(true);
            layout->addWidget(messageLabel);
        }

        // Action button
        if (!n.actionText.isEmpty()) {
            auto* actionButton = new QPushButton(n.actionText, frame);
            actionButton->setStyleSheet(QStringLiteral(
                "QPushButton { background-color: #2A2A2A; color: #D0D0D0;"
                "  border: 1px solid #353535; padding: 3px 10px; font-size: 12px; }"));
            const QString id = n.id;
            connect(actionButton, &QPushButton::clicked, this, [this, id]() {
                emit actionRequested(id);
            });
            layout->addWidget(actionButton, 0, Qt::AlignRight);
        }

        // Insert before stretch
        d->listLayout->insertWidget(d->listLayout->count() - 1, frame);
        d->rowWidgets.insert(n.id, frame);
        ++visible;
    }

    if (d->countLabel) {
        d->countLabel->setText(QStringLiteral("%1 notifications (%2 unread)")
                                   .arg(notifications.size())
                                   .arg(d->center->model()->unreadCount()));
    }
}

} // namespace arete::widgets