#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVariant>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <functional>
#include <memory>

namespace arete::stats {

// ============================================================================
// Metric types
// ============================================================================

enum class MetricType {
    Counter,      // monotonically increasing counter
    Gauge,        // current value
    Timer,        // accumulated duration (ms)
    Histogram,    // distribution
    Ratio,        // value / total
    Latest        // last value
};

QString metricTypeName(MetricType type);

// ============================================================================
// Metric
// ============================================================================

class Metric
{
public:
    Metric() = default;
    Metric(QString name, MetricType type = MetricType::Counter);

    void increment(double delta = 1.0);
    void add(double value) { increment(value); }
    void set(double value);
    void addDuration(qint64 milliseconds);

    [[nodiscard]] QString name() const;
    [[nodiscard]] MetricType type() const;
    [[nodiscard]] double value() const;
    [[nodiscard]] double total() const;
    [[nodiscard]] int sampleCount() const;
    [[nodiscard]] double average() const;
    [[nodiscard]] double min() const;
    [[nodiscard]] double max() const;

    void reset();

    [[nodiscard]] QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);

private:
    QString m_name;
    MetricType m_type = MetricType::Counter;
    double m_value = 0.0;
    double m_total = 0.0;
    double m_min = 0.0;
    double m_max = 0.0;
    int m_count = 0;
};

// ============================================================================
// Statistics collection
// ============================================================================

class StatisticsCollection : public QObject
{
    Q_OBJECT

public:
    explicit StatisticsCollection(const QString& id = QString(), QObject* parent = nullptr);
    ~StatisticsCollection() override;

    // Metrics
    Metric& metric(const QString& name);
    void increment(const QString& name, double delta = 1.0);
    void setValue(const QString& name, double value);
    void addDuration(const QString& name, qint64 milliseconds);
    [[nodiscard]] double value(const QString& name) const;
    [[nodiscard]] bool hasMetric(const QString& name) const;
    void removeMetric(const QString& name);
    void resetAll();
    [[nodiscard]] QStringList metricNames() const;

    // Events (append-only list with timestamps)
    void recordEvent(const QString& name, const QVariantMap& data = {});
    [[nodiscard]] QVector<QVariantMap> events(const QString& name = QString()) const;

    // Daily activity tracking
    void recordActivity(qint64 durationMs, const QDateTime& when = QDateTime::currentDateTime());
    [[nodiscard]] qint64 activityOn(const QDate& date) const;
    [[nodiscard]] QVector<qint64> activityBetween(const QDate& start, const QDate& end) const;
    [[nodiscard]] int activeDayCount(const QDate& start, const QDate& end) const;
    [[nodiscard]] int currentStreakDays() const;

    // Persistence
    [[nodiscard]] QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);
    [[nodiscard]] bool saveToFile(const QString& filePath) const;
    [[nodiscard]] bool loadFromFile(const QString& filePath);

signals:
    void metricUpdated(const QString& name, double value);
    void eventRecorded(const QString& name, const QVariantMap& data);
    void collectionReset();

private:
    class Private;
    std::unique_ptr<Private> d;
};

// ============================================================================
// Report generation
// ============================================================================

struct ReportEntry
{
    QString label;
    QString value;
    QString detail;
};

struct Report
{
    QString title;
    QString period;                    // e.g. "Today", "This week", "All time"
    QVector<ReportEntry> entries;

    [[nodiscard]] QString toText() const;
    [[nodiscard]] QString toMarkdown() const;
};

// Formatting helpers
[[nodiscard]] QString formatDuration(qint64 ms);
[[nodiscard]] QString formatNumber(double value, int decimals = 0);
[[nodiscard]] QString formatPercentage(double ratio, int decimals = 1);

} // namespace arete::stats