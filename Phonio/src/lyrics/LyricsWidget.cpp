#include "lyrics/LyricsWidget.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QVariantAnimation>
#include <QScrollBar>
#include <QFont>

namespace phonio {

namespace {
constexpr int kFontSizeCurrent = 22;
constexpr int kFontSizeNormal = 17;
constexpr int kLineSpacing = 14;
constexpr int kCenterPadding = 140; // vertical padding so the active line lands mid-view
}

LyricsWidget::LyricsWidget(QWidget* parent)
    : QScrollArea(parent)
    , m_content(new QWidget(this))
    , m_layout(new QVBoxLayout(m_content))
    , m_scrollAnimation(new QVariantAnimation(this))
{
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setWidget(m_content);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_layout->setContentsMargins(12, kCenterPadding, 24, kCenterPadding);
    m_layout->setSpacing(kLineSpacing);
    m_layout->setAlignment(Qt::AlignHCenter);

    m_scrollAnimation->setDuration(350);
    m_scrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_scrollAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        verticalScrollBar()->setValue(value.toInt());
    });
}

void LyricsWidget::setDocument(const LyricsDocument& doc)
{
    m_document = doc;
    m_activeLine = -1;
    m_lastPositionMs = -1;
    m_scrollAnimation->stop();
    rebuildLabels();
}

void LyricsWidget::rebuildLabels()
{
    qDeleteAll(m_labels);
    m_labels.clear();

    if (m_document.isEmpty()) {
        showNoLyricsMessage();
        return;
    }

    for (const auto& line : m_document.lines) {
        auto* label = new QLabel(line.text, m_content);
        label->setWordWrap(true);
        label->setAlignment(Qt::AlignHCenter);
        label->setTextInteractionFlags(Qt::NoTextInteraction);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        m_layout->addWidget(label);
        m_labels.append(label);
    }
    applyLineStyles(-1);
    verticalScrollBar()->setValue(0);
}

void LyricsWidget::showNoLyricsMessage()
{
    auto* label = new QLabel(tr("No synchronized lyrics available."), m_content);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral("color: rgba(184,184,184,100); font-size: 16px;"));
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_layout->addWidget(label);
    m_labels.append(label);
}

void LyricsWidget::applyLineStyles(int activeLine)
{
    for (int i = 0; i < m_labels.size(); ++i) {
        QLabel* label = m_labels.at(i);
        QFont font = label->font();
        QString color;
        if (i == activeLine) {
            font.setPixelSize(kFontSizeCurrent);
            font.setWeight(QFont::DemiBold);
            color = QStringLiteral("color: #d8d8d8;");
        } else if (i == activeLine - 1 || i == activeLine + 1) {
            font.setPixelSize(kFontSizeNormal);
            font.setWeight(QFont::Normal);
            color = QStringLiteral("color: rgba(184,184,184,160);");
        } else if (i < activeLine) {
            font.setPixelSize(kFontSizeNormal);
            font.setWeight(QFont::Normal);
            color = QStringLiteral("color: rgba(184,184,184,110);");
        } else {
            font.setPixelSize(kFontSizeNormal);
            font.setWeight(QFont::Normal);
            color = QStringLiteral("color: rgba(184,184,184,80);");
        }
        label->setFont(font);
        label->setStyleSheet(color);
    }
}

void LyricsWidget::setPosition(qint64 positionMs)
{
    if (m_document.isEmpty() || m_labels.size() != m_document.lines.size())
        return;
    const int active = LyricsParser::activeLine(m_document, positionMs);
    if (active == m_activeLine && m_lastPositionMs == positionMs)
        return;
    m_lastPositionMs = positionMs;

    if (active != m_activeLine) {
        m_activeLine = active;
        applyLineStyles(active);
    }

    if (active >= 0) {
        const QLabel* label = m_labels.at(active);
        const int target = label->y() - kCenterPadding;
        if (m_scrollAnimation->state() == QAbstractAnimation::Running)
            m_scrollAnimation->stop();
        animateScrollTo(qMax(0, target));
    }
}

void LyricsWidget::animateScrollTo(int targetValue)
{
    const int current = verticalScrollBar()->value();
    if (qAbs(targetValue - current) < 2) {
        verticalScrollBar()->setValue(targetValue);
        return;
    }
    m_scrollAnimation->stop();
    m_scrollAnimation->setStartValue(current);
    m_scrollAnimation->setEndValue(targetValue);
    m_scrollAnimation->start();
}

} // namespace phonio
