#include "ui/SlidingStack.h"

#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>

namespace phonio {

namespace {
constexpr int kSlideDurationMs = 260;
constexpr int kOffsetPixels = 48;
}

SlidingStack::SlidingStack(QWidget* parent)
    : QStackedWidget(parent)
{
}

void SlidingStack::slideTo(int index, Direction direction)
{
    if (index < 0 || index >= count() || index == currentIndex() || m_animating)
        return;

    const QWidget* outgoing = currentWidget();
    const QWidget* incoming = widget(index);
    animate(outgoing, incoming, direction);
}

void SlidingStack::animate(const QWidget* outgoing, const QWidget* incoming, Direction direction)
{
    auto* opacity = new QGraphicsOpacityEffect(const_cast<QWidget*>(incoming));
    const_cast<QWidget*>(incoming)->setGraphicsEffect(opacity);
    opacity->setOpacity(0.0);

    const int directionSign = (direction == Direction::Left) ? 1 : -1;
    const QPoint startOffset(directionSign * kOffsetPixels, 0);
    const QPoint endOffset(0, 0);

    auto* slide = new QPropertyAnimation(const_cast<QWidget*>(incoming), "pos", this);
    slide->setDuration(kSlideDurationMs);
    slide->setEasingCurve(QEasingCurve::OutCubic);
    slide->setStartValue(incoming->pos() + startOffset);
    slide->setEndValue(incoming->pos() + endOffset);

    auto* fade = new QPropertyAnimation(opacity, "opacity", this);
    fade->setDuration(kSlideDurationMs);
    fade->setEasingCurve(QEasingCurve::OutCubic);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);

    auto* group = new QParallelAnimationGroup(this);
    group->addAnimation(slide);
    group->addAnimation(fade);

    // QStackedWidget must switch before animating the incoming widget's geometry;
    // we animate via its "pos" only after show.
    setCurrentIndex(indexOf(const_cast<QWidget*>(incoming)));

    m_animating = true;
    connect(group, &QParallelAnimationGroup::finished, this, [this, incoming, opacity, group]() {
        const_cast<QWidget*>(incoming)->setGraphicsEffect(nullptr);
        opacity->deleteLater();
        group->deleteLater();
        m_animating = false;
        const QPoint pos(0, 0);
        const_cast<QWidget*>(incoming)->move(pos);
    });
    group->start();
}

} // namespace phonio
