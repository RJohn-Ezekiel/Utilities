#include "ui/SettingsPage.h"

#include "settings/SettingsManager.h"
#include "library/LibraryScanner.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QProgressBar>
#include <QFileDialog>
#include <QDir>

namespace phonio {

SettingsPage::SettingsPage(SettingsManager* settings, LibraryScanner* scanner, QWidget* parent)
    : QWidget(parent)
    , m_settings(settings)
    , m_scanner(scanner)
    , m_folderList(new QListWidget(this))
    , m_rememberPosition(new QCheckBox(tr("Remember playback position per track"), this))
    , m_autoLyrics(new QCheckBox(tr("Automatically load .lrc files next to songs"), this))
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 16);
    root->setSpacing(14);

    auto* title = new QLabel(tr("Settings"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    root->addWidget(title);

    // --- Music folders ---
    auto* foldersHeader = new QLabel(tr("Music Folders"), this);
    foldersHeader->setObjectName(QStringLiteral("sectionHeader"));
    root->addWidget(foldersHeader);

    m_folderList->setObjectName(QStringLiteral("settingsList"));
    m_folderList->setMaximumHeight(180);
    root->addWidget(m_folderList);

    auto* folderButtons = new QHBoxLayout;
    folderButtons->setSpacing(8);
    auto* addButton = new QPushButton(tr("Add Folder..."), this);
    auto* removeButton = new QPushButton(tr("Remove"), this);
    auto* rescanButton = new QPushButton(tr("Rescan Library"), this);
    rescanButton->setObjectName(QStringLiteral("accentButton"));
    folderButtons->addWidget(addButton);
    folderButtons->addWidget(removeButton);
    folderButtons->addStretch();
    folderButtons->addWidget(rescanButton);
    root->addLayout(folderButtons);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_progressBar->setRange(0, 0);
    root->addWidget(m_progressBar);

    // --- Playback ---
    auto* playbackHeader = new QLabel(tr("Playback"), this);
    playbackHeader->setObjectName(QStringLiteral("sectionHeader"));
    root->addWidget(playbackHeader);
    m_rememberPosition->setChecked(m_settings->rememberPosition());
    m_autoLyrics->setChecked(m_settings->autoLoadLyrics());
    root->addWidget(m_rememberPosition);
    root->addWidget(m_autoLyrics);

    root->addStretch();

    // --- Wiring ---
    m_folderList->addItems(m_settings->musicFolders());

    connect(addButton, &QPushButton::clicked, this, [this] {
        const QString dir = QFileDialog::getExistingDirectory(this, tr("Add Music Folder"),
                                                              QDir::homePath());
        if (!dir.isEmpty()) {
            m_settings->addMusicFolder(dir);
            m_folderList->addItem(dir);
        }
    });
    connect(removeButton, &QPushButton::clicked, this, [this] {
        const auto items = m_folderList->selectedItems();
        for (auto* item : items) {
            m_settings->removeMusicFolder(item->text());
            delete item;
        }
    });
    connect(rescanButton, &QPushButton::clicked, this, [this, rescanButton] {
        if (m_scanning)
            return;
        if (m_folderList->count() == 0)
            return;
        emit rescanRequested();
    });
    connect(m_rememberPosition, &QCheckBox::toggled, m_settings, &SettingsManager::setRememberPosition);
    connect(m_autoLyrics, &QCheckBox::toggled, m_settings, &SettingsManager::setAutoLoadLyrics);

    connect(m_scanner, &LibraryScanner::started, this, [this] {
        m_scanning = true;
        m_progressBar->setVisible(true);
    });
    connect(m_scanner, &LibraryScanner::finished, this, [this] {
        m_scanning = false;
        m_progressBar->setVisible(false);
    });
}

} // namespace phonio
