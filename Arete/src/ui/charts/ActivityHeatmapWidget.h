#pragma once

#include <QWidget>
#include <QMap>
#include <QDate>

// GitHub-style activity heatmap: one column per week, one cell per day,
// colour intensity driven by focus minutes. Painted with QPainter only.
class ActivityHeatmapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ActivityHeatmapWidget(QWidget* parent = nullptr);

    // Intensity map: date -> focus minutes (or any metric).
    void setIntensity(const QMap<QDate, int>& intensity, int weeks = 12);
    void setMetricName(const QString& name);

    [[nodiscard]] QSize minimumSizeHint() const override { return QSize(320, 110); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QMap<QDate, int> m_intensity;
    int m_weeks = 12;
    QString m_metricName = QStringLiteral("focus");
};
