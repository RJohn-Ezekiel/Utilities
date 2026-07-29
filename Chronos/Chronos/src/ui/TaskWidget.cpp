#include "TaskWidget.h"
#include "Theme.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

namespace chronos {

TaskWidget::TaskWidget(const Task& task, bool isFirst, bool isLast,
                       QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(80);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 6, 12, 6);
    root->setSpacing(2);

    // ── Row 1: checkbox + title + actions ──
    auto* row1 = new QHBoxLayout();
    row1->setSpacing(8);

    m_completeBtn = new QPushButton(this);
    m_completeBtn->setFixedSize(26, 26);
    m_completeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_completeBtn, &QPushButton::clicked, this, [this]() {
        emit completeToggled(m_taskId, !m_completed);
    });

    m_titleLabel = new QLabel(this);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    row1->addWidget(m_completeBtn);
    row1->addWidget(m_titleLabel, 1);

    // Move buttons
    m_moveUpBtn = new QPushButton(QStringLiteral("\u25B2"), this);
    m_moveUpBtn->setFixedSize(22, 22);
    m_moveUpBtn->setToolTip(QStringLiteral("Move up"));
    m_moveUpBtn->setEnabled(!isFirst);
    connect(m_moveUpBtn, &QPushButton::clicked, this, [this]() {
        emit moveUpRequested(m_taskId);
    });

    m_moveDownBtn = new QPushButton(QStringLiteral("\u25BC"), this);
    m_moveDownBtn->setFixedSize(22, 22);
    m_moveDownBtn->setToolTip(QStringLiteral("Move down"));
    m_moveDownBtn->setEnabled(!isLast);
    connect(m_moveDownBtn, &QPushButton::clicked, this, [this]() {
        emit moveDownRequested(m_taskId);
    });

    row1->addWidget(m_moveUpBtn);
    row1->addWidget(m_moveDownBtn);

    // Start button (only for incomplete tasks)
    m_startBtn = new QPushButton(QStringLiteral("Start"), this);
    m_startBtn->setFixedHeight(24);
    m_startBtn->setToolTip(QStringLiteral("Start a focus session for this task"));
    connect(m_startBtn, &QPushButton::clicked, this, [this]() {
        emit startRequested(m_taskId);
    });
    row1->addWidget(m_startBtn);

    m_editBtn = new QPushButton(QStringLiteral("Edit"), this);
    m_editBtn->setFixedHeight(24);
    connect(m_editBtn, &QPushButton::clicked, this, [this]() {
        emit editRequested(m_taskId);
    });
    row1->addWidget(m_editBtn);

    m_deleteBtn = new QPushButton(QStringLiteral("\u2716"), this);
    m_deleteBtn->setFixedSize(24, 24);
    m_deleteBtn->setToolTip(QStringLiteral("Delete task"));
    connect(m_deleteBtn, &QPushButton::clicked, this, [this]() {
        emit deleteRequested(m_taskId);
    });
    row1->addWidget(m_deleteBtn);

    root->addLayout(row1);

    // ── Row 2: description ──
    m_descriptionLabel = new QLabel(this);
    m_descriptionLabel->setContentsMargins(34, 0, 0, 0);
    root->addWidget(m_descriptionLabel);

    // ── Row 3: sessions progress ──
    m_sessionsLabel = new QLabel(this);
    m_sessionsLabel->setContentsMargins(34, 0, 0, 0);
    root->addWidget(m_sessionsLabel);

    // Apply button styles
    const QString btnStyle = QStringLiteral(
        "QPushButton {"
        "  background: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  padding: 2px 10px;"
        "  font-size: 11px;"
        "  border-radius: 0px;"
        "}"
        "QPushButton:hover { background: %4; }"
    ).arg(Theme::Card.name())
     .arg(Theme::PrimaryText.name())
     .arg(Theme::Border.name())
     .arg(Theme::Hover.name());

    for (auto* btn : {m_startBtn, m_editBtn, m_deleteBtn, m_moveUpBtn, m_moveDownBtn}) {
        btn->setStyleSheet(btnStyle);
    }

    updateTask(task, isFirst, isLast);
}

void TaskWidget::updateTask(const Task& task, bool isFirst, bool isLast)
{
    m_taskId = task.id;
    m_completed = task.completed;

    // Title
    m_titleLabel->setText(task.title);

    // Description
    if (task.description.isEmpty()) {
        m_descriptionLabel->setVisible(false);
    } else {
        m_descriptionLabel->setText(task.description);
        m_descriptionLabel->setVisible(true);
    }

    // Sessions
    m_sessionsLabel->setText(
        QStringLiteral("Sessions: %1/%2")
            .arg(task.completedSessions)
            .arg(task.estimatedSessions));

    // Start button visibility
    m_startBtn->setVisible(!task.completed);

    // Move buttons
    m_moveUpBtn->setEnabled(!isFirst);
    m_moveDownBtn->setEnabled(!isLast);

    applyStyle();
}

void TaskWidget::applyStyle()
{
    if (m_completed) {
        m_completeBtn->setText(QStringLiteral("\u2713"));
        m_completeBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: %1;"
            "  color: %2;"
            "  border: 2px solid %1;"
            "  border-radius: 13px;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "}"
        ).arg(Theme::Success.name()).arg(Theme::Background.name()));

        m_titleLabel->setStyleSheet(QStringLiteral(
            "color: %1; text-decoration: line-through; font-size: 14px;"
        ).arg(Theme::SecondaryText.name()));

        m_sessionsLabel->setStyleSheet(QStringLiteral(
            "color: %1; font-size: 11px;"
        ).arg(Theme::Success.name()));
    } else {
        m_completeBtn->setText(QString());
        m_completeBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: transparent;"
            "  color: %1;"
            "  border: 2px solid %1;"
            "  border-radius: 13px;"
            "}"
            "QPushButton:hover { background: %2; }"
        ).arg(Theme::SecondaryText.name()).arg(Theme::Hover.name()));

        m_titleLabel->setStyleSheet(QStringLiteral(
            "color: %1; font-size: 14px; font-weight: normal;"
        ).arg(Theme::PrimaryText.name()));

        m_sessionsLabel->setStyleSheet(QStringLiteral(
            "color: %1; font-size: 11px;"
        ).arg(Theme::SecondaryText.name()));
    }

    m_descriptionLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 11px;"
    ).arg(Theme::SecondaryText.name()));

    // Card background with bottom border
    setStyleSheet(QStringLiteral(
        "TaskWidget { background: %1; border-bottom: 1px solid %2; }"
    ).arg(Theme::Card.name()).arg(Theme::Border.name()));
}

QString TaskWidget::taskId() const { return m_taskId; }

} // namespace chronos
