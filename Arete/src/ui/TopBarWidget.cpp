#include "ui/TopBarWidget.h"
#include "app/AppContext.h"
#include "services/WeatherService.h"
#include "services/TimerService.h"
#include "services/MusicService.h"
#include "core/Icons.h"
#include "arete/notifications/NotificationCenter.h"

#include <QHBoxLayout>
#include <QDateTime>
#include <QMouseEvent>

namespace arete::ui {

TopBarWidget::TopBarWidget(app::AppContext* context, QWidget* parent)
    : QWidget(parent), m_context(context)
{
    setFixedHeight(52);
    setStyleSheet(QStringLiteral(
        "QWidget { background-color: #181818; border-bottom: 1px solid #353535; }"
        "QLabel { background: transparent; color: #A0A0A0; }"
        "QLabel#clock { color: #D6D6D6; font-size: 15px; letter-spacing: 1px; }"
        "QLabel#date { color: #7A7A7A; font-size: 12px; }"
        "QLabel#verse { color: #7A7A7A; font-size: 12px; }"
        "QLabel#chip { color: #A0A0A0; background-color: #242424; border: 1px solid #353535;"
        " border-radius: 10px; padding: 3px 10px; font-size: 12px; }"
        "QLabel#profile { color: #C4C4C4; background-color: #2A2A2A; border-radius: 13px;"
        " padding: 4px 10px; font-size: 12px; }"
        "QLabel#bell { color: #A0A0A0; font-size: 15px; padding: 4px 8px; }"
        "QLabel#bell:hover { color: #D6D6D6; }"
    ));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(16);

    m_clock = new QLabel(this);
    m_clock->setObjectName(QStringLiteral("clock"));

    m_date = new QLabel(this);
    m_date->setObjectName(QStringLiteral("date"));

    m_weather = new QLabel(this);
    m_weather->setObjectName(QStringLiteral("chip"));
    m_weather->setToolTip(QStringLiteral("Weather"));

    m_timerChip = new QLabel(this);
    m_timerChip->setObjectName(QStringLiteral("chip"));
    m_timerChip->setToolTip(QStringLiteral("Focus timer"));

    m_musicChip = new QLabel(this);
    m_musicChip->setObjectName(QStringLiteral("chip"));
    m_musicChip->setToolTip(QStringLiteral("Music"));

    m_notifications = new QLabel(QStringLiteral("Notifications"), this);
    m_notifications->setObjectName(QStringLiteral("chip"));
    m_notifications->setToolTip(QStringLiteral("Notifications"));
    m_notifications->setCursor(Qt::PointingHandCursor);

    m_profile = new QLabel(QStringLiteral("ARETE"), this);
    m_profile->setObjectName(QStringLiteral("profile"));
    m_profile->setToolTip(QStringLiteral("Arete \u00B7 personal operating system"));

    layout->addWidget(m_clock);
    layout->addWidget(m_date);
    layout->addStretch(1);
    layout->addWidget(m_weather);
    layout->addWidget(m_timerChip);
    layout->addWidget(m_musicChip);
    layout->addWidget(m_notifications);
    layout->addWidget(m_profile);

    // Notifications bell opens the panel.
    connect(m_notifications, &QLabel::linkActivated, this, [this] {});
    m_notifications->installEventFilter(this);

    connect(context->weather(), &services::WeatherService::updated, this, &TopBarWidget::onWeather);
    connect(context->timer(), &services::TimerService::stateChanged, this, &TopBarWidget::refreshTimerChip);
    connect(context->timer(), &services::TimerService::tick, this, &TopBarWidget::refreshTimerChip);
    connect(context->music(), &services::MusicService::playbackChanged, this, &TopBarWidget::refreshMusicChip);

    m_clockTimer.setInterval(1000);
    connect(&m_clockTimer, &QTimer::timeout, this, &TopBarWidget::updateClock);
    m_clockTimer.start();
    updateClock();

    refresh();
}

void TopBarWidget::refresh()
{
    onWeather(m_context->weather()->snapshot());
    refreshTimerChip();
    refreshMusicChip();
}

void TopBarWidget::updateClock()
{
    const QDateTime now = QDateTime::currentDateTime();
    m_clock->setText(now.toString(QStringLiteral("HH:mm")));
    m_date->setText(now.toString(QStringLiteral("dddd, MMM d yyyy")));
}

void TopBarWidget::onWeather(const services::WeatherSnapshot& snapshot)
{
    if (!snapshot.valid) {
        m_weather->setVisible(false);
        return;
    }
    m_weather->setVisible(true);
    m_weather->setText(QStringLiteral("%1\u00B0").arg(qRound(snapshot.temperatureC)));
}

void TopBarWidget::refreshTimerChip()
{
    if (!m_context->timer()->isRunning()) {
        m_timerChip->setVisible(false);
        return;
    }
    m_timerChip->setVisible(true);
    const int remaining = m_context->timer()->remainingSeconds();
    const QString title = m_context->timer()->attachedTaskTitle();
    const QString label = title.isEmpty()
        ? QStringLiteral("focus %1:%2").arg(remaining / 60, 2, 10, QLatin1Char('0'))
                                       .arg(remaining % 60, 2, 10, QLatin1Char('0'))
        : QStringLiteral("%1 \u00B7 %2:%3").arg(title.left(18))
                                           .arg(remaining / 60, 2, 10, QLatin1Char('0'))
                                           .arg(remaining % 60, 2, 10, QLatin1Char('0'));
    m_timerChip->setText(label);
}

void TopBarWidget::refreshMusicChip()
{
    if (!m_context->music()->isPlaying()) {
        m_musicChip->setVisible(false);
        return;
    }
    m_musicChip->setVisible(true);
    m_musicChip->setText(m_context->music()->currentTitle().left(20));
}

bool TopBarWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_notifications) {
        if (event->type() == QEvent::MouseButtonRelease) {
            emit notificationsClicked();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace arete::ui
