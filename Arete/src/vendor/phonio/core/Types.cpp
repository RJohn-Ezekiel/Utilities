#include "core/Types.h"

#include <QFileInfo>

namespace phonio {

QString formatDurationMs(qint64 ms)
{
    const qint64 totalSeconds = ms / 1000;
    return formatTime(totalSeconds * 1000);
}

QString formatTime(qint64 ms)
{
    if (ms < 0)
        ms = 0;
    const qint64 totalSeconds = ms / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;
    if (hours > 0)
        return QStringLiteral("%1:%2:%3").arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
}

QString formatBitrate(int kbps)
{
    if (kbps <= 0)
        return QStringLiteral("Unknown");
    return QStringLiteral("%1 kbps").arg(kbps);
}

} // namespace phonio
