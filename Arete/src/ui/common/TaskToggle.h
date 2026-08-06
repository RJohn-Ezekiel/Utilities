#pragma once

#include <QWidget>
#include <QVariantAnimation>

namespace arete::ui::common {

// The one task-completion control used across Arete: a self-painted
// toggle with exactly two states — ✓ (completed) and ✕ (not completed).
// Hover, keyboard focus and a short ~120ms fade are built in, so no
// platform-native checkbox rendering is ever mixed into the UI.
class TaskToggle : public QWidget
{
    Q_OBJECT

public:
    explicit TaskToggle(QWidget* parent = nullptr);

    [[nodiscard]] bool isChecked() const { return m_checked; }
    [[nodiscard]] qreal progress() const { return m_progress; }

public slots:
    void setChecked(bool checked, bool animate = true);

signals:
    void toggled(bool checked);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    QSize sizeHint() const override { return {26, 26}; }

private:
    bool m_checked = false;
    bool m_hover = false;
    bool m_focused = false;
    qreal m_progress = 0.0;   // 0 = ✕ … 1 = ✓
    QVariantAnimation m_animation;
};

} // namespace arete::ui::common
