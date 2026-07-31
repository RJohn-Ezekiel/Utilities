#pragma once

#include <QColor>
#include <QString>

class QApplication;

namespace phonio {

// Centralized theming: dark palette + QSS with an accent color.
class Theme
{
public:
    static void apply(QApplication& app, const QColor& accentColor);

    static const QColor& base() { return m_base; }
    static const QColor& surface() { return m_surface; }
    static const QColor& surfaceAlt() { return m_surfaceAlt; }
    static const QColor& border() { return m_border; }
    static const QColor& textPrimary() { return m_textPrimary; }
    static const QColor& textSecondary() { return m_textSecondary; }
    static const QColor& accent() { return m_accent; }

    static QString styleSheet(const QColor& accent);

private:
    static QColor m_base;
    static QColor m_surface;
    static QColor m_surfaceAlt;
    static QColor m_border;
    static QColor m_textPrimary;
    static QColor m_textSecondary;
    static QColor m_accent;
};

} // namespace phonio
