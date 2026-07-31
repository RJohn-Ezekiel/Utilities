#pragma once

#include "core/Types.h"
#include "lyrics/LyricsParser.h"

#include <QScrollArea>
#include <QVector>

class QLabel;
class QVBoxLayout;
class QVariantAnimation;

namespace phonio {

// Synchronized LRC lyrics view.
// Current line is bright and large, previous lines fade, upcoming lines are grey.
// Auto-scrolls smoothly; seeking re-synchronizes instantly; pause freezes it.
class LyricsWidget : public QScrollArea
{
    Q_OBJECT

public:
    explicit LyricsWidget(QWidget* parent = nullptr);

    void setDocument(const LyricsDocument& doc);
    void setPosition(qint64 positionMs);

    void showNoLyricsMessage();

private:
    void rebuildLabels();
    void animateScrollTo(int targetValue);
    void applyLineStyles(int activeLine);

    LyricsDocument m_document;
    QWidget* m_content;
    QVBoxLayout* m_layout;
    QVector<QLabel*> m_labels;
    QVariantAnimation* m_scrollAnimation;
    int m_activeLine = -1;
    qint64 m_lastPositionMs = -1;
};

} // namespace phonio
