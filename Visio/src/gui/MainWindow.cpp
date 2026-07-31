#include "MainWindow.hpp"
#include <visio/ui/Theme.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFileDialog>
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>
#include <QFont>
#include <QScrollArea>
#include <QDialog>

#include <thread>
#include <format>
#include <functional>

namespace visio {

namespace {

QString formatViews(std::uint64_t views)
{
    if (views >= 1'000'000'000)
        return QString::fromStdString(std::format("{:.1f}B", views / 1'000'000'000.0));
    if (views >= 1'000'000)
        return QString::fromStdString(std::format("{:.1f}M", views / 1'000'000.0));
    if (views >= 1'000)
        return QString::fromStdString(std::format("{:.1f}K", views / 1'000.0));
    return QString::number(views);
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Visio — YouTube Browser");
    resize(1280, 800);
    setMinimumSize(900, 600);

    setStyleSheet(Theme::appStyleSheet());

    setupUi();
}

MainWindow::~MainWindow() noexcept = default;

void MainWindow::setupUi()
{
    setupToolbar();
    setupStatusBar();

    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // ---- Left: Tabs ----
    auto* leftPanel = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    m_tabs = new QTabWidget(this);
    m_thumbnailManager = new QNetworkAccessManager(this);

    m_searchResults = new QListWidget(this);
    m_searchResults->setAlternatingRowColors(false);
    connect(m_searchResults, &QListWidget::currentRowChanged,
            this, &MainWindow::onSearchResultClicked);
    m_tabs->addTab(m_searchResults, "Search");

    m_historyList = new QListWidget(this);
    m_historyList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(m_historyList, &QListWidget::currentRowChanged,
            this, &MainWindow::onHistoryClicked);
    m_tabs->addTab(m_historyList, "History");

    m_queueList = new QListWidget(this);
    m_queueList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(m_queueList, &QListWidget::currentRowChanged,
            this, &MainWindow::onQueueClicked);
    m_tabs->addTab(m_queueList, "Queue");

    m_subList = new QListWidget(this);
    connect(m_subList, &QListWidget::currentRowChanged,
            this, &MainWindow::onSubClicked);
    connect(m_subList, &QListWidget::itemSelectionChanged,
            this, [this]() {
                m_unsubscribeBtn->setEnabled(!m_subList->selectedItems().isEmpty());
            });
    m_tabs->addTab(m_subList, "Subs");

    m_playlistList = new QListWidget(this);
    connect(m_playlistList, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem*) {
                onLoadPlaylist();
            });
    connect(m_playlistList, &QListWidget::itemClicked,
            this, [this](QListWidgetItem* item) {
                if (item)
                    setStatus(QString::fromStdString(
                        std::format("Playlist: {} (double-click to open)", item->text().toStdString())));
            });
    m_tabs->addTab(m_playlistList, "Playlists");

    connect(m_tabs, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    leftLayout->addWidget(m_tabs);

    // History buttons
    auto* historyBar = new QHBoxLayout();
    m_clearHistoryBtn = new QPushButton("Clear History", this);
    connect(m_clearHistoryBtn, &QPushButton::clicked, this, &MainWindow::onClearHistory);
    auto* historyInfoBtn = new QPushButton("Show Info", this);
    historyInfoBtn->setToolTip("Show detailed info about the selected video");
    connect(historyInfoBtn, &QPushButton::clicked, this, &MainWindow::onShowInfo);
    auto* historyRemoveBtn = new QPushButton("Remove Selected", this);
    historyRemoveBtn->setToolTip("Remove the selected item from history");
    connect(historyRemoveBtn, &QPushButton::clicked, this, &MainWindow::onRemoveSelected);
    historyBar->addWidget(historyRemoveBtn);
    historyBar->addWidget(historyInfoBtn);
    historyBar->addWidget(m_clearHistoryBtn);
    leftLayout->addLayout(historyBar);

    // Queue buttons
    auto* queueBar = new QHBoxLayout();
    m_clearQueueBtn = new QPushButton("Clear Queue", this);
    connect(m_clearQueueBtn, &QPushButton::clicked, this, &MainWindow::onClearQueue);
    m_removeQueueBtn = new QPushButton("Remove Selected", this);
    m_removeQueueBtn->setToolTip("Remove the selected item from the queue");
    connect(m_removeQueueBtn, &QPushButton::clicked, this, &MainWindow::onRemoveSelected);
    m_saveQueueBtn = new QPushButton("Save as Playlist", this);
    connect(m_saveQueueBtn, &QPushButton::clicked, this, &MainWindow::onSaveQueue);
    queueBar->addWidget(m_removeQueueBtn);
    queueBar->addWidget(m_clearQueueBtn);
    queueBar->addWidget(m_saveQueueBtn);
    leftLayout->addLayout(queueBar);

    // Playlist buttons
    auto* playlistBar = new QHBoxLayout();
    m_loadPlaylistBtn = new QPushButton("Open", this);
    m_loadPlaylistBtn->setToolTip("Load playlist videos into the Search tab");
    connect(m_loadPlaylistBtn, &QPushButton::clicked, this, &MainWindow::onLoadPlaylist);
    m_downloadPlaylistBtn = new QPushButton("Download All", this);
    m_downloadPlaylistBtn->setToolTip("Download all videos in search results or loaded playlist");
    connect(m_downloadPlaylistBtn, &QPushButton::clicked, this, &MainWindow::onDownloadPlaylist);
    m_deletePlaylistBtn = new QPushButton("Delete", this);
    connect(m_deletePlaylistBtn, &QPushButton::clicked, this, &MainWindow::onDeletePlaylist);
    playlistBar->addWidget(m_loadPlaylistBtn);
    playlistBar->addWidget(m_downloadPlaylistBtn);
    playlistBar->addWidget(m_deletePlaylistBtn);
    leftLayout->addLayout(playlistBar);

    // Subs buttons
    auto* subBar = new QHBoxLayout();
    m_unsubscribeBtn = new QPushButton("Unsubscribe", this);
    m_unsubscribeBtn->setEnabled(false);
    connect(m_unsubscribeBtn, &QPushButton::clicked, this, &MainWindow::onUnsubscribe);
    m_searchChannelBtn = new QPushButton("Search Channels", this);
    m_searchChannelBtn->setToolTip("Search for a channel to subscribe");
    connect(m_searchChannelBtn, &QPushButton::clicked, this, &MainWindow::onSearchChannels);
    subBar->addWidget(m_unsubscribeBtn);
    subBar->addWidget(m_searchChannelBtn);
    leftLayout->addLayout(subBar);

    splitter->addWidget(leftPanel);

    // ---- Right: Detail Panel ----
    auto* rightPanel = new QWidget(this);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* detailWidget = new QWidget(this);
    auto* detailLayout = new QVBoxLayout(detailWidget);
    detailLayout->setContentsMargins(12, 12, 12, 12);
    detailLayout->setSpacing(8);

    m_thumbnail = new QLabel(this);
    m_thumbnail->setFixedHeight(140);
    m_thumbnail->setAlignment(Qt::AlignCenter);
    m_thumbnail->setStyleSheet("background-color: #232323; border-radius: 8px;");
    m_thumbnail->setText("No video selected");
    detailLayout->addWidget(m_thumbnail);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setWordWrap(true);
    auto titleFont = m_titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    detailLayout->addWidget(m_titleLabel);

    m_authorLabel = new QLabel(this);
    m_authorLabel->setStyleSheet("color: #A9A9A9;");
    detailLayout->addWidget(m_authorLabel);

    auto* metaRow = new QHBoxLayout();
    m_durationLabel = new QLabel(this);
    m_durationLabel->setStyleSheet("color: #8A8A8A;");
    m_viewsLabel = new QLabel(this);
    m_viewsLabel->setStyleSheet("color: #9E9E9E;");
    m_publishedLabel = new QLabel(this);
    m_publishedLabel->setStyleSheet("color: #A9A9A9;");
    metaRow->addWidget(m_durationLabel);
    metaRow->addWidget(m_viewsLabel);
    metaRow->addWidget(m_publishedLabel);
    metaRow->addStretch();
    detailLayout->addLayout(metaRow);

    m_description = new QTextEdit(this);
    m_description->setReadOnly(true);
    m_description->setMaximumHeight(150);
    m_description->setStyleSheet(
        "QTextEdit { background-color: #202020; border: 1px solid #353535; "
        "border-radius: 4px; padding: 8px; color: #A9A9A9; }");
    detailLayout->addWidget(m_description);

    // Action buttons
    auto* actionsWidget = new QWidget(this);
    auto* actionsLayout = new QVBoxLayout(actionsWidget);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(6);

    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(6);

    m_qualitySelector = new QComboBox(this);
    m_qualitySelector->addItem("Best", static_cast<int>(Quality::Best));
    m_qualitySelector->addItem("720p", static_cast<int>(Quality::P720));
    m_qualitySelector->addItem("1080p", static_cast<int>(Quality::P1080));
    m_qualitySelector->addItem("2160p", static_cast<int>(Quality::P2160));
    m_qualitySelector->addItem("Audio Only", static_cast<int>(Quality::AudioOnly));
    m_qualitySelector->addItem("Worst", static_cast<int>(Quality::Worst));
    topRow->addWidget(m_qualitySelector);

    m_playBtn = new QPushButton("Play", this);
    m_playBtn->setStyleSheet(
        "QPushButton { background-color: #8A8A8A; color: #1B1B1B; "
        "font-weight: bold; padding: 8px 24px; border: none; border-radius: 4px; }"
        "QPushButton:hover { background-color: #9A9A9A; }");
    connect(m_playBtn, &QPushButton::clicked, this, &MainWindow::onPlay);
    topRow->addWidget(m_playBtn);

    m_downloadBtn = new QPushButton("Download", this);
    connect(m_downloadBtn, &QPushButton::clicked, this, &MainWindow::onDownload);
    topRow->addWidget(m_downloadBtn);

    m_downloadMp3Btn = new QPushButton("MP3", this);
    m_downloadMp3Btn->setToolTip("Download audio as MP3 with thumbnail and metadata");
    connect(m_downloadMp3Btn, &QPushButton::clicked, this, &MainWindow::onDownloadMp3);
    topRow->addWidget(m_downloadMp3Btn);

    actionsLayout->addLayout(topRow);

    auto* bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(6);

    m_queueBtn = new QPushButton("Add to Queue", this);
    connect(m_queueBtn, &QPushButton::clicked, this, &MainWindow::onAddToQueue);
    bottomRow->addWidget(m_queueBtn);

    auto* playlistAddBtn = new QPushButton("+ Playlist", this);
    playlistAddBtn->setToolTip("Add current video to an existing playlist");
    connect(playlistAddBtn, &QPushButton::clicked, this, &MainWindow::onAddToPlaylist);
    bottomRow->addWidget(playlistAddBtn);

    m_subscribeBtn = new QPushButton("Subscribe", this);
    connect(m_subscribeBtn, &QPushButton::clicked, this, &MainWindow::onSubscribe);
    bottomRow->addWidget(m_subscribeBtn);

    bottomRow->addStretch();
    actionsLayout->addLayout(bottomRow);

    detailLayout->addWidget(actionsWidget);
    detailLayout->addStretch();

    scrollArea->setWidget(detailWidget);
    rightPanel->setMinimumWidth(320);
    rightPanel->setMaximumWidth(420);
    auto* rightOuterLayout = new QVBoxLayout(rightPanel);
    rightOuterLayout->setContentsMargins(0, 0, 0, 0);
    rightOuterLayout->addWidget(scrollArea);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter);
}

void MainWindow::setupToolbar()
{
    m_toolbar = new QToolBar("Main", this);
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(16, 16));
    addToolBar(m_toolbar);

    auto* searchLabel = new QLabel(" Search: ", this);
    searchLabel->setStyleSheet("color: #A9A9A9; font-weight: bold;");
    m_toolbar->addWidget(searchLabel);

    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText("Search YouTube...");
    m_searchInput->setMinimumWidth(300);
    m_searchInput->setStyleSheet(
        "QLineEdit { background-color: #333333; color: #D8D8D8; "
        "border: 1px solid #353535; border-radius: 4px; "
        "padding: 6px 12px; font-size: 14px; }"
        "QLineEdit:focus { border-color: #8A8A8A; }");
    m_toolbar->addWidget(m_searchInput);

    m_searchBtn = new QPushButton("Search", this);
    m_searchBtn->setStyleSheet(
        "QPushButton { background-color: #8A8A8A; color: #1B1B1B; "
        "font-weight: bold; padding: 6px 20px; border: none; "
        "border-radius: 4px; }"
        "QPushButton:hover { background-color: #9A9A9A; }");
    connect(m_searchBtn, &QPushButton::clicked, this, &MainWindow::onSearch);
    connect(m_searchInput, &QLineEdit::returnPressed, this, &MainWindow::onSearch);
    m_toolbar->addWidget(m_searchBtn);

    m_toolbar->addSeparator();

    auto* helpBtn = new QPushButton("Help", this);
    helpBtn->setStyleSheet(
        "QPushButton { background-color: #333333; color: #A9A9A9; "
        "padding: 6px 16px; border: 1px solid #353535; "
        "border-radius: 4px; }"
        "QPushButton:hover { background-color: #3A3A3A; color: #D8D8D8; }");
    connect(helpBtn, &QPushButton::clicked, this, &MainWindow::onHelp);
    m_toolbar->addWidget(helpBtn);
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("Ready");
}

void MainWindow::onSearch()
{
    auto query = m_searchInput->text().trimmed();
    if (query.isEmpty()) return;

    setStatus(QString::fromStdString(std::format("Searching for \"{}\"...", query.toStdString())));
    m_searchBtn->setEnabled(false);
    QApplication::processEvents();

    auto results = m_client.search(query.toStdString(), 30);
    m_searchBtn->setEnabled(true);

    if (!results.hasValue()) {
        setStatus(QString::fromStdString(std::format("Search failed: {}", results.error().message())));
        return;
    }

    m_currentResults = std::move(results).value();
    m_tabs->setCurrentIndex(0);
    populateSearchResults(m_currentResults);
    setStatus(QString::fromStdString(std::format("Found {} results", m_currentResults.size())));
}

void MainWindow::populateSearchResults(const std::vector<Video>& videos)
{
    m_searchResults->clear();
    for (const auto& v : videos) {
        auto* item = makeVideoItem(v);
        m_searchResults->addItem(item);
    }
}

void MainWindow::populateHistory()
{
    m_historyList->clear();
    auto history = m_client.getHistory();
    if (!history.hasValue()) return;
    for (const auto& v : history.value()) {
        m_historyList->addItem(makeVideoItem(v));
    }
}

void MainWindow::populateQueue()
{
    m_queueList->clear();
    auto queue = m_client.getQueue();
    if (!queue.hasValue()) return;
    for (const auto& v : queue.value()) {
        m_queueList->addItem(makeVideoItem(v));
    }
}

void MainWindow::populateSubscriptions()
{
    m_subList->clear();
    auto subs = m_client.getSubscriptions();
    if (!subs.hasValue()) return;
    for (const auto& ch : subs.value()) {
        auto text = std::format("{}  [{}]  {}",
            ch.title, ch.channelName, ch.subscribers);
        auto* item = new QListWidgetItem(QString::fromStdString(text));
        item->setData(Qt::UserRole, QString::fromStdString(ch.channelId));
        item->setData(Qt::UserRole + 1, QString::fromStdString(ch.title));
        m_subList->addItem(item);
    }
}

void MainWindow::populatePlaylists()
{
    m_playlistList->clear();
    auto playlists = m_client.listPlaylists();
    if (!playlists.hasValue()) return;
    for (const auto& name : playlists.value()) {
        auto* item = new QListWidgetItem(QString::fromStdString(name));
        item->setData(Qt::UserRole, QString::fromStdString(name));
        m_playlistList->addItem(item);
    }
}

QListWidgetItem* MainWindow::makeVideoItem(const Video& video)
{
    auto text = std::format("{}\n{}  ·  {}  ·  {} views",
        video.title, video.author, video.duration, formatViews(video.views).toStdString());
    auto* item = new QListWidgetItem(QString::fromStdString(text));
    item->setData(Qt::UserRole, QString::fromStdString(video.id));
    item->setToolTip(QString::fromStdString(video.title));
    return item;
}

void MainWindow::onSearchResultClicked(int row)
{
    if (row < 0 || row >= static_cast<int>(m_currentResults.size())) return;
    showVideoDetail(m_currentResults[row]);
}

void MainWindow::showVideoDetail(const Video& video)
{
    m_currentVideo = video;

    m_titleLabel->setText(QString::fromStdString(video.title));
    m_authorLabel->setText(QString::fromStdString(std::format("by {}", video.author)));

    m_durationLabel->setText(QString::fromStdString(std::format("Duration: {}", video.duration)));
    m_viewsLabel->setText(formatViews(video.views) + " views");
    m_publishedLabel->setText(QString::fromStdString(video.published));
    m_description->setText(QString::fromStdString(video.description));

    // Load thumbnail via network (single shared manager, no re-fetch of the same URL)
    m_thumbnail->clear();
    if (!video.thumbnail.empty()) {
        const auto url = QString::fromStdString(video.thumbnail);
        if (url != m_thumbnailUrl) {
            m_thumbnailUrl = url;
            auto* reply = m_thumbnailManager->get(QNetworkRequest(QUrl(url)));
            connect(reply, &QNetworkReply::finished, this, [this, reply, url]() {
                reply->deleteLater();
                if (reply->error() != QNetworkReply::NoError)
                    return;
                if (url != m_thumbnailUrl)
                    return; // stale response from an older click
                auto data = reply->readAll();
                QPixmap pix;
                if (pix.loadFromData(data)) {
                    m_thumbnail->setPixmap(
                        pix.scaled(m_thumbnail->width(), 140,
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation));
                }
            });
        }
    } else {
        m_thumbnailUrl.clear();
        m_thumbnail->setText("No thumbnail");
    }
}

void MainWindow::onPlay()
{
    if (m_currentVideo.id.empty()) return;
    auto quality = static_cast<Quality>(m_qualitySelector->currentData().toInt());
    auto video = m_currentVideo;

    setStatus(QString::fromStdString(std::format("Playing: {}", video.title)));

    std::thread([this, video, quality]() {
        m_client.play(video, quality);
        QMetaObject::invokeMethod(this, [this]() {
            populateHistory();
            setStatus("Playback finished");
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::onDownload()
{
    if (m_currentVideo.id.empty()) return;

    auto dir = QFileDialog::getExistingDirectory(this, "Download to",
        QDir::homePath() + "/Downloads");
    if (dir.isEmpty()) return;

    auto quality = static_cast<Quality>(m_qualitySelector->currentData().toInt());
    auto video = m_currentVideo;
    auto dirStr = dir.toStdString();

    setStatus(QString::fromStdString(std::format("Downloading: {}", video.title)));

    std::thread([this, video, dirStr, quality]() {
        auto result = m_client.download(video, dirStr, quality);
        auto ok = result.hasValue();
        auto msg = ok ? std::string("Download complete")
                      : std::format("Download failed: {}", result.error().message());
        QMetaObject::invokeMethod(this, [this, ok, msg]() {
            setStatus(QString::fromStdString(msg));
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::onDownloadMp3()
{
    if (m_currentVideo.id.empty()) return;

    auto dir = QFileDialog::getExistingDirectory(this, "Download MP3 to",
        QDir::homePath() + "/Downloads");
    if (dir.isEmpty()) return;

    auto video = m_currentVideo;
    auto dirStr = dir.toStdString();

    setStatus(QString::fromStdString(std::format("Downloading MP3: {}", video.title)));

    std::thread([this, video, dirStr]() {
        auto result = m_client.downloadAudio(video, dirStr);
        auto ok = result.hasValue();
        auto msg = ok ? std::string("MP3 download complete")
                      : std::format("MP3 download failed: {}", result.error().message());
        QMetaObject::invokeMethod(this, [this, ok, msg]() {
            setStatus(QString::fromStdString(msg));
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::onDownloadPlaylist()
{
    if (m_currentResults.empty()) {
        setStatus("No results to download. Search something first, or load a playlist.");
        return;
    }

    auto dir = QFileDialog::getExistingDirectory(this, "Download All to",
        QDir::homePath() + "/Downloads");
    if (dir.isEmpty()) return;

    auto quality = static_cast<Quality>(m_qualitySelector->currentData().toInt());
    auto results = m_currentResults;
    auto dirStr = dir.toStdString();

    setStatus(QString::fromStdString(std::format("Downloading {} videos...", results.size())));

    std::thread([this, results, dirStr, quality]() {
        auto result = m_client.downloadMultiple(results, dirStr, quality);
        auto ok = result.hasValue();
        auto msg = ok ? std::string("Batch download complete")
                      : std::format("Batch download failed: {}", result.error().message());
        QMetaObject::invokeMethod(this, [this, ok, msg]() {
            setStatus(QString::fromStdString(msg));
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::onAddToQueue()
{
    if (m_currentVideo.id.empty()) return;
    m_client.addToQueue(m_currentVideo);
    setStatus(QString::fromStdString(std::format("Added to queue: {}", m_currentVideo.title)));
    populateQueue();
}

void MainWindow::onAddToPlaylist()
{
    if (m_currentVideo.id.empty()) return;

    auto playlists = m_client.listPlaylists();
    if (!playlists.hasValue() || playlists.value().empty()) {
        setStatus("No playlists exist. Save the Queue as a playlist first.");
        return;
    }

    QStringList names;
    for (const auto& p : playlists.value()) {
        names << QString::fromStdString(p);
    }

    bool ok = false;
    auto chosen = QInputDialog::getItem(this, "Add to Playlist",
        "Choose a playlist:", names, 0, false, &ok);
    if (!ok || chosen.isEmpty()) return;

    setStatus(QString::fromStdString(std::format("Adding to playlist '{}'...", chosen.toStdString())));
    auto result = m_client.addToPlaylist(chosen.toStdString(), m_currentVideo);
    if (result.hasValue()) {
        setStatus(QString::fromStdString(std::format("Added to '{}'", chosen.toStdString())));
    } else {
        setStatus(QString::fromStdString(std::format("Failed: {}", result.error().message())));
    }
}

void MainWindow::onClearHistory()
{
    m_client.clearHistory();
    m_historyList->clear();
    setStatus("History cleared");
}

void MainWindow::onClearQueue()
{
    m_client.clearQueue();
    m_queueList->clear();
    setStatus("Queue cleared");
}

void MainWindow::onRemoveSelected()
{
    QListWidget* list = nullptr;
    std::function<void(std::size_t)> removeAt;
    QString what;

    switch (m_tabs->currentIndex()) {
    case 1:
        list = m_historyList;
        removeAt = [this](std::size_t i) { m_client.removeHistoryEntry(i); };
        what = QStringLiteral("history");
        break;
    case 2:
        list = m_queueList;
        removeAt = [this](std::size_t i) { m_client.removeQueueEntry(i); };
        what = QStringLiteral("queue");
        break;
    default:
        setStatus("Select an item in History or Queue first");
        return;
    }

    const auto items = list->selectedItems();
    if (items.isEmpty()) {
        setStatus(QStringLiteral("Select an item in %1 first").arg(what));
        return;
    }

    QVector<int> rows;
    rows.reserve(items.size());
    for (const auto* item : items)
        rows.append(list->row(item));
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (const int row : rows)
        removeAt(static_cast<std::size_t>(row));

    if (m_tabs->currentIndex() == 1)
        populateHistory();
    else
        populateQueue();

    setStatus(QStringLiteral("Removed %1 entr%2 from %3")
                  .arg(rows.size())
                  .arg(rows.size() == 1 ? QLatin1String("y") : QLatin1String("ies"), what));
}

void MainWindow::onShowInfo()
{
    Video video;
    bool found = false;

    switch (m_tabs->currentIndex()) {
    case 0: { // Search
        const int row = m_searchResults->currentRow();
        if (row >= 0 && row < static_cast<int>(m_currentResults.size())) {
            video = m_currentResults[row];
            found = true;
        }
        break;
    }
    case 1: { // History
        const int row = m_historyList->currentRow();
        auto history = m_client.getHistory();
        if (row >= 0 && history.hasValue() && row < static_cast<int>(history.value().size())) {
            video = history.value()[row];
            found = true;
        }
        break;
    }
    case 2: { // Queue
        const int row = m_queueList->currentRow();
        auto queue = m_client.getQueue();
        if (row >= 0 && queue.hasValue() && row < static_cast<int>(queue.value().size())) {
            video = queue.value()[row];
            found = true;
        }
        break;
    }
    default:
        break;
    }

    if (!found) {
        if (m_currentVideo.id.empty()) {
            setStatus("No video selected");
            return;
        }
        video = m_currentVideo;
    }

    auto* dialog = new QDialog(this);
    dialog->setWindowTitle("Video Info");
    dialog->setModal(true);
    dialog->setMinimumWidth(460);
    dialog->setMaximumWidth(640);

    auto* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    auto* title = new QLabel(QString::fromStdString(video.title), dialog);
    auto tf = title->font();
    tf.setPointSize(15);
    tf.setBold(true);
    title->setFont(tf);
    title->setWordWrap(true);
    layout->addWidget(title);

    auto* meta = new QLabel(
        QStringLiteral("by %1 · %2 · %3 · %4")
            .arg(QString::fromStdString(video.author),
                 QString::fromStdString(video.duration),
                 formatViews(video.views),
                 QString::fromStdString(video.published)),
        dialog);
    meta->setStyleSheet(QStringLiteral("color: #A9A9A9;"));
    layout->addWidget(meta);

    auto* idLabel = new QLabel(
        QStringLiteral("Video ID: %1").arg(QString::fromStdString(video.id)), dialog);
    idLabel->setStyleSheet(QStringLiteral("color: #8A8A8A; font-family: monospace;"));
    layout->addWidget(idLabel);

    const QString watchUrl = QString::fromStdString(VideoUtils::watchUrl(video.id));
    auto* urlLabel = new QLabel(
        QStringLiteral("<a href=\"%1\" style=\"color: #8A8A8A;\">%1</a>").arg(watchUrl), dialog);
    urlLabel->setOpenExternalLinks(true);
    urlLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    urlLabel->setWordWrap(true);
    layout->addWidget(urlLabel);

    auto* desc = new QTextEdit(dialog);
    desc->setReadOnly(true);
    desc->setPlainText(QString::fromStdString(video.description));
    desc->setMaximumHeight(200);
    layout->addWidget(desc);

    auto* buttons = new QHBoxLayout();
    auto* openBtn = new QPushButton("Open in Browser", dialog);
    connect(openBtn, &QPushButton::clicked, dialog, [dialog, watchUrl]() {
        QDesktopServices::openUrl(QUrl(watchUrl));
    });
    auto* closeBtn = new QPushButton("Close", dialog);
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    buttons->addStretch();
    buttons->addWidget(openBtn);
    buttons->addWidget(closeBtn);
    layout->addLayout(buttons);

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
}

void MainWindow::onSaveQueue()
{
    bool ok;
    auto name = QInputDialog::getText(this, "Save Queue",
        "Playlist name:", QLineEdit::Normal, {}, &ok);
    if (!ok || name.isEmpty()) return;

    auto result = m_client.saveQueueAsPlaylist(name.toStdString());
    if (result.hasValue()) {
        setStatus(QString::fromStdString(std::format("Queue saved as playlist '{}'", name.toStdString())));
        populatePlaylists();
    } else {
        setStatus(QString::fromStdString(std::format("Save failed: {}", result.error().message())));
    }
}

void MainWindow::onLoadPlaylist()
{
    auto items = m_playlistList->selectedItems();
    if (items.isEmpty()) return;

    auto name = items[0]->data(Qt::UserRole).toString().toStdString();
    auto pl = m_client.loadPlaylist(name);
    if (!pl.hasValue()) {
        setStatus(QString::fromStdString(std::format("Failed to load playlist: {}", pl.error().message())));
        return;
    }

    m_currentResults = std::move(pl).value();
    populateSearchResults(m_currentResults);
    m_tabs->setCurrentIndex(0);
    setStatus(QString::fromStdString(std::format("Loaded playlist '{}' ({} videos)", name, m_currentResults.size())));
}

void MainWindow::onDeletePlaylist()
{
    auto items = m_playlistList->selectedItems();
    if (items.isEmpty()) return;

    auto name = items[0]->data(Qt::UserRole).toString().toStdString();
    auto result = m_client.deletePlaylist(name);
    if (result.hasValue()) {
        setStatus(QString::fromStdString(std::format("Deleted playlist '{}'", name)));
        populatePlaylists();
    } else {
        setStatus(QString::fromStdString(std::format("Delete failed: {}", result.error().message())));
    }
}

void MainWindow::onHistoryClicked(int row)
{
    if (row < 0) return;
    auto history = m_client.getHistory();
    if (!history.hasValue() || row >= static_cast<int>(history.value().size())) return;
    showVideoDetail(history.value()[row]);
}

void MainWindow::onQueueClicked(int row)
{
    if (row < 0) return;
    auto queue = m_client.getQueue();
    if (!queue.hasValue() || row >= static_cast<int>(queue.value().size())) return;
    showVideoDetail(queue.value()[row]);
}

void MainWindow::onSubClicked(int row)
{
    if (row < 0) return;
    auto item = m_subList->item(row);
    if (!item) return;

    auto channelId = item->data(Qt::UserRole).toString().toStdString();
    auto channelName = item->data(Qt::UserRole + 1).toString().toStdString();

    setStatus(QString::fromStdString(std::format("Loading videos from {}...", channelName)));
    QApplication::processEvents();

    auto videos = m_client.getChannelVideos(channelId, 30);
    if (!videos.hasValue() && channelName.starts_with('@')) {
        // Stored channel id may be stale/invalid — retry with the @handle.
        setStatus(QString::fromStdString(
            std::format("Channel id invalid, retrying with handle {}...", channelName)));
        QApplication::processEvents();
        videos = m_client.getChannelVideos(channelName, 30);
    }
    if (!videos.hasValue()) {
        setStatus(QString::fromStdString(std::format("Failed to load channel: {}", videos.error().message())));
        return;
    }

    m_currentResults = std::move(videos).value();
    populateSearchResults(m_currentResults);
    m_tabs->setCurrentIndex(0);
    setStatus(QString::fromStdString(std::format("{} — {} videos", channelName, m_currentResults.size())));
}

void MainWindow::onSubscribe()
{
    if (m_currentVideo.author.empty()) return;

    // Search channels by the video's author name
    auto channels = m_client.searchChannels(m_currentVideo.author);
    if (!channels.hasValue() || channels.value().empty()) {
        setStatus(QString::fromStdString(
            std::format("No channels found for '{}'. Try Search Channels tab.", m_currentVideo.author)));
        return;
    }

    auto& ch = channels.value()[0];
    auto result = m_client.subscribe(ch);
    if (result.hasValue()) {
        setStatus(QString::fromStdString(std::format("Subscribed to {}", ch.title)));
        populateSubscriptions();
    } else {
        setStatus(QString::fromStdString(std::format("Subscribe failed: {}", result.error().message())));
    }
}

void MainWindow::onSearchChannels()
{
    bool ok;
    auto query = QInputDialog::getText(this, "Search Channels",
        "Channel name:", QLineEdit::Normal, {}, &ok);
    if (!ok || query.isEmpty()) return;

    setStatus(QString::fromStdString(std::format("Searching channels: \"{}\"...", query.toStdString())));
    QApplication::processEvents();

    auto channels = m_client.searchChannels(query.toStdString());
    if (!channels.hasValue() || channels.value().empty()) {
        setStatus("No channels found");
        return;
    }

    // Show results and let user pick
    QStringList names;
    for (const auto& ch : channels.value()) {
        names << QString::fromStdString(
            std::format("{} ({})", ch.title, ch.subscribers));
    }

    bool picked = false;
    auto chosen = QInputDialog::getItem(this, "Select Channel",
        "Choose a channel to subscribe:", names, 0, false, &picked);
    if (!picked || chosen.isEmpty()) return;

    int idx = names.indexOf(chosen);
    if (idx < 0) return;

    auto result = m_client.subscribe(channels.value()[idx]);
    if (result.hasValue()) {
        setStatus(QString::fromStdString(
            std::format("Subscribed to {}", channels.value()[idx].title)));
        populateSubscriptions();
    } else {
        setStatus(QString::fromStdString(
            std::format("Subscribe failed: {}", result.error().message())));
    }
}

void MainWindow::onUnsubscribe()
{
    auto items = m_subList->selectedItems();
    if (items.isEmpty()) return;

    auto channelId = items[0]->data(Qt::UserRole).toString().toStdString();
    auto result = m_client.unsubscribe(channelId);
    if (result.hasValue()) {
        setStatus("Unsubscribed");
        populateSubscriptions();
    } else {
        setStatus(QString::fromStdString(std::format("Unsubscribe failed: {}", result.error().message())));
    }
}

void MainWindow::onTabChanged(int index)
{
    // Lists are kept fresh by their mutations; only populate on first visit.
    if (index == 1 && m_historyList->count() == 0) populateHistory();
    else if (index == 2 && m_queueList->count() == 0) populateQueue();
    else if (index == 3 && m_subList->count() == 0) populateSubscriptions();
    else if (index == 4 && m_playlistList->count() == 0) populatePlaylists();
}

void MainWindow::onHelp()
{
    auto* msg = new QMessageBox(this);
    msg->setWindowTitle("Help — Visio YouTube Browser");
    msg->setIcon(QMessageBox::Information);
    msg->setText(
        "<h2>Visio — YouTube Browser</h2>"
        "<hr>"
        "<h3>Getting Started</h3>"
        "<p>Type a search query in the toolbar and press <b>Enter</b> "
        "or click <b>Search</b>. Click any result to view its details.</p>"
        "<h3>Actions</h3>"
        "<ul>"
        "<li><b>Play</b> — Stream video in mpv (select quality first)</li>"
        "<li><b>Download</b> — Save video to disk (select quality/format)</li>"
        "<li><b>Add to Queue</b> — Add video to the playback queue</li>"
        "<li><b>Subscribe</b> — Subscribe to the current video's channel</li>"
        "<li><b>+ Playlist</b> — Add the current video to an existing playlist</li>"
        "</ul>"
        "<h3>Tabs</h3>"
        "<ul>"
        "<li><b>Search</b> — Search results (default tab)</li>"
        "<li><b>History</b> — Previously watched videos</li>"
        "<li><b>Queue</b> — Your playback queue; save as a playlist</li>"
        "<li><b>Subs</b> — Subscribed channels (use <b>Search Channels</b> to find channels)</li>"
        "<li><b>Playlists</b> — Saved playlists; click <b>Open</b> to load</li>"
        "</ul>"
        "<h3>Tips</h3>"
        "<ul>"
        "<li>Use the quality dropdown before playing or downloading</li>"
        "<li>Save your queue as a playlist from the Queue tab</li>"
        "<li>Search channels from the Subs tab using <b>Search Channels</b></li>"
        "<li>Click items in History or Queue to re-view details</li>"
        "</ul>"
        "<hr>"
        "<p style='color: #A9A9A9;'>Visio — C++20 Qt6 YouTube Browser</p>"
    );
    msg->setTextFormat(Qt::RichText);
    msg->setMinimumWidth(500);
    msg->exec();
}

void MainWindow::setStatus(const QString& message)
{
    statusBar()->showMessage(message, 5000);
}

} // namespace visio
