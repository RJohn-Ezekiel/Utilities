#include "ReminderScheduler.h"

namespace chronos {

ReminderScheduler::ReminderScheduler(QObject* parent)
    : QObject(parent)
{
}

void ReminderScheduler::configure(const Settings& settings)
{
    m_waterInterval = settings.waterReminderInterval;
    m_standInterval = settings.standReminderInterval;
    m_stretchInterval = settings.stretchReminderInterval;
    m_eyeInterval = settings.eyeReminderInterval;

    // Re-create timers with new intervals
    setupTimer(m_waterTimer, m_waterInterval, HealthReminderType::Water);
    setupTimer(m_standTimer, m_standInterval, HealthReminderType::Stand);
    setupTimer(m_stretchTimer, m_stretchInterval, HealthReminderType::Stretch);
    setupTimer(m_eyeTimer, m_eyeInterval, HealthReminderType::Eye);
}

void ReminderScheduler::setupTimer(QTimer*& timer, int intervalSec,
                                   HealthReminderType type)
{
    if (timer) {
        timer->stop();
        timer->deleteLater();
    }

    timer = new QTimer(this);
    timer->setInterval(intervalSec * 1000);
    connect(timer, &QTimer::timeout, this, [this, type]() {
        emit reminderTriggered(type);
    });
}

void ReminderScheduler::startAll()
{
    for (auto* t : {m_waterTimer, m_standTimer, m_stretchTimer, m_eyeTimer}) {
        if (t) t->start();
    }
}

void ReminderScheduler::stopAll()
{
    for (auto* t : {m_waterTimer, m_standTimer, m_stretchTimer, m_eyeTimer}) {
        if (t) t->stop();
    }
}

} // namespace chronos
