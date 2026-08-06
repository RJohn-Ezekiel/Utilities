#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QTimer>
#include <memory>

class QNetworkAccessManager;
class QNetworkReply;

namespace arete {
namespace app { class AppContext; }

namespace services {

struct WeatherSnapshot
{
    bool valid = false;
    double temperatureC = 0.0;
    int weatherCode = 0;   // WMO code
    QString location;
    QDateTime fetchedAt;
};

// Fetches temperature + WMO weather code from Open-Meteo (no API key).
// Refreshes every 30-60 minutes as configured; cache persists to JSON.
class WeatherService : public QObject
{
    Q_OBJECT

public:
    explicit WeatherService(app::AppContext* context, QObject* parent = nullptr);
    ~WeatherService() override;

    [[nodiscard]] WeatherSnapshot snapshot() const;
    [[nodiscard]] QString icon() const;   // greyscale glyph
    [[nodiscard]] QString label() const;  // short description

public slots:
    void refresh();

signals:
    void updated(const WeatherSnapshot& snapshot);

private:
    void handleReply(QNetworkReply* reply);
    void loadCache();
    void saveCache() const;
    void ensureLocation();

    app::AppContext* m_context;
    std::unique_ptr<QNetworkAccessManager> m_network;
    WeatherSnapshot m_snapshot;
    QTimer m_refreshTimer;
};

} // namespace services
} // namespace arete
