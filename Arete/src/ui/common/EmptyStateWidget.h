#pragma once

#include <QWidget>
#include <QLabel>

namespace arete::ui::common {

// Calm, encouraging placeholder used instead of generic "no items" text.
class EmptyStateWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EmptyStateWidget(const QString& icon, const QString& message,
                              QWidget* parent = nullptr);

    void setMessage(const QString& message);
    void setIcon(const QString& icon);

private:
    QLabel* m_iconLabel;
    QLabel* m_messageLabel;
};

} // namespace arete::ui::common
