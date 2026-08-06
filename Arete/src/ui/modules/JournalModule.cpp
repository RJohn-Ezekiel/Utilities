#include "ui/modules/JournalModule.h"
#include "app/AppContext.h"
#include "data/JournalRepository.h"
#include "models/JournalEntry.h"
#include "ui/common/SectionLabel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>

namespace arete::ui {

namespace {
const int kStretch = 3;
const int kSmallStretch = 2;

QTextEdit* makeField(QWidget* parent)
{
    auto* field = new QTextEdit(parent);
    field->setAcceptRichText(false);
    field->setPlaceholderText(QStringLiteral("Write here\u2026"));
    field->setStyleSheet(QStringLiteral(
        "QTextEdit { background-color: #1C1C1C; border: 1px solid #2A2A2A;"
        " border-radius: 10px; color: #C4C4C4; padding: 8px; font-size: 13px; }"
        "QTextEdit:focus { border-color: #4A4A4A; }"));
    return field;
}
}

JournalModule::JournalModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(32, 24, 32, 24);
    outer->setSpacing(12);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("Journal"), this);
    title->setStyleSheet(QStringLiteral("font-size: 22px; color: #E2E2E2; letter-spacing: 1px;"));
    header->addWidget(title);
    header->addStretch(1);

    auto* prev = new QPushButton(QStringLiteral("<"), this);
    auto* next = new QPushButton(QStringLiteral(">"), this);
    for (QPushButton* b : {prev, next}) {
        b->setFixedSize(32, 32);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QStringLiteral(
            "QPushButton { color: #A0A0A0; background-color: #242424; border: 1px solid #353535;"
            " border-radius: 8px; font-size: 16px; }"
            "QPushButton:hover { background-color: #2A2A2A; }"));
    }
    connect(prev, &QPushButton::clicked, this, [this] {
        saveCurrent();
        m_date = m_date.addDays(-1);
        load(m_date);
    });
    connect(next, &QPushButton::clicked, this, [this] {
        saveCurrent();
        m_date = m_date.addDays(1);
        load(m_date);
    });
    header->addWidget(prev);

    m_dateLabel = new QLabel(this);
    m_dateLabel->setStyleSheet(QStringLiteral("font-size: 15px; color: #D6D6D6;"));
    header->addWidget(m_dateLabel);
    header->addWidget(next);

    m_streak = new QLabel(this);
    m_streak->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 12px;"));
    header->addWidget(m_streak);

    m_dateEdit = new QDateEdit(this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDate(m_date);
    m_dateEdit->setStyleSheet(QStringLiteral(
        "QDateEdit { background-color: #242424; border: 1px solid #353535; border-radius: 8px;"
        " color: #C4C4C4; padding: 4px 8px; }"));
    connect(m_dateEdit, &QDateEdit::dateChanged, this, [this](const QDate& date) {
        saveCurrent();
        m_date = date;
        load(date);
    });
    header->addWidget(m_dateEdit);
    outer->addLayout(header);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; }"));

    auto* content = new QWidget;
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 8, 8, 8);
    layout->setSpacing(10);
    layout->setAlignment(Qt::AlignTop);

    m_reflection = makeField(content);
    m_gratitude = makeField(content);
    m_mistakes = makeField(content);
    m_lessons = makeField(content);
    m_scripture = makeField(content);
    m_tomorrow = makeField(content);

    layout->addWidget(new common::SectionLabel(QStringLiteral("REFLECTION"), content));
    layout->addWidget(m_reflection, kStretch);
    layout->addWidget(new common::SectionLabel(QStringLiteral("GRATITUDE"), content));
    layout->addWidget(m_gratitude, kStretch);
    layout->addWidget(new common::SectionLabel(QStringLiteral("MISTAKES"), content));
    layout->addWidget(m_mistakes, kStretch);
    layout->addWidget(new common::SectionLabel(QStringLiteral("LESSONS"), content));
    layout->addWidget(m_lessons, kStretch);
    layout->addWidget(new common::SectionLabel(QStringLiteral("SCRIPTURE"), content));
    layout->addWidget(m_scripture, kSmallStretch);
    layout->addWidget(new common::SectionLabel(QStringLiteral("TOMORROW"), content));
    layout->addWidget(m_tomorrow, kSmallStretch);

    scroll->setWidget(content);
    outer->addWidget(scroll, 1);
}

void JournalModule::onActivated()
{
    load(m_date);
}

void JournalModule::refreshView()
{
    load(m_date);
}

void JournalModule::setDateLabel()
{
    m_dateLabel->setText(m_date.toString(QStringLiteral("dddd, MMMM d, yyyy")).toUpper());
    m_streak->setText(QStringLiteral("\u2014 %1 day streak").arg(context()->journal()->streak()));
}

void JournalModule::saveCurrent()
{
    models::JournalEntry entry = context()->journal()->forDate(m_date);
    entry.date = m_date;
    entry.reflection = m_reflection->toPlainText();
    entry.gratitude = m_gratitude->toPlainText();
    entry.mistakes = m_mistakes->toPlainText();
    entry.lessons = m_lessons->toPlainText();
    entry.scripture = m_scripture->toPlainText();
    entry.tomorrow = m_tomorrow->toPlainText();
    entry.updatedAt = QDateTime::currentDateTime();
    context()->journal()->upsert(entry);
}

void JournalModule::load(const QDate& date)
{
    setDateLabel();
    m_dateEdit->blockSignals(true);
    m_dateEdit->setDate(date);
    m_dateEdit->blockSignals(false);

    const models::JournalEntry entry = context()->journal()->forDate(date);
    m_reflection->setPlainText(entry.reflection);
    m_gratitude->setPlainText(entry.gratitude);
    m_mistakes->setPlainText(entry.mistakes);
    m_lessons->setPlainText(entry.lessons);
    m_scripture->setPlainText(entry.scripture);
    m_tomorrow->setPlainText(entry.tomorrow);
}

void JournalModule::openDate(const QDate& date)
{
    saveCurrent();
    m_date = date;
    load(date);
}

} // namespace arete::ui
