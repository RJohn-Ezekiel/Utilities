#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QToolBar>
#include <QComboBox>
#include <QHash>
#include <QIcon>
#include <QSet>
#include <QStringList>

#include <string_view>

#include <visio/client.hpp>

class QNetworkAccessManager;

namespace visio {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() noexcept override;

    // Add a URL to the download queue (used by deep links / CLI)
    void addDownload(const QString& url);

private slots:
    void onSearch();
    void onSearchResultClicked(int row);
    void onPlay();
    void onDownload();
    void onDownloadMp3();
    void onDownloadPlaylist();
    void onAddToQueue();
    void onAddToPlaylist();
    void onClearHistory();
    void onClearQueue();
    void onRemoveSelected();
    void onShowInfo();
    void onSaveQueue();
    void onLoadPlaylist();
    void onDeletePlaylist();
    void onSubscribe();
    void onUnsubscribe();
    void onSearchChannels();
    void onHistoryClicked(int row);
    void onQueueClicked(int row);
    void onSubClicked(int row);
    void onTabChanged(int index);
    void onHelp();
    void onUpdateYtdlp();
    void onRetryFailed();
    void onClipboardToggle(bool enabled);
    void onWellnessReminder();

private:
    void setupUi();
    void setupToolbar();
    void setupStatusBar();
    void populateSearchResults(const std::vector<Video>& videos);
    void populateHistory();
    void populateQueue();
    void populateSubscriptions();
    void populatePlaylists();
    void showVideoDetail(const Video& video);
    void setStatus(const QString& message);
    QListWidgetItem* makeVideoItem(const Video& video);
    void startDownload(const Video& video, bool audioOnly = false);
    void loadThumbnail(const QString& url, QListWidgetItem* item);

    Client m_client;

    QToolBar* m_toolbar{};
    QLineEdit* m_searchInput{};
    QPushButton* m_searchBtn{};
    QTabWidget* m_tabs{};
    QListWidget* m_searchResults{};
    QListWidget* m_historyList{};
    QListWidget* m_queueList{};
    QListWidget* m_subList{};
    QListWidget* m_playlistList{};

    QLabel* m_thumbnail{};
    QLabel* m_titleLabel{};
    QLabel* m_authorLabel{};
    QLabel* m_durationLabel{};
    QLabel* m_viewsLabel{};
    QLabel* m_publishedLabel{};
    QTextEdit* m_description{};
    QPushButton* m_playBtn{};
    QPushButton* m_downloadBtn{};
    QPushButton* m_downloadMp3Btn{};
    QPushButton* m_downloadPlaylistBtn{};
    QPushButton* m_queueBtn{};
    QPushButton* m_subscribeBtn{};
    QPushButton* m_unsubscribeBtn{};
    QPushButton* m_searchChannelBtn{};
    QPushButton* m_clearHistoryBtn{};
    QPushButton* m_clearQueueBtn{};
    QPushButton* m_removeQueueBtn{};
    QPushButton* m_saveQueueBtn{};
    QPushButton* m_loadPlaylistBtn{};
    QPushButton* m_deletePlaylistBtn{};
    QPushButton* m_updateYtdlpBtn{};
    QPushButton* m_clipboardBtn{};
    QPushButton* m_retryBtn{};
    QComboBox* m_qualitySelector{};

    QNetworkAccessManager* m_thumbnailManager{};
    QString m_thumbnailUrl;

    // Per-URL icon cache and in-flight guard for list thumbnails.
    QHash<QString, QIcon> m_thumbnailCache;
    QSet<QString> m_thumbnailPending;

    Video m_currentVideo;
    std::vector<Video> m_currentResults;

    // Clipboard-detection state.
    QTimer* m_clipboardTimer{};
    QString m_lastClipboardText;

    // Failed downloads (id -> last used title).
    QStringList m_failedIds;

    // Wellness reminder (45-minute cadence).
    QTimer* m_wellnessTimer{};
};

} // namespace visio
