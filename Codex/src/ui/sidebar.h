#pragma once

#include <QTreeWidget>
#include <QMenu>
#include <QWidget>
#include <filesystem>

namespace codex {

class VaultManager;

class Sidebar : public QTreeWidget {
    Q_OBJECT
public:
    explicit Sidebar(VaultManager *vault, QWidget *parent = nullptr);

    void refresh();
    std::filesystem::path selectedPath() const;
    void selectNote(const std::filesystem::path &relativePath);

    // Determine selected folder for new note creation
    std::filesystem::path selectedFolder() const;

signals:
    void noteSelected(const std::filesystem::path &relativePath);
    void folderSelected(const std::filesystem::path &folderPath);

    void newNoteRequested(const std::filesystem::path &folder);
    void newNotebookRequested(const std::filesystem::path &parentFolder);
    void renameRequested(const std::filesystem::path &path);
    void duplicateRequested(const std::filesystem::path &path);
    void deleteRequested(const std::filesystem::path &path);
    void restoreRequested(const std::filesystem::path &path);

private:
    VaultManager *m_vault;
    QMenu *m_contextMenu;

    void buildTree();
    void onContextMenu(const QPoint &pos);
    QTreeWidgetItem* addNoteItem(QTreeWidgetItem *parent, const std::filesystem::path &relativePath,
                                 const std::string &title, bool isFolder = false);
    QTreeWidgetItem* ensureFolderBranch(QTreeWidgetItem *root, const std::filesystem::path &folderPath);
};

} // namespace codex
