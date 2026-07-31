#pragma once

#include <QWidget>

class QListWidget;
class QCheckBox;
class QProgressBar;

namespace phonio {

class SettingsManager;
class LibraryScanner;

// Settings page: music folders, rescan, playback preferences.
class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(SettingsManager* settings, LibraryScanner* scanner, QWidget* parent = nullptr);

signals:
    void rescanRequested();

private:
    SettingsManager* m_settings;
    LibraryScanner* m_scanner;
    QListWidget* m_folderList;
    QCheckBox* m_rememberPosition;
    QCheckBox* m_autoLyrics;
    QProgressBar* m_progressBar;
    bool m_scanning = false;
};

} // namespace phonio
