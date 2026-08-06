#include "ui/modules/HabitsModule.h"
#include "app/AppContext.h"
#include "data/HabitRepository.h"
#include "models/Habit.h"
#include "ui/common/SectionLabel.h"
#include "ui/common/EmptyStateWidget.h"
#include "ui/common/TaskToggle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QLineEdit>
#include <QDate>

namespace arete::ui {

HabitsModule::HabitsModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(48, 32, 48, 32);
    outer->setSpacing(16);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("Habits"), this);
    title->setStyleSheet(QStringLiteral("font-size: 22px; color: #E2E2E2; letter-spacing: 1px;"));
    header->addWidget(title);
    header->addStretch(1);

    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 12px;"));
    header->addWidget(m_summaryLabel);

    m_addButton = new QPushButton(QStringLiteral("+  NEW HABIT"), this);
    m_addButton->setCursor(Qt::PointingHandCursor);
    m_addButton->setStyleSheet(QStringLiteral(
        "QPushButton { font-size: 12px; letter-spacing: 1px; color: #D6D6D6;"
        " background-color: #242424; border: 1px solid #353535; border-radius: 8px;"
        " padding: 6px 14px; }"
        "QPushButton:hover { background-color: #2A2A2A; }"));
    m_editButton = new QPushButton(QStringLiteral("Edit"), this);
    m_deleteButton = new QPushButton(QStringLiteral("Delete"), this);
    m_editButton->setStyleSheet(m_addButton->styleSheet());
    m_deleteButton->setStyleSheet(m_addButton->styleSheet());
    m_editButton->setCursor(Qt::PointingHandCursor);
    m_deleteButton->setCursor(Qt::PointingHandCursor);
    header->addWidget(m_editButton);
    header->addWidget(m_deleteButton);
    header->addWidget(m_addButton);
    outer->addLayout(header);

    m_list = new QListWidget(this);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setStyleSheet(QStringLiteral(
        "QListWidget { background: transparent; outline: none; }"
        "QListWidget::item { padding: 8px 4px; border-bottom: 1px solid #262626; }"
        "QListWidget::item:selected { background: #1E1E1E; }"));
    outer->addWidget(m_list, 1);

    connect(m_addButton, &QPushButton::clicked, this, &HabitsModule::onAdd);
    connect(m_editButton, &QPushButton::clicked, this, &HabitsModule::onEdit);
    connect(m_deleteButton, &QPushButton::clicked, this, &HabitsModule::onDelete);
    connect(m_list, &QListWidget::itemActivated, this, &HabitsModule::onItemActivated);
    connect(m_list, &QListWidget::itemSelectionChanged, this, [this] {
        const bool has = m_list->currentRow() >= 0;
        m_editButton->setEnabled(has);
        m_deleteButton->setEnabled(has);
    });
    m_editButton->setEnabled(false);
    m_deleteButton->setEnabled(false);
}

void HabitsModule::onActivated()
{
    rebuild();
}

void HabitsModule::refreshView()
{
    rebuild();
}

void HabitsModule::handleCommand(const QString& command)
{
    if (command == QLatin1String("habits:new")) {
        onAdd();
    } else if (command == QLatin1String("habits:today")) {
        onItemActivated(nullptr);
    }
}

void HabitsModule::rebuild()
{
    m_list->clear();
    const QVector<models::Habit> habits = context()->habits()->all();

    int done = 0;
    for (const models::Habit& h : habits) {
        if (h.doneToday) ++done;
    }
    m_summaryLabel->setText(habits.isEmpty()
                                ? QString()
                                : QStringLiteral("%1 / %2 done today").arg(done).arg(habits.size()));

    if (habits.isEmpty()) {
        m_list->addItem(QString());
        m_list->setItemWidget(
            m_list->item(0),
            new common::EmptyStateWidget(QStringLiteral(""),
                                         QStringLiteral("No habits yet. Add one — small daily marks become long streaks."),
                                         this));
        return;
    }

    const QDate today = QDate::currentDate();
    for (const models::Habit& h : habits) {
        auto* item = new QListWidgetItem;
        item->setData(Qt::UserRole, h.id);
        item->setData(Qt::UserRole + 1, h.name);
        item->setSizeHint(QSize(0, 56));
        m_list->addItem(item);

        auto* widget = new QWidget(this);
        auto* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(4, 0, 4, 0);
        layout->setSpacing(12);

        // Completion toggle for today.
        auto* check = new common::TaskToggle(widget);
        check->setChecked(h.doneToday);
        check->setToolTip(QStringLiteral("Mark done for today"));
        connect(check, &common::TaskToggle::toggled, this, [this, h](bool) {
            context()->habits()->toggle(h.id, QDate::currentDate());
            rebuild();
        });
        layout->addWidget(check);

        auto* text = new QLabel(h.name, widget);
        text->setStyleSheet(QStringLiteral("font-size: 15px; color: #D6D6D6;"));
        layout->addWidget(text, 1);

        // Seven-day strip (recent history, right to left: oldest..today).
        const QDate weekStart = today.addDays(-6);
        auto* strip = new QHBoxLayout;
        strip->setSpacing(3);
        for (int i = 0; i < 7; ++i) {
            const QDate d = weekStart.addDays(i);
            const bool done = context()->habits()->isDone(h.id, d);
            auto* dot = new QLabel(done ? QStringLiteral("*") : QStringLiteral("."), widget);
            dot->setStyleSheet(QStringLiteral("color: %1; font-size: 9px;")
                                   .arg(done ? QStringLiteral("#9A9A9A") : QStringLiteral("#3A3A3A")));
            dot->setToolTip(d.toString(QStringLiteral("ddd MMM d")));
            strip->addWidget(dot);
        }
        layout->addLayout(strip);

        auto* meta = new QLabel(QStringLiteral("%1/7  \u00B7  %2 day streak")
                                    .arg(h.doneThisWeek).arg(h.currentStreak),
                                widget);
        meta->setStyleSheet(QStringLiteral("color: #6E6E6E; font-size: 12px;"));
        layout->addWidget(meta);

        m_list->setItemWidget(item, widget);
    }
}

void HabitsModule::onItemActivated(QListWidgetItem*)
{
    // Clicking the row edits the habit.
    onEdit();
}

void HabitsModule::showEditor(const models::Habit* habit)
{
    QDialog dialog(this);
    dialog.setWindowTitle(habit ? QStringLiteral("Edit Habit") : QStringLiteral("New Habit"));
    auto* form = new QFormLayout(&dialog);
    auto* nameEdit = new QLineEdit(habit ? habit->name : QString(), &dialog);
    auto* targetSpin = new QSpinBox(&dialog);
    targetSpin->setRange(1, 7);
    targetSpin->setValue(habit ? habit->targetPerWeek : 5);
    nameEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { background: #1C1C1C; border: 1px solid #2A2A2A; color: #D6D6D6;"
        " border-radius: 6px; padding: 6px 8px; }"));
    targetSpin->setStyleSheet(nameEdit->styleSheet());
    form->addRow(QStringLiteral("Name"), nameEdit);
    form->addRow(QStringLiteral("Days per week"), targetSpin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Save"));
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted) return;
    const QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) return;
    if (habit) {
        context()->habits()->update(habit->id, name, targetSpin->value());
    } else {
        context()->habits()->insert(name, targetSpin->value());
    }
    rebuild();
}

void HabitsModule::onAdd()
{
    showEditor(nullptr);
}

void HabitsModule::onEdit()
{
    const int row = m_list->currentRow();
    if (row < 0) return;
    const QListWidgetItem* item = m_list->item(row);
    const QVector<models::Habit> habits = context()->habits()->all();
    const qint64 id = item->data(Qt::UserRole).toLongLong();
    for (const models::Habit& h : habits) {
        if (h.id == id) {
            showEditor(&h);
            return;
        }
    }
}

void HabitsModule::onDelete()
{
    const int row = m_list->currentRow();
    if (row < 0) return;
    const QListWidgetItem* item = m_list->item(row);
    const QString name = item->data(Qt::UserRole + 1).toString();
    const auto answer = QMessageBox::question(
        this, QStringLiteral("Delete habit"),
        QStringLiteral("Remove \u201C%1\u201D and its history?").arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    context()->habits()->remove(item->data(Qt::UserRole).toLongLong());
    rebuild();
}

} // namespace arete::ui