#pragma once

#include <QStackedWidget>

class QPropertyAnimation;

namespace phonio {

// QStackedWidget with smooth slide+fade page transitions.
class SlidingStack : public QStackedWidget
{
    Q_OBJECT

public:
    explicit SlidingStack(QWidget* parent = nullptr);

    // Slides to the given index. Direction indicates where the incoming page comes from.
    enum class Direction { Left, Right };
    void slideTo(int index, Direction direction = Direction::Left);

private:
    void animate(const QWidget* outgoing, const QWidget* incoming, Direction direction);

    QPropertyAnimation* m_offsetAnimation;
    QPropertyAnimation* m_opacityAnimation;
    bool m_animating = false;
};

} // namespace phonio
