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

#include <string_view>

#include <visio/client.hpp>

namespace visio {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() noexcept override;

private slots:
    void onSearch();
    void onSearchResultClicked(int row);
    void onPlay();
    void onDownload();
    void onDownloadMp3();
    void onDownloadPlaylist();
    void onAddToQueue();
    void onClearHistory();
    void onClearQueue();
    void onRemoveFromHistory();
    void onRemoveFromQueue();
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
    QPushButton* m_removeHistoryBtn{};
    QPushButton* m_removeQueueBtn{};
    QPushButton* m_saveQueueBtn{};
    QPushButton* m_loadPlaylistBtn{};
    QPushButton* m_deletePlaylistBtn{};
    QComboBox* m_qualitySelector{};

    Video m_currentVideo;
    std::vector<Video> m_currentResults;
};

} // namespace visio
