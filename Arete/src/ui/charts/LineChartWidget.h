#pragma once

#include <QWidget>
#include <QVector>
#include <QColor>

// Minimal multi-series line chart painted with QPainter. Used for the
// wellbeing mood/energy trends; each series is one QVector<double> (1..5).
class LineChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LineChartWidget(QWidget* parent = nullptr);

    struct Series
    {
        QString name;
        QColor color;
        QVector<double> values;
    };

    void setSeries(const QVector<Series>& series, const QStringList& xLabels = {});
    void setRange(double min, double max);
    void setEmptyText(const QString& text);

    [[nodiscard]] QSize minimumSizeHint() const override { return QSize(220, 120); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<Series> m_series;
    QStringList m_xLabels;
    double m_min = 1.0;
    double m_max = 5.0;
    QString m_emptyText;
};
