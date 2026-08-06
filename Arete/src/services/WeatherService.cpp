#include "services/WeatherService.h"
#include "app/AppContext.h"
#include "arete/settings/SettingsManager.h"

#include <QNetworkAccessManager>
#include <QNetworkInformation>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>

namespace arete::services {

namespace {
// WMO weather codes → short description + glyph.
struct CodeInfo { const char* label; const char* glyph; };

CodeInfo codeInfo(int code)
{
    if (code == 0) return {"Clear", ""};
    if (code <= 2) return {"Partly cloudy", ""};
    if (code == 3) return {"Overcast", ""};
    if (code <= 48) return {"Fog", ""};
    if (code <= 57) return {"Drizzle", ""};
    if (code <= 67) return {"Rain", ""};
    if (code <= 77) return {"Snow", ""};
    if (code <= 82) return {"Showers", ""};
    if (code >= 95) return {"Thunderstorm", ""};
    return {"Cloudy", ""};
}
} // namespace

WeatherService::WeatherService(app::AppContext* context, QObject* parent)
    : QObject(parent), m_context(context)
{
    m_network = std::make_unique<QNetworkAccessManager>(this);

    if (QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability)) {
        connect(QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged,
                this, [this](QNetworkInformation::Reachability reachability) {
                    if (reachability == QNetworkInformation::Reachability::Online)
                        refresh();
                });
    }
    const int intervalMinutes = m_context->settings()->get(QStringLiteral("weather/intervalMinutes"), 30);
    m_refreshTimer.setInterval(std::chrono::minutes(qMax(15, intervalMinutes)));
    m_refreshTimer.setSingleShot(false);
    connect(&m_refreshTimer, &QTimer::timeout, this, &WeatherService::refresh);

    loadCache();
    if (m_snapshot.valid) {
        emit updated(m_snapshot);
    }
    refresh();
    m_refreshTimer.start();
}

WeatherService::~WeatherService() = default;

WeatherSnapshot WeatherService::snapshot() const
{
    return m_snapshot;
}

QString WeatherService::icon() const
{
    return m_snapshot.valid ? QString::fromUtf8(codeInfo(m_snapshot.weatherCode).glyph) : QString();
}

QString WeatherService::label() const
{
    if (!m_snapshot.valid) return {};
    return QString::fromUtf8(codeInfo(m_snapshot.weatherCode).label);
}

void WeatherService::refresh()
{
    // Only hit the network when we are actually online; the UI keeps the
    // cached snapshot meanwhile, and a reconnect triggers the next refresh.
    if (QNetworkInformation::instance()
        && QNetworkInformation::instance()->reachability() != QNetworkInformation::Reachability::Online) {
        return;
    }
    ensureLocation();

    const double lat = m_context->settings()->get(QStringLiteral("weather/latitude"), 0.0);
    const double lon = m_context->settings()->get(QStringLiteral("weather/longitude"), 0.0);
    if (lat == 0.0 && lon == 0.0) return;

    QUrl url(QStringLiteral("https://api.open-meteo.com/v1/forecast"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("latitude"), QString::number(lat, 'f', 4));
    query.addQueryItem(QStringLiteral("longitude"), QString::number(lon, 'f', 4));
    query.addQueryItem(QStringLiteral("current"), QStringLiteral("temperature_2m,weather_code"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setTransferTimeout(15000);
    auto* reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] { handleReply(reply); });
}

void WeatherService::handleReply(QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        return; // keep the cached snapshot
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    const QJsonObject current = doc.object().value(QStringLiteral("current")).toObject();
    if (current.isEmpty()) return;

    m_snapshot.valid = true;
    m_snapshot.temperatureC = current.value(QStringLiteral("temperature_2m")).toDouble();
    m_snapshot.weatherCode = current.value(QStringLiteral("weather_code")).toInt();
    m_snapshot.location = m_context->settings()->get(QStringLiteral("weather/city"), QStringLiteral("Home"));
    m_snapshot.fetchedAt = QDateTime::currentDateTime();
    saveCache();
    emit updated(m_snapshot);
}

void WeatherService::loadCache()
{
    QFile file(m_context->dataDirectory() + QStringLiteral("/weather.json"));
    if (!file.open(QIODevice::ReadOnly)) return;
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QDateTime fetched = QDateTime::fromString(root.value(QStringLiteral("fetchedAt")).toString(), Qt::ISODate);
    if (fetched.isValid() && fetched.secsTo(QDateTime::currentDateTime()) < 3600 * 12) {
        m_snapshot.valid = true;
        m_snapshot.temperatureC = root.value(QStringLiteral("temperatureC")).toDouble();
        m_snapshot.weatherCode = root.value(QStringLiteral("weatherCode")).toInt();
        m_snapshot.location = root.value(QStringLiteral("location")).toString();
        m_snapshot.fetchedAt = fetched;
    }
}

void WeatherService::saveCache() const
{
    QJsonObject root;
    root[QStringLiteral("temperatureC")] = m_snapshot.temperatureC;
    root[QStringLiteral("weatherCode")] = m_snapshot.weatherCode;
    root[QStringLiteral("location")] = m_snapshot.location;
    root[QStringLiteral("fetchedAt")] = m_snapshot.fetchedAt.toString(Qt::ISODate);
    QFile file(m_context->dataDirectory() + QStringLiteral("/weather.json"));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }
}

void WeatherService::ensureLocation()
{
    if (m_context->settings()->contains(QStringLiteral("weather/latitude"))) return;

    // One-time approximate location from the local IP (no API key needed).
    auto* reply = m_network->get(QNetworkRequest(
        QUrl(QStringLiteral("https://ipapi.co/json/"))));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        m_context->settings()->set(QStringLiteral("weather/latitude"),
                                   obj.value(QStringLiteral("latitude")).toDouble());
        m_context->settings()->set(QStringLiteral("weather/longitude"),
                                   obj.value(QStringLiteral("longitude")).toDouble());
        m_context->settings()->set(QStringLiteral("weather/city"),
                                   obj.value(QStringLiteral("city")).toString());
        m_context->settings()->sync();
        refresh();
    });
}

} // namespace arete::services
