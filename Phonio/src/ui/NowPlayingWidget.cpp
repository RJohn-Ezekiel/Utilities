#include "ui/NowPlayingWidget.h"

#include "ui/ArtworkLabel.h"
#include "ui/Theme.h"
#include "ui/TransportIcons.h"
#include "lyrics/LyricsWidget.h"
#include "player/PlaybackController.h"
#include "artwork/ArtworkManager.h"
#include "lyrics/LyricsManager.h"

#include <QLabel>
#include <QSlider>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QPushButton>
#include <utility>

namespace phonio {

namespace {
constexpr int kArtworkSize = 420;
}

NowPlayingWidget::NowPlayingWidget(PlaybackController* controller, ArtworkManager* artwork,
                                   LyricsManager* lyrics, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_artwork(artwork)
    , m_lyrics(lyrics)
    , m_artworkLabel(new ArtworkLabel(this))
    , m_title(new QLabel(this))
    , m_artist(new QLabel(this))
    , m_album(new QLabel(this))
    , m_metaSummary(new QLabel(this))
    , m_metaGrid(nullptr)
    , m_metaContainer(nullptr)
    , m_lyricsWidget(new LyricsWidget(this))
    , m_lyricsContainer(nullptr)
    , m_backButton(nullptr)
    , m_seekSlider(new QSlider(Qt::Horizontal, this))
    , m_currentTime(new QLabel(this))
    , m_remainingTime(new QLabel(this))
    , m_playPause(nullptr)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 16, 24, 16);
    root->setSpacing(0);

    // Top bar: back button
    m_backButton = makeIconButton(TransportIcons::backArrow(Theme::textPrimary()), tr("Back"));
    auto* topBar = new QHBoxLayout;
    topBar->addWidget(m_backButton);
    topBar->addStretch();
    root->addLayout(topBar);

    auto* body = new QHBoxLayout;
    body->setSpacing(36);

    // Left: artwork + info
    auto* left = new QVBoxLayout;
    left->setSpacing(18);
    m_artworkLabel->setMinimumSize(180, 180);
    m_artworkLabel->setMaximumSize(420, 420);
    m_artworkLabel->setCornerRadius(16);
    left->addWidget(m_artworkLabel, 1, Qt::AlignHCenter);

    m_title->setObjectName(QStringLiteral("bigTitle"));
    m_title->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_artist->setObjectName(QStringLiteral("pageSubtitle"));
    m_artist->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_album->setObjectName(QStringLiteral("pageSubtitle"));
    m_album->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_metaSummary->setObjectName(QStringLiteral("metaLabel"));
    m_metaSummary->setWordWrap(true);
    m_metaSummary->setTextInteractionFlags(Qt::TextSelectableByMouse);

    left->addWidget(m_title);
    left->addWidget(m_artist);
    left->addWidget(m_album);
    left->addWidget(m_metaSummary);
    left->addWidget(m_metaContainer);

    // Metadata grid (populated per track)
    m_metaContainer = new QWidget(this);
    auto* metaLayout = new QVBoxLayout(m_metaContainer);
    metaLayout->setContentsMargins(0, 0, 0, 0);
    metaLayout->setSpacing(8);
    m_metaGrid = new QGridLayout;
    m_metaGrid->setHorizontalSpacing(28);
    m_metaGrid->setVerticalSpacing(8);
    metaLayout->addLayout(m_metaGrid);
    metaLayout->addStretch();
    left->addWidget(m_metaContainer);

    body->addLayout(left, 0);

    // Right: lyrics
    auto* right = new QVBoxLayout;
    right->setSpacing(8);
    auto* lyricsHeader = new QLabel(tr("Lyrics"), this);
    lyricsHeader->setObjectName(QStringLiteral("sectionHeader"));
    right->addWidget(lyricsHeader);
    m_lyricsContainer = new QWidget(this);
    auto* lyricsBox = new QVBoxLayout(m_lyricsContainer);
    lyricsBox->setContentsMargins(0, 0, 0, 0);
    lyricsBox->addWidget(m_lyricsWidget);
    right->addWidget(m_lyricsContainer, 1);
    body->addLayout(right, 1);

    root->addLayout(body, 1);

    // Bottom: big seek bar + transport
    auto* bottom = new QVBoxLayout;
    bottom->setSpacing(10);

    auto* timeRow = new QHBoxLayout;
    m_currentTime->setStyleSheet(QStringLiteral("color: rgba(184,184,184,160); font-size: 12px;"));
    m_remainingTime->setStyleSheet(QStringLiteral("color: rgba(184,184,184,160); font-size: 12px;"));
    timeRow->addWidget(m_currentTime);
    timeRow->addStretch();
    timeRow->addWidget(m_remainingTime);

    m_seekSlider->setRange(0, 1);
    m_seekSlider->setEnabled(false);
    m_seekSlider->setFixedHeight(16);

    auto* transportRow = new QHBoxLayout;
    transportRow->setSpacing(10);
    auto* prev = makeIconButton(TransportIcons::skipBack(Theme::textPrimary()), tr("Previous"));
    m_playPause = makeIconButton(TransportIcons::play(Theme::textPrimary()), tr("Play/Pause"));
    m_playPause->setFixedSize(56, 56);
    m_playPause->setIconSize(QSize(28, 28));
    auto* next = makeIconButton(TransportIcons::skipForward(Theme::textPrimary()), tr("Next"));
    transportRow->addStretch();
    transportRow->addWidget(prev);
    transportRow->addWidget(m_playPause);
    transportRow->addWidget(next);
    transportRow->addStretch();

    bottom->addLayout(timeRow);
    bottom->addWidget(m_seekSlider);
    bottom->addLayout(transportRow);
    root->addLayout(bottom);

    // Wiring
    connect(m_backButton, &QToolButton::clicked, this, &NowPlayingWidget::backRequested);
    connect(prev, &QToolButton::clicked, this, [this] { m_controller->playPrevious(); });
    connect(next, &QToolButton::clicked, this, [this] { m_controller->playNext(); });
    connect(m_playPause, &QToolButton::clicked, this, [this] { m_controller->togglePlayPause(); });

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

    connect(m_controller, &PlaybackController::currentTrackChanged, this, &NowPlayingWidget::onCurrentTrackChanged);
    connect(m_controller, &PlaybackController::positionChanged, this, &NowPlayingWidget::onPositionChanged);
    connect(m_controller, &PlaybackController::durationChanged, this, &NowPlayingWidget::onDurationChanged);
    connect(m_controller, &PlaybackController::playbackStateChanged, this, &NowPlayingWidget::onPlaybackStateChanged);
    connect(m_lyrics, &LyricsManager::lyricsLoaded, this, &NowPlayingWidget::onLyricsLoaded);

    onLyricsLoaded();
}

QToolButton* NowPlayingWidget::makeIconButton(const QIcon& icon, const QString& tooltip)
{
    auto* button = new QToolButton(this);
    button->setObjectName(QStringLiteral("transportButton"));
    button->setIcon(icon);
    button->setIconSize(QSize(22, 22));
    button->setToolTip(tooltip);
    button->setFixedSize(44, 44);
    return button;
}

void NowPlayingWidget::onCurrentTrackChanged(const Track& track)
{
    m_title->setText(track.displayTitle());
    m_artist->setText(track.displayArtist());
    m_album->setText(track.album.isEmpty() ? QString() : track.album);
    reloadArtwork();
    m_lyrics->loadLyricsFor(track);
    m_seekSlider->setEnabled(track.durationMs > 0);
    m_seekSlider->setRange(0, track.durationMs > 0 ? track.durationMs : 1);
    m_seekSlider->setValue(0);
    m_currentTime->setText(QStringLiteral("0:00"));
    m_remainingTime->setText(QStringLiteral("-") + formatDurationMs(track.durationMs));
    m_metaSummary->setText(QStringLiteral("%1  ·  %2  ·  %3")
                               .arg(formatDurationMs(track.durationMs),
                                    formatBitrate(track.bitrate),
                                    track.sampleRate > 0
                                        ? tr("%1 Hz").arg(track.sampleRate)
                                        : tr("unknown sample rate")));
    populateMetaGrid(track);
}

void NowPlayingWidget::populateMetaGrid(const Track& track)
{
    // Remove previously added widgets (kept in m_metaLabels).
    for (QLabel* label : std::as_const(m_metaLabels)) {
        label->deleteLater();
    }
    m_metaLabels.clear();

    struct Field {
        const char* label;
        QString value;
    };
    const QVector<Field> fields = {
        { QT_TR_NOOP("Genre"), track.genre },
        { QT_TR_NOOP("Year"), track.year > 0 ? QString::number(track.year) : QString() },
        { QT_TR_NOOP("Track"), track.trackNumber > 0 ? QString::number(track.trackNumber) : QString() },
        { QT_TR_NOOP("Bitrate"), formatBitrate(track.bitrate) },
        { QT_TR_NOOP("Sample Rate"), track.sampleRate > 0 ? tr("%1 Hz").arg(track.sampleRate) : QString() },
        { QT_TR_NOOP("File Type"), track.fileType.toUpper() },
        { QT_TR_NOOP("Play Count"), QString::number(track.playCount) },
    };

    int row = 0;
    for (const auto& field : fields) {
        if (field.value.isEmpty())
            continue;
        auto* key = new QLabel(tr(field.label), this);
        key->setObjectName(QStringLiteral("metaLabel"));
        auto* value = new QLabel(field.value, this);
        value->setObjectName(QStringLiteral("metaValue"));
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_metaGrid->addWidget(key, row, 0);
        m_metaGrid->addWidget(value, row, 1);
        m_metaLabels.append(key);
        m_metaLabels.append(value);
        ++row;
    }
}

void NowPlayingWidget::reloadArtwork()
{
    if (const auto track = m_controller->currentTrack())
        m_artworkLabel->setArtwork(m_artwork->artworkFor(*track));
    else
        m_artworkLabel->setArtwork(m_artwork->placeholder(kArtworkSize));
}

void NowPlayingWidget::onPositionChanged(qint64 positionMs)
{
    if (m_seeking)
        return;
    updateBigSeek(positionMs);
}

void NowPlayingWidget::updateBigSeek(qint64 positionMs)
{
    m_seekSlider->setValue(qMin<qint64>(positionMs, m_seekSlider->maximum()));
    m_currentTime->setText(formatTime(positionMs));
    const qint64 remaining = m_controller->player()->durationMs() - positionMs;
    m_remainingTime->setText(QStringLiteral("-") + formatTime(qMax<qint64>(0, remaining)));
}

void NowPlayingWidget::onDurationChanged(qint64 durationMs)
{
    m_seekSlider->setRange(0, durationMs > 0 ? durationMs : 1);
    m_remainingTime->setText(QStringLiteral("-") + formatDurationMs(durationMs));
}

void NowPlayingWidget::onPlaybackStateChanged()
{
    const bool playing = m_controller->isPlaying();
    const QIcon icon = playing ? TransportIcons::pause(Theme::textPrimary())
                               : TransportIcons::play(Theme::textPrimary());
    m_playPause->setIcon(icon);
}

void NowPlayingWidget::onLyricsLoaded()
{
    m_lyricsWidget->setDocument(m_lyrics->document());
    m_lyricsContainer->setVisible(m_lyrics->hasLyrics());
}

} // namespace phonio
