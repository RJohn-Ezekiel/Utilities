#include "SessionCompleteDialog.h"
#include "Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace chronos {

SessionCompleteDialog::SessionCompleteDialog(int durationMinutes,
                                             const QString& taskTitle,
                                             QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Session Complete"));
    setFixedSize(380, 240);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(12);

    auto* titleLabel = new QLabel(QStringLiteral("Focus Session Complete"), this);
    titleLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 16px; font-weight: bold;"
    ).arg(Theme::PrimaryText.name()));
    root->addWidget(titleLabel);

    auto* infoLabel = new QLabel(
        QStringLiteral("%1 minute session completed.").arg(durationMinutes), this);
    infoLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 13px;"
    ).arg(Theme::SecondaryText.name()));
    root->addWidget(infoLabel);

    if (!taskTitle.isEmpty()) {
        auto* taskLabel = new QLabel(
            QStringLiteral("Task: %1").arg(taskTitle), this);
        taskLabel->setStyleSheet(QStringLiteral(
            "color: %1; font-size: 13px;"
        ).arg(Theme::Accent.name()));
        root->addWidget(taskLabel);
    }

    // Note
    auto* noteLabel = new QLabel(QStringLiteral("Session note (optional):"), this);
    noteLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 12px;"
    ).arg(Theme::SecondaryText.name()));
    root->addWidget(noteLabel);

    m_noteEdit = new QLineEdit(this);
    m_noteEdit->setPlaceholderText(QStringLiteral("What did you work on?"));
    root->addWidget(m_noteEdit);

    root->addStretch();

    // Buttons
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    const QString btnStyle = QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %3;"
        "  padding: 6px 14px; font-size: 12px; }"
        "QPushButton:hover { background: %4; }"
    ).arg(Theme::Card.name()).arg(Theme::PrimaryText.name())
     .arg(Theme::Border.name()).arg(Theme::Hover.name());

    const QString accentStyle = QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %1;"
        "  padding: 6px 14px; font-size: 12px; }"
        "QPushButton:hover { background: %3; }"
    ).arg(Theme::Accent.name()).arg(Theme::Background.name())
     .arg(Theme::AccentDim.name());

    m_markDoneBtn = new QPushButton(QStringLiteral("Mark Done && Break"), this);
    m_markDoneBtn->setStyleSheet(accentStyle);
    connect(m_markDoneBtn, &QPushButton::clicked, this, [this]() {
        m_result.action = Result::ProceedToBreak;
        m_result.markTaskCompleted = true;
        m_result.note = m_noteEdit->text().trimmed();
        accept();
    });
    btnRow->addWidget(m_markDoneBtn);

    m_proceedBtn = new QPushButton(QStringLiteral("Continue"), this);
    m_proceedBtn->setStyleSheet(btnStyle);
    connect(m_proceedBtn, &QPushButton::clicked, this, [this]() {
        m_result.action = Result::ProceedToBreak;
        m_result.note = m_noteEdit->text().trimmed();
        accept();
    });
    btnRow->addWidget(m_proceedBtn);

    m_skipBtn = new QPushButton(QStringLiteral("Skip Break"), this);
    m_skipBtn->setStyleSheet(btnStyle);
    connect(m_skipBtn, &QPushButton::clicked, this, [this]() {
        m_result.action = Result::SkipBreak;
        m_result.note = m_noteEdit->text().trimmed();
        accept();
    });
    btnRow->addWidget(m_skipBtn);

    root->addLayout(btnRow);

    // Style dialog
    setStyleSheet(QStringLiteral(
        "QDialog { background: %1; }"
        "QLineEdit { background: %2; color: %3; border: 1px solid %4;"
        "  padding: 6px 8px; font-size: 13px; }"
    ).arg(Theme::Background.name()).arg(Theme::Card.name())
     .arg(Theme::PrimaryText.name()).arg(Theme::Border.name()));
}

SessionCompleteDialog::Result SessionCompleteDialog::result() const
{
    return m_result;
}

} // namespace chronos
