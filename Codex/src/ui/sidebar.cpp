#include "ui/sidebar.h"
#include "core/vault.h"

#include <QHeaderView>
#include <QAction>
#include <algorithm>

namespace codex {

Sidebar::Sidebar(VaultManager *vault, QWidget *parent)
    : QTreeWidget(parent)
    , m_vault(vault)
    , m_contextMenu(new QMenu(this))
{
    setHeaderHidden(true);
    setIndentation(16);
    setAnimated(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) {
        auto pathStr = item->data(0, Qt::UserRole).toString();
        if (pathStr.isEmpty()) {
            auto folderName = item->text(0).toLower().toStdString();
            if (folderName == "journal")
                emit folderSelected(vault_layout::DIR_JOURNAL);
            else if (folderName == "templates")
                emit folderSelected(vault_layout::DIR_TEMPLATES);
            else if (folderName == "trash")
                emit folderSelected(vault_layout::DIR_TRASH);
            else
                emit folderSelected(vault_layout::DIR_NOTES);
        } else {
            emit noteSelected(std::filesystem::path(pathStr.toStdString()));
        }
    });

    connect(this, &QTreeWidget::customContextMenuRequested, this, &Sidebar::onContextMenu);

    refresh();
}

void Sidebar::refresh()
{
    clear();
    buildTree();
}

std::filesystem::path Sidebar::selectedPath() const
{
    auto items = selectedItems();
    if (items.isEmpty())
        return {};
    return std::filesystem::path(items.first()->data(0, Qt::UserRole).toString().toStdString());
}

std::filesystem::path Sidebar::selectedFolder() const
{
    auto items = selectedItems();
    if (items.isEmpty())
        return vault_layout::DIR_NOTES;

    auto *item = items.first();
    auto pathStr = item->data(0, Qt::UserRole).toString();
    auto folderType = item->data(0, Qt::UserRole + 1).toString();

    if (pathStr.isEmpty()) {
        auto name = item->text(0).toLower().toStdString();
        if (name == "journal") return vault_layout::DIR_JOURNAL;
        if (name == "templates") return vault_layout::DIR_TEMPLATES;
        if (name == "trash") return vault_layout::DIR_TRASH;
        return vault_layout::DIR_NOTES;
    }

    if (folderType == QStringLiteral("folder"))
        return std::filesystem::path(pathStr.toStdString());

    if (folderType == QStringLiteral("note"))
        return std::filesystem::path(pathStr.toStdString()).parent_path();

    return vault_layout::DIR_NOTES;
}

void Sidebar::selectNote(const std::filesystem::path &relativePath)
{
    auto pathStr = QString::fromStdString(relativePath.string());
    std::function<void(QTreeWidgetItem*)> search = [&](QTreeWidgetItem *parent) {
        for (int i = 0; i < parent->childCount(); ++i) {
            auto *child = parent->child(i);
            if (child->data(0, Qt::UserRole).toString() == pathStr) {
                setCurrentItem(child);
                scrollToItem(child);
                return;
            }
            search(child);
        }
    };

    for (int i = 0; i < topLevelItemCount(); ++i)
        search(topLevelItem(i));
}

QTreeWidgetItem* Sidebar::addNoteItem(QTreeWidgetItem *parent, const std::filesystem::path &relativePath,
                                       const std::string &title, bool isFolder)
{
    auto display = isFolder
        ? QString::fromStdString(relativePath.filename().string())
        : QString::fromStdString(title.empty() ? relativePath.stem().string() : title);

    auto *item = new QTreeWidgetItem(parent, {display});
    item->setData(0, Qt::UserRole, QString::fromStdString(relativePath.string()));
    item->setData(0, Qt::UserRole + 1, isFolder ? QStringLiteral("folder") : QStringLiteral("note"));
    item->setToolTip(0, QString::fromStdString(relativePath.string()));
    return item;
}

void Sidebar::buildTree()
{
    if (!m_vault || !m_vault->isOpen())
        return;

    auto rootNotes     = new QTreeWidgetItem(this, {QStringLiteral("Notes")});
    auto rootJournal   = new QTreeWidgetItem(this, {QStringLiteral("Journal")});
    auto rootTemplates = new QTreeWidgetItem(this, {QStringLiteral("Templates")});
    auto rootTrash     = new QTreeWidgetItem(this, {QStringLiteral("Trash")});

    rootNotes->setExpanded(true);

    // Recursive scanner for nested folders
    std::function<void(const std::filesystem::path&, QTreeWidgetItem*)> scanDirRecursive =
        [&](const std::filesystem::path &relDir, QTreeWidgetItem *parentItem) {
        auto fullDir = m_vault->absPath(relDir);
        if (!std::filesystem::exists(fullDir)) return;

        std::vector<std::filesystem::path> subdirs;
        std::vector<std::filesystem::path> mdFiles;

        for (const auto &entry : std::filesystem::directory_iterator(fullDir)) {
            if (entry.is_directory()) {
                subdirs.push_back(entry.path());
            } else if (entry.is_regular_file() && entry.path().extension() == ".md") {
                mdFiles.push_back(entry.path());
            }
        }
        std::sort(subdirs.begin(), subdirs.end());
        std::sort(mdFiles.begin(), mdFiles.end());

        for (const auto &sd : subdirs) {
            auto rel = m_vault->relPath(sd);
            auto *fi = addNoteItem(parentItem, rel, {}, true);
            fi->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            scanDirRecursive(rel, fi);
        }

        for (const auto &f : mdFiles) {
            auto rel = m_vault->relPath(f);
            addNoteItem(parentItem, rel, rel.stem().string(), false);
        }
    };

    auto scanDir = [&](const std::filesystem::path &vaultDir, QTreeWidgetItem *rootItem, bool allowSubfolders) {
        auto dir = m_vault->absPath(vaultDir);
        if (!std::filesystem::exists(dir)) return;

        std::vector<std::filesystem::path> subdirs;
        std::vector<std::filesystem::path> mdFiles;

        for (const auto &entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_directory()) {
                subdirs.push_back(entry.path());
            } else if (entry.is_regular_file() && entry.path().extension() == ".md") {
                mdFiles.push_back(entry.path());
            }
        }
        std::sort(subdirs.begin(), subdirs.end());
        std::sort(mdFiles.begin(), mdFiles.end());

        if (allowSubfolders) {
            for (const auto &sd : subdirs) {
                auto rel = m_vault->relPath(sd);
                auto *fi = addNoteItem(rootItem, rel, {}, true);
                fi->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
                scanDirRecursive(rel, fi);
            }
        }

        for (const auto &f : mdFiles) {
            auto rel = m_vault->relPath(f);
            auto disp = rel.stem().string();
            if (rel == vault_layout::FILE_README)
                continue;
            addNoteItem(rootItem, rel, disp, false);
        }
    };

    scanDir(vault_layout::DIR_NOTES, rootNotes, true);
    scanDir(vault_layout::DIR_JOURNAL, rootJournal, false);
    scanDir(vault_layout::DIR_TEMPLATES, rootTemplates, false);
    scanDir(vault_layout::DIR_TRASH, rootTrash, false);
}

void Sidebar::onContextMenu(const QPoint &pos)
{
    auto *item = itemAt(pos);
    if (!item) return;

    auto pathStr = item->data(0, Qt::UserRole).toString();
    auto itemType = item->data(0, Qt::UserRole + 1).toString();
    auto rootName = item->text(0);

    m_contextMenu->clear();

    bool isRootNotes = (pathStr.isEmpty() && rootName == QStringLiteral("Notes"));
    bool isFolder = (itemType == QStringLiteral("folder"));
    bool isNote = (itemType == QStringLiteral("note"));
    bool inTrash = false;

    // Check if item is under Trash
    QTreeWidgetItem *p = item->parent();
    while (p) {
        if (p->text(0) == QStringLiteral("Trash") && p->data(0, Qt::UserRole).toString().isEmpty()) {
            inTrash = true;
            break;
        }
        p = p->parent();
    }

    if (isRootNotes || isFolder) {
        auto folder = isRootNotes
            ? std::filesystem::path(vault_layout::DIR_NOTES)
            : std::filesystem::path(pathStr.toStdString());

        auto *newNoteAct = m_contextMenu->addAction(QStringLiteral("New Note"));
        connect(newNoteAct, &QAction::triggered, this, [this, folder]() {
            emit newNoteRequested(folder);
        });

        auto *newNbAct = m_contextMenu->addAction(QStringLiteral("New Notebook"));
        connect(newNbAct, &QAction::triggered, this, [this, folder]() {
            emit newNotebookRequested(folder);
        });

        m_contextMenu->addSeparator();
    }

    if (isNote) {
        auto notePath = std::filesystem::path(pathStr.toStdString());

        if (!inTrash) {
            auto *renameAct = m_contextMenu->addAction(QStringLiteral("Rename"));
            connect(renameAct, &QAction::triggered, this, [this, notePath]() {
                emit renameRequested(notePath);
            });

            auto *dupAct = m_contextMenu->addAction(QStringLiteral("Duplicate"));
            connect(dupAct, &QAction::triggered, this, [this, notePath]() {
                emit duplicateRequested(notePath);
            });

            m_contextMenu->addSeparator();

            auto *delAct = m_contextMenu->addAction(QStringLiteral("Delete"));
            connect(delAct, &QAction::triggered, this, [this, notePath]() {
                emit deleteRequested(notePath);
            });
        } else {
            auto *restoreAct = m_contextMenu->addAction(QStringLiteral("Restore"));
            connect(restoreAct, &QAction::triggered, this, [this, notePath]() {
                emit restoreRequested(notePath);
            });

            auto *delAct = m_contextMenu->addAction(QStringLiteral("Delete Permanently"));
            connect(delAct, &QAction::triggered, this, [this, notePath]() {
                emit deleteRequested(notePath);
            });
        }
    }

    if (!m_contextMenu->isEmpty())
        m_contextMenu->exec(mapToGlobal(pos));
}

} // namespace codex
