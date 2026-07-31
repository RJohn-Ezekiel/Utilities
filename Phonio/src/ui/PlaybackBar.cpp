#include "ui/PlaybackBar.h"

#include "ui/ArtworkLabel.h"
#include "ui/Theme.h"
#include "ui/TransportIcons.h"
#include "player/PlaybackController.h"
#include "artwork/ArtworkManager.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QToolButton>
#include <QStyle>
#include <QMouseEvent>

namespace phonio {

PlaybackBar::PlaybackBar(PlaybackController* controller, ArtworkManager* artwork, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_artwork(artwork)
    , m_art(new ArtworkLabel(this))
    , m_title(new QLabel(tr("Nothing playing"), this))
    , m_artist(new QLabel(tr("—"), this))
    , m_playPause(nullptr)
    , m_shuffle(nullptr)
    , m_repeat(nullptr)
    , m_seekSlider(new QSlider(Qt::Horizontal, this))
    , m_currentTime(new QLabel(QStringLiteral("0:00"), this))
    , m_totalTime(new QLabel(QStringLiteral("0:00"), this))
    , m_volumeSlider(new QSlider(Qt::Horizontal, this))
{
    setObjectName(QStringLiteral("playbackBar"));
    setFixedHeight(84);
    setAttribute(Qt::WA_StyledBackground);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 10, 16, 10);
    layout->setSpacing(14);

    // --- Track info ---
    m_art->setFixedSize(58, 58);
    m_art->setCornerRadius(8);
    m_title->setObjectName(QStringLiteral("nowPlayingTitle"));
    m_title->setTextInteractionFlags(Qt::NoTextInteraction);
    m_artist->setObjectName(QStringLiteral("nowPlayingArtist"));
    m_artist->setTextInteractionFlags(Qt::NoTextInteraction);

    auto* info = new QVBoxLayout;
    info->setSpacing(2);
    info->addWidget(m_title);
    info->addWidget(m_artist);
    info->addStretch();

    auto* trackWidget = new QWidget(this);
    trackWidget->setCursor(Qt::PointingHandCursor);
    trackWidget->setToolTip(tr("Open Now Playing"));
    auto* trackLayout = new QHBoxLayout(trackWidget);
    trackLayout->setContentsMargins(0, 0, 0, 0);
    trackLayout->setSpacing(12);
    trackLayout->addWidget(m_art);
    trackLayout->addLayout(info);
    trackWidget->setFixedWidth(260);
    m_trackInfoArea = trackWidget;

    // --- Transport ---
    const QColor iconColor = Theme::textPrimary();
    const QIcon prevIcon = TransportIcons::skipBack(iconColor);
    const QIcon nextIcon = TransportIcons::skipForward(iconColor);
    const QIcon playIcon = TransportIcons::play(iconColor);
    const QIcon pauseIcon = TransportIcons::pause(iconColor);
    const QIcon shuffleIcon = TransportIcons::shuffle(iconColor);
    const QIcon repeatIcon = TransportIcons::repeat(iconColor);
    const QIcon volumeIcon = TransportIcons::volume(iconColor);

    auto* prev = makeTransportButton(prevIcon, tr("Previous"));
    m_playPause = makeTransportButton(playIcon, tr("Play/Pause"));
    m_playPause->setCheckable(true);
    auto* next = makeTransportButton(nextIcon, tr("Next"));
    m_shuffle = makeTransportButton(shuffleIcon, tr("Shuffle"));
    m_shuffle->setCheckable(true);
    m_repeat = makeTransportButton(repeatIcon, tr("Repeat: Off"));
    m_repeat->setCheckable(true);

    auto* transport = new QHBoxLayout;
    transport->setSpacing(6);
    transport->addStretch(1);
    transport->addWidget(m_shuffle);
    transport->addWidget(prev);
    transport->addWidget(m_playPause);
    transport->addWidget(next);
    transport->addWidget(m_repeat);
    transport->addStretch(1);

    // --- Seek ---
    m_seekSlider->setRange(0, 1);
    m_seekSlider->setFixedWidth(420);
    m_seekSlider->setEnabled(false);
    m_currentTime->setStyleSheet(QStringLiteral("color: rgba(184,184,184,140); font-size: 11px; min-width: 38px;"));
    m_totalTime->setStyleSheet(QStringLiteral("color: rgba(184,184,184,140); font-size: 11px; min-width: 38px;"));

    auto* seekRow = new QHBoxLayout;
    seekRow->setSpacing(8);
    seekRow->addWidget(m_currentTime);
    seekRow->addWidget(m_seekSlider);
    seekRow->addWidget(m_totalTime);

    auto* center = new QVBoxLayout;
    center->setSpacing(6);
    center->addLayout(transport, 0);
    center->addLayout(seekRow, 0);

    // --- Volume ---
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(qRound(m_controller->player()->volume() * 100.0));
    m_volumeSlider->setFixedWidth(110);
    auto* volIcon = new QLabel(this);
    volIcon->setPixmap(volumeIcon.pixmap(18, 18));
    auto* volumeRow = new QHBoxLayout;
    volumeRow->setSpacing(6);
    volumeRow->addWidget(volIcon);
    volumeRow->addWidget(m_volumeSlider);
    volumeRow->addStretch(1);

    auto* volumeWrap = new QWidget(this);
    volumeWrap->setFixedWidth(260);
    volumeWrap->setLayout(volumeRow);

    layout->addWidget(trackWidget);
    layout->addStretch(1);
    layout->addLayout(center);
    layout->addStretch(1);
    layout->addWidget(volumeWrap);

    // --- Wiring ---
    connect(prev, &QToolButton::clicked, this, [this] { m_controller->playPrevious(); });
    connect(next, &QToolButton::clicked, this, [this] { m_controller->playNext(); });
    connect(m_playPause, &QToolButton::clicked, this, [this] { m_controller->togglePlayPause(); });
    connect(m_shuffle, &QToolButton::toggled, this,
            [this](bool on) { m_controller->setShuffle(on); });
    connect(m_repeat, &QToolButton::clicked, this, [this] { m_controller->cycleRepeatMode(); });

    connect(m_seekSlider, &QSlider::sliderPressed, this, [this] {
        m_seeking = true;
        m_wasPlayingBeforeSeek = m_controller->isPlaying();
    });
    connect(m_seekSlider, &QSlider::sliderReleased, this, [this] {
        m_controller->player()->seek(m_seekSlider->value());
        if (m_wasPlayingBeforeSeek)
            m_controller->player()->play();
        m_seeking = false;
    });
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        m_controller->player()->setVolume(value / 100.0);
    });

    connect(m_controller, &PlaybackController::currentTrackChanged, this, &PlaybackBar::onCurrentTrackChanged);
    connect(m_controller, &PlaybackController::positionChanged, this, &PlaybackBar::onPositionChanged);
    connect(m_controller, &PlaybackController::durationChanged, this, &PlaybackBar::onDurationChanged);
    connect(m_controller, &PlaybackController::playbackStateChanged, this, &PlaybackBar::onPlaybackStateChanged);
    connect(m_controller, &PlaybackController::shuffleChanged, this, &PlaybackBar::onShuffleChanged);
    connect(m_controller, &PlaybackController::repeatModeChanged, this, &PlaybackBar::onRepeatModeChanged);

    onShuffleChanged(m_controller->shuffleEnabled());
    onRepeatModeChanged(m_controller->repeatMode());
}

QToolButton* PlaybackBar::makeTransportButton(const QIcon& icon, const QString& tooltip)
{
    auto* button = new QToolButton(this);
    button->setObjectName(QStringLiteral("transportButton"));
    button->setIcon(icon);
    button->setIconSize(QSize(20, 20));
    button->setToolTip(tooltip);
    button->setFixedSize(36, 36);
    return button;
}

void PlaybackBar::onCurrentTrackChanged(const Track& track)
{
    m_title->setText(track.displayTitle());
    m_artist->setText(track.displayArtist());
    m_title->setToolTip(track.displayTitle());
    m_art->setArtwork(m_artwork->artworkFor(track));
    m_seekSlider->setEnabled(track.durationMs > 0);
    m_seekSlider->setRange(0, track.durationMs > 0 ? track.durationMs : 1);
    m_seekSlider->setValue(0);
    m_currentTime->setText(QStringLiteral("0:00"));
    m_totalTime->setText(formatDurationMs(track.durationMs));
}

void PlaybackBar::onPositionChanged(qint64 positionMs)
{
    if (m_seeking)
        return;
    m_seekSlider->setValue(qMin<qint64>(positionMs, m_seekSlider->maximum()));
    m_currentTime->setText(formatTime(positionMs));
}

void PlaybackBar::onDurationChanged(qint64 durationMs)
{
    m_seekSlider->setRange(0, durationMs > 0 ? durationMs : 1);
    m_totalTime->setText(formatDurationMs(durationMs));
}

void PlaybackBar::onPlaybackStateChanged()
{
    const bool playing = m_controller->isPlaying();
    const QIcon icon = playing ? TransportIcons::pause(Theme::textPrimary())
                               : TransportIcons::play(Theme::textPrimary());
    m_playPause->setIcon(icon);
    m_playPause->setChecked(playing);
}

void PlaybackBar::onShuffleChanged(bool enabled)
{
    m_shuffle->setChecked(enabled);
    m_shuffle->setIcon(TransportIcons::shuffle(enabled ? Theme::accent() : Theme::textPrimary()));
    m_shuffle->setToolTip(enabled ? tr("Shuffle: On") : tr("Shuffle: Off"));
}

void PlaybackBar::onRepeatModeChanged(RepeatMode mode)
{
    const bool active = mode != RepeatMode::Off;
    m_repeat->setChecked(active);
    m_repeat->setIcon(TransportIcons::repeat(active ? Theme::accent() : Theme::textPrimary()));
    switch (mode) {
    case RepeatMode::Off:
        m_repeat->setToolTip(tr("Repeat: Off"));
        break;
    case RepeatMode::All:
        m_repeat->setToolTip(tr("Repeat: All"));
        break;
    case RepeatMode::One:
        m_repeat->setToolTip(tr("Repeat: One"));
        break;
    }
}

void PlaybackBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    QWidget::mouseDoubleClickEvent(event);
    if (m_trackInfoArea->geometry().contains(event->pos()))
        emit artworkClicked();
}

void PlaybackBar::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
    if (event->button() == Qt::LeftButton
        && m_trackInfoArea->geometry().contains(event->pos()))
        emit artworkClicked();
}

} // namespace phonio
