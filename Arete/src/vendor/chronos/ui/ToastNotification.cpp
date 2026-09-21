#include "ToastNotification.h"
#include "Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

namespace chronos {

ToastNotification::ToastNotification(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                   | Qt::Tool | Qt::X11BypassWindowManagerHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedSize(340, 90);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto* container = new QWidget(this);
    container->setStyleSheet(QStringLiteral(
        "background: %1; border: 1px solid %2;"
    ).arg(Theme::Panel.name()).arg(Theme::Border.name()));

    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(4);

    auto* titleRow = new QHBoxLayout();
    m_titleLabel = new QLabel(container);
    m_titleLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 13px; font-weight: bold; background: transparent;"
    ).arg(Theme::PrimaryText.name()));
    titleRow->addWidget(m_titleLabel, 1);

    m_closeBtn = new QPushButton(QStringLiteral("\u2716"), container);
    m_closeBtn->setFixedSize(20, 20);
    m_closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: %1; border: none;"
        "  font-size: 12px; }"
        "QPushButton:hover { color: %2; }"
    ).arg(Theme::SecondaryText.name()).arg(Theme::PrimaryText.name()));
    connect(m_closeBtn, &QPushButton::clicked, this, &QWidget::hide);
    titleRow->addWidget(m_closeBtn);

    layout->addLayout(titleRow);

    m_messageLabel = new QLabel(container);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 12px; background: transparent;"
    ).arg(Theme::SecondaryText.name()));
    layout->addWidget(m_messageLabel);

    root->addWidget(container);

    // Close timer
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &ToastNotification::fadeOut);

    // Fade timer
    m_fadeTimer = new QTimer(this);
    m_fadeTimer->setInterval(40);
    connect(m_fadeTimer, &QTimer::timeout, this, [this]() {
        m_opacity -= 0.05;
        if (m_opacity <= 0) {
            m_fadeTimer->stop();
            hide();
            m_opacity = 1.0;
        }
        update();
    });
}

void ToastNotification::showMessage(const QString& title, const QString& message,
                                    int timeoutMs)
{
    m_titleLabel->setText(title);
    m_messageLabel->setText(message);
    m_opacity = 1.0;

    positionOnScreen();
    show();
    raise();
    m_timer->start(timeoutMs);
}

void ToastNotification::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setOpacity(m_opacity);
    p.fillRect(rect(), Qt::transparent);
}

void ToastNotification::mousePressEvent(QMouseEvent* /*event*/)
{
    m_timer->stop();
    hide();
}

void ToastNotification::positionOnScreen()
{
    auto* screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    QRect geo = screen->availableGeometry();
    move(geo.right() - width() - 16, geo.bottom() - height() - 16);
}

void ToastNotification::fadeOut()
{
    m_opacity = 1.0;
    m_fadeTimer->start();
}

} // namespace chronos
