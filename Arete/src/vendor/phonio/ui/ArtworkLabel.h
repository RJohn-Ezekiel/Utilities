#pragma once

#include <QLabel>

class QPropertyAnimation;

namespace phonio {

// Rounded-corner artwork display with a smooth crossfade on pixmap change.
class ArtworkLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(qreal fadeProgress READ fadeProgress WRITE setFadeProgress)

public:
    explicit ArtworkLabel(QWidget* parent = nullptr);

    void setArtwork(const QPixmap& pixmap, bool animate = true);
    int cornerRadius() const { return m_cornerRadius; }
    void setCornerRadius(int radius) { m_cornerRadius = radius; update(); }

    qreal fadeProgress() const { return m_fadeProgress; }
    void setFadeProgress(qreal value)
    {
        m_fadeProgress = value;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPixmap m_pixmap;
    QPixmap m_oldPixmap;
    QPropertyAnimation* m_fade;
    qreal m_fadeProgress = 1.0;
    int m_cornerRadius = 12;
};

} // namespace phonio
