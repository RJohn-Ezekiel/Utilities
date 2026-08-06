#include "ui/modules/WellbeingModule.h"
#include "app/AppContext.h"
#include "data/CheckinRepository.h"
#include "data/StatsRepository.h"
#include "models/Checkin.h"
#include "ui/common/SectionLabel.h"
#include "ui/charts/LineChartWidget.h"
#include "ui/charts/ActivityHeatmapWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollArea>
#include <QDate>

namespace arete::ui {

namespace {
QString moodLabel(int v)
{
    switch (v) {
    case 1: return QStringLiteral("Low");
    case 2: return QStringLiteral("Meh");
    case 3: return QStringLiteral("Steady");
    case 4: return QStringLiteral("Good");
    case 5: return QStringLiteral("Great");
    default: return QString();
    }
}
} // namespace

WellbeingModule::WellbeingModule(app::AppContext* context, QWidget* parent)
    : core::Module(context, parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(48, 32, 48, 32);
    outer->setSpacing(16);

    auto* title = new QLabel(QStringLiteral("Wellbeing"), this);
    title->setStyleSheet(QStringLiteral("font-size: 22px; color: #E2E2E2; letter-spacing: 1px;"));
    outer->addWidget(title);

    // ── Check-in card ──
    auto* card = new QWidget(this);
    card->setStyleSheet(QStringLiteral("QWidget { background-color: #1C1C1C;"
                                       " border: 1px solid #2A2A2A; border-radius: 12px; }"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 14, 18, 14);
    cardLayout->setSpacing(10);

    auto* todayLabel = new QLabel(QStringLiteral("TODAY  \u00B7  %1")
                                      .arg(QDate::currentDate().toString(QStringLiteral("dddd, MMM d"))),
                                  card);
    todayLabel->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 12px; letter-spacing: 1px;"));
    cardLayout->addWidget(todayLabel);

    auto* moodRow = new QHBoxLayout;
    moodRow->addWidget(new QLabel(QStringLiteral("Mood"), card));
    m_moodSlider = new QSlider(Qt::Horizontal, card);
    m_moodSlider->setRange(1, 5);
    m_moodSlider->setValue(3);
    moodRow->addWidget(m_moodSlider, 1);
    m_moodValue = new QLabel(card);
    moodRow->addWidget(m_moodValue);
    cardLayout->addLayout(moodRow);

    auto* energyRow = new QHBoxLayout;
    energyRow->addWidget(new QLabel(QStringLiteral("Energy"), card));
    m_energySlider = new QSlider(Qt::Horizontal, card);
    m_energySlider->setRange(1, 5);
    m_energySlider->setValue(3);
    energyRow->addWidget(m_energySlider, 1);
    m_energyValue = new QLabel(card);
    energyRow->addWidget(m_energyValue);
    cardLayout->addLayout(energyRow);

    m_noteEdit = new QPlainTextEdit(card);
    m_noteEdit->setPlaceholderText(QStringLiteral("A line about today (optional)"));
    m_noteEdit->setFixedHeight(64);
    m_noteEdit->setStyleSheet(QStringLiteral(
        "QPlainTextEdit { background: #141414; border: 1px solid #2A2A2A; color: #D6D6D6;"
        " border-radius: 8px; padding: 8px; }"));
    cardLayout->addWidget(m_noteEdit);

    auto* saveRow = new QHBoxLayout;
    m_saveButton = new QPushButton(QStringLiteral("SAVE CHECK-IN"), card);
    m_saveButton->setCursor(Qt::PointingHandCursor);
    m_saveButton->setStyleSheet(QStringLiteral(
        "QPushButton { font-size: 12px; letter-spacing: 1px; color: #D6D6D6;"
        " background-color: #242424; border: 1px solid #353535; border-radius: 8px;"
        " padding: 8px 16px; }"
        "QPushButton:hover { background-color: #2A2A2A; }"));
    m_savedLabel = new QLabel(card);
    m_savedLabel->setStyleSheet(QStringLiteral("color: #5A5A5A; font-size: 12px;"));
    saveRow->addWidget(m_saveButton);
    saveRow->addWidget(m_savedLabel);
    saveRow->addStretch(1);
    cardLayout->addLayout(saveRow);

    outer->addWidget(card);

    auto* body = new QHBoxLayout;
    body->setSpacing(16);

    // ── Trend chart ──
    auto* trendBox = new QVBoxLayout;
    trendBox->addWidget(new common::SectionLabel(QStringLiteral("14-DAY TREND"), this));
    m_trend = new LineChartWidget(this);
    m_trend->setMinimumHeight(150);
    trendBox->addWidget(m_trend, 1);
    body->addLayout(trendBox, 3);

    // ── History list ──
    auto* historyBox = new QVBoxLayout;
    historyBox->addWidget(new common::SectionLabel(QStringLiteral("RECENT"), this));
    m_history = new QListWidget(this);
    m_history->setFrameShape(QFrame::NoFrame);
    m_history->setFocusPolicy(Qt::NoFocus);
    m_history->setStyleSheet(QStringLiteral(
        "QListWidget { background: transparent; outline: none; }"
        "QListWidget::item { padding: 4px 0; border-bottom: 1px solid #262626; }"));
    historyBox->addWidget(m_history, 1);
    body->addLayout(historyBox, 2);

    outer->addLayout(body, 1);

    // ── Activity heatmap ──
    auto* heatBox = new QVBoxLayout;
    heatBox->addWidget(new common::SectionLabel(QStringLiteral("FOCUS ACTIVITY  \u00B7  12 WEEKS"), this));
    m_heatmap = new ActivityHeatmapWidget(this);
    m_heatmap->setMinimumHeight(110);
    heatBox->addWidget(m_heatmap);
    outer->addLayout(heatBox);

    connect(m_moodSlider, &QSlider::valueChanged, this, [this](int v) {
        m_moodValue->setText(moodLabel(v));
    });
    connect(m_energySlider, &QSlider::valueChanged, this, [this](int v) {
        m_energyValue->setText(moodLabel(v));
    });
    connect(m_saveButton, &QPushButton::clicked, this, &WellbeingModule::onSave);
    m_moodValue->setText(moodLabel(3));
    m_energyValue->setText(moodLabel(3));
}

void WellbeingModule::onActivated()
{
    refreshView();
}

void WellbeingModule::refreshView()
{
    const models::Checkin entry = context()->checkins()->entryFor(QDate::currentDate());
    if (entry.id >= 0) {
        m_moodSlider->setValue(entry.mood);
        m_energySlider->setValue(entry.energy);
        m_noteEdit->setPlainText(entry.note);
        m_savedLabel->setText(QStringLiteral("Saved at %1")
                                  .arg(QDate::currentDate().toString(QStringLiteral("MMM d"))));
    } else {
        m_savedLabel->clear();
    }
    refreshHistory();
    refreshCharts();
}

void WellbeingModule::onSave()
{
    context()->checkins()->upsert(QDate::currentDate(), m_moodSlider->value(),
                                  m_energySlider->value(), m_noteEdit->toPlainText());
    m_savedLabel->setText(QStringLiteral("Saved"));
    refreshHistory();
    refreshCharts();
}

void WellbeingModule::refreshHistory()
{
    m_history->clear();
    const QDate today = QDate::currentDate();
    const QVector<models::Checkin> entries = context()->checkins()->range(today.addDays(-13), today);
    for (const models::Checkin& c : entries) {
        auto* item = new QListWidgetItem(m_history);
        item->setText(QStringLiteral("%1    mood %2/5   energy %3/5")
                          .arg(c.date.toString(QStringLiteral("ddd d MMM")))
                          .arg(c.mood)
                          .arg(c.energy));
        if (!c.note.trimmed().isEmpty()) {
            item->setToolTip(c.note);
        }
        m_history->addItem(item);
    }
    if (entries.isEmpty()) {
        auto* item = new QListWidgetItem(m_history);
        item->setText(QStringLiteral("No check-ins yet."));
        m_history->addItem(item);
    }
}

void WellbeingModule::refreshCharts()
{
    const QDate today = QDate::currentDate();
    const QVector<models::Checkin> entries = context()->checkins()->range(today.addDays(-13), today);

    LineChartWidget::Series mood;
    mood.name = QStringLiteral("mood");
    mood.color = QColor(0xC8, 0xC8, 0xC8);
    LineChartWidget::Series energy;
    energy.name = QStringLiteral("energy");
    energy.color = QColor(0x7A, 0x7A, 0x7A);

    QMap<QDate, models::Checkin> byDate;
    for (const models::Checkin& c : entries) byDate.insert(c.date, c);

    QStringList labels;
    for (QDate d = today.addDays(-13); d <= today; d = d.addDays(1)) {
        const auto it = byDate.find(d);
        if (it != byDate.end()) {
            mood.values.append(it->mood);
            energy.values.append(it->energy);
        } else {
            mood.values.append(0);
            energy.values.append(0);
        }
        labels.append(d.day() == 1 ? d.toString(QStringLiteral("MMM")) : QString());
    }
    m_trend->setSeries({mood, energy}, labels);
    m_trend->setRange(0, 5);

    // Heatmap: focus minutes per day over the last twelve weeks.
    QMap<QDate, int> intensity;
    const QDate start = today.addDays(-(12 * 7 - 1));
    const QVector<int> minutes = context()->stats()->focusMinutesPerDay(start, today);
    for (int i = 0; i < minutes.size(); ++i) {
        intensity.insert(start.addDays(i), minutes.at(i));
    }
    m_heatmap->setIntensity(intensity, 12);
}

} // namespace arete::ui