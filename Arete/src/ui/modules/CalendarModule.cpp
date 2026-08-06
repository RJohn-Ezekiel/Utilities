#include "ui/modules/CalendarModule.h"
#include "app/AppContext.h"
#include "data/EventRepository.h"
#include "models/CalendarEvent.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QScrollArea>
#include <QHash>
#include <QDateTime>
#include <QDateEdit>
#include <QTimeEdit>
#include <QCheckBox>
#include <QLineEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QMouseEvent>
#include <functional>

namespace arete::ui {

namespace {
QString kWeekdayLabels[7] = {QStringLiteral("M"), QStringLiteral("T"), QStringLiteral("W"),
                             QStringLiteral("T"), QStringLiteral("F"), QStringLiteral("S"),
                             QStringLiteral("S")};

// Day cell: QPushButton with a double-click hook (QAbstractButton has no
// doubleClicked signal).
class DayCell : public QPushButton
{
public:
    std::function<void()> onDoubleClick;

    explicit DayCell(QWidget* parent = nullptr) : QPushButton(parent) {}

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && onDoubleClick) {
            onDoubleClick();
            event->accept();
            return;
        }
        QPushButton::mouseDoubleClickEvent(event);
    }
};
}

CalendarModule::CalendarModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(32, 24, 32, 24);
    outer->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("Calendar"), this);
    title->setStyleSheet(QStringLiteral("font-size: 22px; color: #E2E2E2; letter-spacing: 1px;"));
    outer->addWidget(title);

    auto* body = new QHBoxLayout;
    body->setSpacing(24);

    // ── Month grid ──
    auto* monthPanel = new QVBoxLayout;
    auto* nav = new QHBoxLayout;
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
        m_cursor = m_cursor.addMonths(-1);
        rebuildGrid();
    });
    connect(next, &QPushButton::clicked, this, [this] {
        m_cursor = m_cursor.addMonths(1);
        rebuildGrid();
    });
    nav->addWidget(prev);
    m_monthLabel = new QLabel(this);
    m_monthLabel->setStyleSheet(QStringLiteral("font-size: 15px; color: #D6D6D6;"));
    nav->addWidget(m_monthLabel);
    nav->addWidget(next);
    nav->addStretch(1);
    monthPanel->addLayout(nav);

    auto* weekdayRow = new QHBoxLayout;
    weekdayRow->setSpacing(4);
    for (const QString& label : kWeekdayLabels) {
        auto* wd = new QLabel(label, this);
        wd->setAlignment(Qt::AlignCenter);
        wd->setStyleSheet(QStringLiteral("color: #5A5A5A; font-size: 11px;"));
        weekdayRow->addWidget(wd, 1);
    }
    monthPanel->addLayout(weekdayRow);

    m_grid = new QGridLayout;
    m_grid->setSpacing(4);
    monthPanel->addLayout(m_grid);
    monthPanel->addStretch(1);

    // ── Day panel ──
    auto* dayPanel = new QVBoxLayout;
    auto* dayHead = new QHBoxLayout;
    m_dayLabel = new QLabel(this);
    m_dayLabel->setStyleSheet(QStringLiteral("font-size: 15px; color: #D6D6D6;"));
    dayHead->addWidget(m_dayLabel);
    dayHead->addStretch(1);

    auto* addButton = new QPushButton(QStringLiteral("+  EVENT"), this);
    addButton->setCursor(Qt::PointingHandCursor);
    addButton->setStyleSheet(QStringLiteral(
        "QPushButton { font-size: 11px; letter-spacing: 1px; color: #D6D6D6;"
        " background-color: #242424; border: 1px solid #353535; border-radius: 8px;"
        " padding: 5px 12px; }"
        "QPushButton:hover { background-color: #2A2A2A; }"));
    connect(addButton, &QPushButton::clicked, this, [this] { showEventEditor(); });
    dayHead->addWidget(addButton);
    dayPanel->addLayout(dayHead);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* scrollContent = new QWidget;
    auto* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 8, 0);
    scrollLayout->setAlignment(Qt::AlignTop);
    m_dayList = new QLabel(this);
    m_dayList->setTextFormat(Qt::RichText);
    m_dayList->setWordWrap(true);
    m_dayList->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    m_dayList->setOpenExternalLinks(false);
    m_dayList->setStyleSheet(QStringLiteral("color: #A0A0A0; font-size: 13px;"));
    connect(m_dayList, &QLabel::linkActivated, this, [this, context](const QString& href) {
        if (href.startsWith(QLatin1String("edit:"))) {
            const qint64 id = href.mid(5).toLongLong();
            const models::CalendarEvent event = context->events()->byId(id);
            if (event.id >= 0) showEventEditor(&event);
        } else if (href.startsWith(QLatin1String("del:"))) {
            const qint64 id = href.mid(4).toLongLong();
            const models::CalendarEvent event = context->events()->byId(id);
            if (event.id < 0) return;
            const auto answer = QMessageBox::question(
                this, QStringLiteral("Delete event"),
                QStringLiteral("Remove \u201C%1\u201D?").arg(event.title),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer == QMessageBox::Yes) {
                context->events()->remove(id);
                onActivated();
            }
        }
    });
    scrollLayout->addWidget(m_dayList);
    scroll->setWidget(scrollContent);
    dayPanel->addWidget(scroll, 1);

    body->addLayout(monthPanel, 3);
    body->addLayout(dayPanel, 2);
    outer->addLayout(body, 1);
}

void CalendarModule::rebuildGrid()
{
    while (QLayoutItem* item = m_grid->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }

    m_monthLabel->setText(m_cursor.toString(QStringLiteral("MMMM yyyy")).toUpper());

    const QDate first = QDate(m_cursor.year(), m_cursor.month(), 1);
    const int offset = (first.dayOfWeek() - 1 + 7) % 7; // Monday-start
    const int daysInMonth = m_cursor.daysInMonth();

    // Gather the month's events once.
    const QDateTime monthStart = QDateTime(first, QTime(0, 0));
    const QDateTime monthEnd = QDateTime(QDate(m_cursor.year(), m_cursor.month(), daysInMonth), QTime(23, 59));
    const auto events = context()->events()->between(monthStart, monthEnd);

    QHash<QDate, int> counts;
    for (const models::CalendarEvent& e : events) {
        const QDate day = e.start.date();
        if (day.month() == m_cursor.month() && day.year() == m_cursor.year()) {
            ++counts[day];
        }
    }

    int dayNumber = 1;
    const int totalCells = 42;
    for (int cell = 0; cell < totalCells; ++cell) {
        const int row = cell / 7;
        const int col = cell % 7;
        const int gridIndex = row * 7 + col;

        auto* dayCell = new DayCell(this);
        dayCell->setCursor(Qt::PointingHandCursor);
        dayCell->setStyleSheet(QStringLiteral(
            "QPushButton { color: #7A7A7A; background-color: #1C1C1C; border: 1px solid #2A2A2A;"
            " border-radius: 10px; font-size: 13px; padding: 10px 0; }"
            "QPushButton:hover { background-color: #262626; }"));

        if (gridIndex >= offset && dayNumber <= daysInMonth) {
            const QDate date(m_cursor.year(), m_cursor.month(), dayNumber);
            dayCell->setText(QString::number(dayNumber));
            dayCell->setStyleSheet(QStringLiteral(
                "QPushButton { color: #C4C4C4; background-color: #1C1C1C;"
                " border: 1px solid #2A2A2A; border-radius: 10px; font-size: 13px;"
                " padding: 10px 0; }"
                "QPushButton:hover { background-color: #262626; }"));
            const int count = counts.value(date, 0);
            if (count > 0) {
                dayCell->setText(QStringLiteral("%1\n\u00B7 %2").arg(dayNumber).arg(count));
            }
            if (date == QDate::currentDate()) {
                dayCell->setStyleSheet(QStringLiteral(
                    "QPushButton { color: #181818; background-color: #D0D0D0;"
                    " border-radius: 10px; font-size: 13px; padding: 10px 0; font-weight: 600; }"));
            } else if (date == m_selected) {
                dayCell->setStyleSheet(QStringLiteral(
                    "QPushButton { color: #E2E2E2; background-color: #2E2E2E;"
                    " border: 1px solid #4A4A4A; border-radius: 10px; font-size: 13px;"
                    " padding: 10px 0; }"));
            }
            connect(dayCell, &QPushButton::clicked, this, [this, date] {
                m_selected = date;
                rebuildGrid();
                refreshSidePanel();
            });
            dayCell->onDoubleClick = [this, date] {
                m_selected = date;
                showEventEditor(nullptr, date);
            };
            ++dayNumber;
        } else {
            dayCell->setEnabled(false);
        }
        m_grid->addWidget(dayCell, row, col);
    }
}

void CalendarModule::refreshSidePanel()
{
    m_dayLabel->setText(m_selected.toString(QStringLiteral("dddd, MMMM d")).toUpper());

    QString html;
    const QDateTime dayStart(m_selected, QTime(0, 0));
    const QDateTime dayEnd(m_selected, QTime(23, 59, 59));
    const auto events = context()->events()->between(dayStart, dayEnd);
    if (events.isEmpty()) {
        html = QStringLiteral("<span style='color:#7A7A7A'>No events. Double-click a day to add one.</span>");
    } else {
        QStringList lines;
        for (const models::CalendarEvent& e : events) {
            QString range;
            if (e.allDay) {
                range = QStringLiteral("all day");
            } else {
                range = QStringLiteral("%1 \u2013 %2")
                            .arg(e.start.time().toString(QStringLiteral("HH:mm")))
                            .arg(e.end.time().toString(QStringLiteral("HH:mm")));
            }
            lines.append(QStringLiteral(
                             "<span style='color:#7A7A7A'>%1</span> "
                             "<a href='edit:%2' style='color:#D6D6D6; text-decoration:none;'>%3</a> "
                             "<a href='del:%4' style='color:#6A6A6A; text-decoration:none;'>[x]</a>")
                             .arg(range)
                             .arg(e.id)
                             .arg(e.title.toHtmlEscaped())
                             .arg(e.id));
        }
        html = lines.join(QStringLiteral("<br>"));
    }
    m_dayList->setText(html);
}

void CalendarModule::showEventEditor(const models::CalendarEvent* existing, const QDate& prefill)
{
    QDialog dialog(this);
    dialog.setWindowTitle(existing ? QStringLiteral("Edit Event") : QStringLiteral("New Event"));
    auto* form = new QFormLayout(&dialog);

    auto* titleEdit = new QLineEdit(existing ? existing->title : QString(), &dialog);
    auto* allDayCheck = new QCheckBox(QStringLiteral("All day"), &dialog);
    auto* startDate = new QDateEdit(&dialog);
    auto* startTime = new QTimeEdit(&dialog);
    auto* endTime = new QTimeEdit(&dialog);
    const QString editStyle = QStringLiteral(
        "QLineEdit, QDateEdit, QTimeEdit { background: #1C1C1C; border: 1px solid #2A2A2A;"
        " color: #D6D6D6; border-radius: 6px; padding: 6px 8px; }");

    if (existing) {
        startDate->setDate(existing->start.date());
        startTime->setTime(existing->start.time());
        endTime->setTime(existing->end.time());
        allDayCheck->setChecked(existing->allDay);
    } else {
        const QDate day = prefill.isValid() ? prefill : m_selected;
        startDate->setDate(day);
        startTime->setTime(QTime(9, 0));
        endTime->setTime(QTime(10, 0));
    }
    startDate->setCalendarPopup(true);
    startDate->setDisplayFormat(QStringLiteral("MMM d, yyyy"));
    for (QWidget* w : {static_cast<QWidget*>(titleEdit), static_cast<QWidget*>(startDate),
                       static_cast<QWidget*>(startTime), static_cast<QWidget*>(endTime)}) {
        w->setStyleSheet(editStyle);
    }
    titleEdit->setPlaceholderText(QStringLiteral("Event title"));
    form->addRow(QStringLiteral("Title"), titleEdit);
    form->addRow(QStringLiteral("Date"), startDate);
    form->addRow(QStringLiteral("Starts"), startTime);
    form->addRow(QStringLiteral("Ends"), endTime);
    form->addRow(QString(), allDayCheck);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(existing ? QStringLiteral("Save")
                                                            : QStringLiteral("Add"));
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted) return;
    const QString title = titleEdit->text().trimmed();
    if (title.isEmpty()) return;

    models::CalendarEvent event;
    if (existing) event = *existing;
    event.title = title;
    event.allDay = allDayCheck->isChecked();
    event.start = QDateTime(startDate->date(), startTime->time());
    event.end = QDateTime(startDate->date(), endTime->time());
    if (event.end <= event.start) event.end = event.start.addSecs(3600);

    if (existing) {
        context()->events()->update(event);
    } else {
        context()->events()->insert(event);
    }
    onActivated();
}

void CalendarModule::onActivated()
{
    rebuildGrid();
    refreshSidePanel();
}

void CalendarModule::refreshView()
{
    rebuildGrid();
    refreshSidePanel();
}

void CalendarModule::handleCommand(const QString& command)
{
    if (command == QLatin1String("today")) {
        m_cursor = QDate::currentDate();
        m_selected = QDate::currentDate();
        onActivated();
    }
}

void CalendarModule::openDate(const QDate& date)
{
    m_cursor = date;
    m_selected = date;
    onActivated();
}

} // namespace arete::ui
