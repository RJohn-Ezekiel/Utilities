#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QEvent>

namespace arete {
namespace app { class AppContext; }
namespace services { struct WeatherSnapshot; }

namespace ui {

// Top bar: time, date, weather, timer and music chips, notifications and
// profile. Stays visible above the tab strip. The daily scripture lives on
// the Home module, not here.
class TopBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TopBarWidget(app::AppContext* context, QWidget* parent = nullptr);

    void refresh();

signals:
    void notificationsClicked();
    void paletteRequested();

private:
    void updateClock();
    void onWeather(const services::WeatherSnapshot& snapshot);
    void refreshTimerChip();
    void refreshMusicChip();
    bool eventFilter(QObject* watched, QEvent* event) override;

    app::AppContext* m_context;
    QLabel* m_clock;
    QLabel* m_date;
    QLabel* m_weather;
    QLabel* m_timerChip;
    QLabel* m_musicChip;
    QLabel* m_notifications;
    QLabel* m_profile;
    QTimer m_clockTimer;
};

} // namespace ui
} // namespace arete