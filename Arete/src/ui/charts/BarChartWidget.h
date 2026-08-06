#pragma once

#include <QWidget>
#include <QVector>
#include <QString>

// Minimal monochrome bar chart painted with QPainter. No external chart
// library — the greyscale palette keeps it consistent with the shell.
class BarChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BarChartWidget(QWidget* parent = nullptr);

    // Values are drawn as vertical bars; labels sit under each bar.
    void setData(const QVector<int>& values, const QStringList& labels = {});
    void setBarColor(const QColor& color);
    void setEmptyText(const QString& text);

    [[nodiscard]] QSize minimumSizeHint() const override { return QSize(220, 140); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<int> m_values;
    QStringList m_labels;
    QColor m_barColor { 0x7A, 0x7A, 0x7A };
    QString m_emptyText;
};
