#include "arete/statistics/Statistics.h"
#include "arete/json/JsonUtils.h"
#include <QJsonDocument>
#include <QFile>
#include <QDate>
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace arete::stats {

QString metricTypeName(MetricType type)
{
    switch (type) {
        case MetricType::Counter: return QStringLiteral("counter");
        case MetricType::Gauge: return QStringLiteral("gauge");
        case MetricType::Timer: return QStringLiteral("timer");
        case MetricType::Histogram: return QStringLiteral("histogram");
        case MetricType::Ratio: return QStringLiteral("ratio");
        case MetricType::Latest: return QStringLiteral("latest");
    }
    return QStringLiteral("unknown");
}

// ============================================================================
// Metric
// ============================================================================

Metric::Metric(QString name, MetricType type)
    : m_name(std::move(name)), m_type(type)
{
}

void Metric::increment(double delta)
{
    m_value += delta;
    m_total += delta;
    ++m_count;
    if (m_count == 1) {
        m_min = m_max = delta;
    } else {
        m_min = std::min(m_min, delta);
        m_max = std::max(m_max, delta);
    }
}

void Metric::set(double value)
{
    m_value = value;
    m_total += value;
    ++m_count;
    if (m_count == 1) {
        m_min = m_max = value;
    } else {
        m_min = std::min(m_min, value);
        m_max = std::max(m_max, value);
    }
}

void Metric::addDuration(qint64 milliseconds)
{
    if (m_type == MetricType::Timer) {
        set(static_cast<double>(milliseconds));
        m_total += static_cast<double>(milliseconds) / 1000.0; // seconds
    } else {
        increment(static_cast<double>(milliseconds));
    }
}

QString Metric::name() const { return m_name; }
MetricType Metric::type() const { return m_type; }
double Metric::value() const { return m_value; }
double Metric::total() const { return m_total; }
int Metric::sampleCount() const { return m_count; }

double Metric::average() const
{
    return m_count > 0 ? m_total / m_count : 0.0;
}

double Metric::min() const { return m_min; }
double Metric::max() const { return m_max; }

void Metric::reset()
{
    m_value = 0.0;
    m_total = 0.0;
    m_min = 0.0;
    m_max = 0.0;
    m_count = 0;
}

QJsonObject Metric::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("name")] = m_name;
    obj[QStringLiteral("type")] = metricTypeName(m_type);
    obj[QStringLiteral("value")] = m_value;
    obj[QStringLiteral("total")] = m_total;
    obj[QStringLiteral("min")] = m_min;
    obj[QStringLiteral("max")] = m_max;
    obj[QStringLiteral("count")] = m_count;
    return obj;
}

void Metric::fromJson(const QJsonObject& obj)
{
    m_name = obj.value(QStringLiteral("name")).toString(m_name);
    const QString typeStr = obj.value(QStringLiteral("type")).toString();
    for (int i = 0; i <= static_cast<int>(MetricType::Latest); ++i) {
        if (metricTypeName(static_cast<MetricType>(i)) == typeStr) {
            m_type = static_cast<MetricType>(i);
            break;
        }
    }
    m_value = obj.value(QStringLiteral("value")).toDouble(m_value);
    m_total = obj.value(QStringLiteral("total")).toDouble(m_total);
    m_min = obj.value(QStringLiteral("min")).toDouble(m_min);
    m_max = obj.value(QStringLiteral("max")).toDouble(m_max);
    m_count = obj.value(QStringLiteral("count")).toInt(m_count);
}

// ============================================================================
// StatisticsCollection
// ============================================================================

class StatisticsCollection::Private
{
public:
    QString id;
    std::unordered_map<QString, Metric> metrics;
    QVector<QVariantMap> events;
    QHash<QDate, qint64> activity;
    QHash<QDate, bool> activeDays; // days with any activity
};


StatisticsCollection::~StatisticsCollection() = default;

StatisticsCollection::StatisticsCollection(const QString& id, QObject* parent)
    : QObject(parent), d(std::make_unique<Private>())
{
    d->id = id;
}

Metric& StatisticsCollection::metric(const QString& name)
{
    auto it = d->metrics.find(name);
    if (it == d->metrics.end()) {
        it = d->metrics.emplace(name, Metric(name, MetricType::Counter)).first;
    }
    return it->second;
}

void StatisticsCollection::increment(const QString& name, double delta)
{
    metric(name).increment(delta);
    emit metricUpdated(name, metric(name).value());
}

void StatisticsCollection::setValue(const QString& name, double value)
{
    Metric& m = metric(name);
    if (m.type() == MetricType::Gauge || m.type() == MetricType::Latest) {
        m.set(value);
    } else {
        m.increment(value);
    }
    emit metricUpdated(name, m.value());
}

void StatisticsCollection::addDuration(const QString& name, qint64 milliseconds)
{
    metric(name).addDuration(milliseconds);
    emit metricUpdated(name, metric(name).value());
}

double StatisticsCollection::value(const QString& name) const
{
    auto it = d->metrics.find(name);
    return it != d->metrics.end() ? it->second.value() : 0.0;
}

bool StatisticsCollection::hasMetric(const QString& name) const
{
    return d->metrics.find(name) != d->metrics.end();
}

void StatisticsCollection::removeMetric(const QString& name)
{
    d->metrics.erase(name);
}

void StatisticsCollection::resetAll()
{
    for (auto& [name, m] : d->metrics) {
        m.reset();
    }
    d->events.clear();
    d->activity.clear();
    d->activeDays.clear();
    emit collectionReset();
}

QStringList StatisticsCollection::metricNames() const
{
    QStringList names;
    for (const auto& [name, m] : d->metrics) {
        names.append(name);
    }
    return names;
}

void StatisticsCollection::recordEvent(const QString& name, const QVariantMap& data)
{
    QVariantMap event = data;
    event[QStringLiteral("_event")] = name;
    event[QStringLiteral("_time")] = QDateTime::currentDateTime();
    d->events.append(event);
    if (d->events.size() > 10000) {
        d->events.remove(0, d->events.size() - 10000);
    }
    emit eventRecorded(name, data);
}

QVector<QVariantMap> StatisticsCollection::events(const QString& name) const
{
    if (name.isEmpty()) return d->events;
    QVector<QVariantMap> result;
    for (const auto& e : d->events) {
        if (e.value(QStringLiteral("_event")) == name) {
            result.append(e);
        }
    }
    return result;
}

void StatisticsCollection::recordActivity(qint64 durationMs, const QDateTime& when)
{
    const QDate date = when.date();
    d->activity[date] += durationMs;
    d->activeDays[date] = true;
}

qint64 StatisticsCollection::activityOn(const QDate& date) const
{
    auto it = d->activity.find(date);
    return it != d->activity.end() ? it.value() : 0;
}

QVector<qint64> StatisticsCollection::activityBetween(const QDate& start, const QDate& end) const
{
    QVector<qint64> result;
    for (QDate date = start; date <= end; date = date.addDays(1)) {
        result.append(activityOn(date));
    }
    return result;
}

int StatisticsCollection::activeDayCount(const QDate& start, const QDate& end) const
{
    int count = 0;
    for (QDate date = start; date <= end; date = date.addDays(1)) {
        if (activityOn(date) > 0) ++count;
    }
    return count;
}

int StatisticsCollection::currentStreakDays() const
{
    int streak = 0;
    QDate date = QDate::currentDate();
    while (activityOn(date) > 0) {
        ++streak;
        date = date.addDays(-1);
    }
    return streak;
}

QJsonObject StatisticsCollection::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("id")] = d->id;

    QJsonArray metricsArray;
    for (const auto& [name, m] : d->metrics) {
        metricsArray.append(m.toJson());
    }
    obj[QStringLiteral("metrics")] = metricsArray;

    QJsonArray eventsArray;
    for (const auto& e : d->events) {
        QJsonObject eventObj;
        for (auto it = e.begin(); it != e.end(); ++it) {
            eventObj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        eventsArray.append(eventObj);
    }
    obj[QStringLiteral("events")] = eventsArray;

    QJsonObject activityObj;
    const auto dates = d->activity.keys();
    for (const QDate& date : dates) {
        activityObj[date.toString(Qt::ISODate)] = static_cast<double>(d->activity.value(date));
    }
    obj[QStringLiteral("activity")] = activityObj;

    return obj;
}

void StatisticsCollection::fromJson(const QJsonObject& obj)
{
    d->id = obj.value(QStringLiteral("id")).toString(d->id);

    for (const auto& val : obj.value(QStringLiteral("metrics")).toArray()) {
        Metric m;
        m.fromJson(val.toObject());
        d->metrics[m.name()] = m;
    }

    for (const auto& val : obj.value(QStringLiteral("events")).toArray()) {
        QVariantMap event;
        const QJsonObject eventObj = val.toObject();
        for (auto it = eventObj.begin(); it != eventObj.end(); ++it) {
            event[it.key()] = it.value().toVariant();
        }
        d->events.append(event);
    }

    const QJsonObject activityObj = obj.value(QStringLiteral("activity")).toObject();
    for (auto it = activityObj.begin(); it != activityObj.end(); ++it) {
        const QDate date = QDate::fromString(it.key(), Qt::ISODate);
        if (date.isValid()) {
            d->activity[date] = static_cast<qint64>(it.value().toDouble());
            d->activeDays[date] = 1;
        }
    }
}

bool StatisticsCollection::saveToFile(const QString& filePath) const
{
    return json::saveObject(filePath, toJson()).hasError() == false;
}

bool StatisticsCollection::loadFromFile(const QString& filePath)
{
    auto result = json::loadObject(filePath);
    if (result.hasError()) return false;
    fromJson(result.value());
    return true;
}

// ============================================================================
// Report generation
// ============================================================================

QString Report::toText() const
{
    QString result;
    if (!title.isEmpty()) result += title + QStringLiteral("\n");
    if (!period.isEmpty()) result += period + QStringLiteral("\n");
    if (!result.isEmpty()) result += QStringLiteral("\n");

    int labelWidth = 0;
    for (const auto& e : entries) {
        labelWidth = std::max(labelWidth, static_cast<int>(e.label.size()));
    }

    for (const auto& e : entries) {
        result += e.label.leftJustified(labelWidth + 2);
        result += e.value;
        if (!e.detail.isEmpty()) {
            result += QStringLiteral("  (%1)").arg(e.detail);
        }
        result += QLatin1Char('\n');
    }
    return result;
}

QString Report::toMarkdown() const
{
    QString result;
    if (!title.isEmpty()) result += QStringLiteral("# %1\n\n").arg(title);
    if (!period.isEmpty()) result += QStringLiteral("_%1_\n\n").arg(period);

    result += QStringLiteral("| %1 | %2 |\n| --- | --- |\n").arg(QStringLiteral("Metric"), QStringLiteral("Value"));
    for (const auto& e : entries) {
        QString value = e.value;
        if (!e.detail.isEmpty()) {
            value += QStringLiteral(" (%1)").arg(e.detail);
        }
        result += QStringLiteral("| %1 | %2 |\n").arg(e.label, value);
    }
    return result;
}

QString formatDuration(qint64 ms)
{
    if (ms < 0) ms = 0;
    const qint64 seconds = ms / 1000;
    const qint64 minutes = seconds / 60;
    const qint64 hours = minutes / 60;
    const qint64 days = hours / 24;

    if (days > 0) {
        return QStringLiteral("%1d %2h %3m").arg(days).arg(hours % 24).arg(minutes % 60);
    }
    if (hours > 0) {
        return QStringLiteral("%1h %2m").arg(hours).arg(minutes % 60);
    }
    if (minutes > 0) {
        return QStringLiteral("%1m %2s").arg(minutes).arg(seconds % 60);
    }
    return QStringLiteral("%1s").arg(seconds);
}

QString formatNumber(double value, int decimals)
{
    return QString::number(value, 'f', decimals);
}

QString formatPercentage(double ratio, int decimals)
{
    return QStringLiteral("%1%").arg(QString::number(ratio * 100.0, 'f', decimals));
}

} // namespace arete::stats