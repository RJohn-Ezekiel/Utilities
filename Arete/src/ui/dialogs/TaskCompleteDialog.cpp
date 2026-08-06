#include "ui/dialogs/TaskCompleteDialog.h"
#include "app/AppContext.h"
#include "services/TimerService.h"
#include "data/TaskRepository.h"
#include "data/JournalRepository.h"
#include "models/Task.h"
#include "models/JournalEntry.h"
#include "core/Icons.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateEdit>
#include <QDialogButtonBox>

namespace arete::ui {

TaskCompleteDialog::TaskCompleteDialog(app::AppContext* context, QWidget* parent)
    : QDialog(parent), m_context(context)
{
    setWindowTitle(QStringLiteral("Task Complete"));
    setModal(true);
    setMinimumWidth(420);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);

    auto* done = new QLabel(QStringLiteral("Task completed"), this);
    done->setAlignment(Qt::AlignCenter);
    done->setStyleSheet(QStringLiteral("color: #B0B0B0; font-size: 15px; letter-spacing: 1px;"));

    m_title = new QLabel(this);
    m_title->setAlignment(Qt::AlignCenter);
    m_title->setWordWrap(true);
    m_title->setStyleSheet(QStringLiteral("color: #C4C4C4; font-size: 15px;"));

    auto* prompt = new QLabel(QStringLiteral("Task complete. What next?"), this);
    prompt->setAlignment(Qt::AlignCenter);
    prompt->setStyleSheet(QStringLiteral("color: #A0A0A0;"));

    m_startNext = new QPushButton(QStringLiteral("Start next task"), this);
    m_startNext->setProperty("primary", true);
    m_shortBreak = new QPushButton(QStringLiteral("Take a short break"), this);
    m_longBreak = new QPushButton(QStringLiteral("Take a long break"), this);
    m_reschedule = new QPushButton(QStringLiteral("Reschedule"), this);
    m_journal = new QPushButton(QStringLiteral("Journal this task"), this);

    m_dateEdit = new QDateEdit(QDate::currentDate().addDays(1), this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setMinimumDate(QDate::currentDate());
    m_dateEdit->setVisible(false);
    m_dateEdit->setStyleSheet(QStringLiteral("padding: 6px;"));

    for (QPushButton* b : {m_startNext, m_shortBreak, m_longBreak, m_reschedule, m_journal}) {
        layout->addWidget(b);
    }
    layout->addWidget(m_dateEdit);

    layout->addWidget(done);
    layout->addWidget(m_title);
    layout->addWidget(prompt);

    connect(m_startNext, &QPushButton::clicked, this, [this] { choose(Action::StartNext); });
    connect(m_shortBreak, &QPushButton::clicked, this, [this] { choose(Action::ShortBreak); });
    connect(m_longBreak, &QPushButton::clicked, this, [this] { choose(Action::LongBreak); });
    connect(m_reschedule, &QPushButton::clicked, this, [this] {
        // First click: reveal the date picker; second click: confirm.
        if (!m_dateEdit->isVisible()) {
            m_dateEdit->setVisible(true);
            m_reschedule->setText(QStringLiteral("Confirm reschedule"));
            return;
        }
        m_rescheduleDate = m_dateEdit->date();
        choose(Action::Reschedule);
    });
    connect(m_journal, &QPushButton::clicked, this, [this] { choose(Action::Journal); });
}

void TaskCompleteDialog::setCompletedTask(const QString& title)
{
    m_title->setText(QStringLiteral("\u201C%1\u201D").arg(title));
}

void TaskCompleteDialog::choose(Action action)
{
    m_action = action;
    accept();
}

} // namespace arete::ui
