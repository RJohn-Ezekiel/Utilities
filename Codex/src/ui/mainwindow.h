#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QToolBar>
#include <QTabWidget>
#include <QLabel>
#include <QStackedWidget>
#include <filesystem>

#include "core/vault.h"
#include "ui/editor.h"
#include "ui/sidebar.h"
#include "ui/backlinkspanel.h"
#include "ui/tagspanel.h"
#include "ui/searchbar.h"
#include "ui/fileinfopanel.h"
#include "export/export.h"

namespace codex {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(VaultManager *vault, QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNoteSelected(const std::filesystem::path &path);
    void onNoteSaved(const std::filesystem::path &path);
    void onSearchRequested(const std::string &query);
    void onBacklinkClicked(const std::filesystem::path &path);
    void onTagClicked(const std::string &tag);
    void onEditorCursorChanged(int line, int col);
    void onEditorTextChanged();
    void onFolderSelected(const std::filesystem::path &folder);
    void onWikiLinkClicked(const QString &target);

    void newNote();
    void newNoteInFolder(const std::filesystem::path &folder);
    void newNotebook(const std::filesystem::path &parentFolder);
    void newTemplate();
    void renameNote(const std::filesystem::path &path);
    void duplicateNote(const std::filesystem::path &path);
    void deleteNote();
    void deletePath(const std::filesystem::path &path);
    void restoreNote(const std::filesystem::path &path);
    void changeVault();
    void newVault();
    void deleteVault();
    void renameVault();
    void openSettings();
    void doExport(codex::ExportManager::Format fmt);
    void showHelp();

    void setVaultPassword();
    void changeVaultPassword();
    void removeVaultPassword();
    void lockVault();
    void unlockVault();

private:
    VaultManager *m_vault;

    Editor *m_editor;

    Sidebar *m_sidebar;
    BacklinksPanel *m_backlinksPanel;
    TagsPanel *m_tagsPanel;
    SearchBar *m_searchBar;
    FileInfoPanel *m_fileInfo;
    QTabWidget *m_rightTabs;

    QLabel *m_statusLabel;
    std::filesystem::path m_currentNote;
    bool m_vaultUnlocked = true;
    bool m_updatingPanels = false;

    void setupUi();
    void setupToolbar(QToolBar *toolbar);
    void updatePanels();
    void refreshSidebar();
    void applyLockState();
};

} // namespace codex
