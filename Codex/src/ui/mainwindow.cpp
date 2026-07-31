#include "ui/mainwindow.h"
#include "ui/style.h"
#include "ui/settingsdialog.h"
#include "core/config.h"
#include "core/util.h"
#include "markdown/parser.h"
#include "index/search.h"
#include "index/backlinks.h"
#include "index/tags.h"
#include "export/export.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>
#include <QStatusBar>
#include <QScrollArea>
#include <QFileDialog>
#include <QDir>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMenu>
#include <QLabel>
#include <QLineEdit>
#include <QDialog>
#include <fstream>
#include <ctime>

namespace codex {

MainWindow::MainWindow(VaultManager *vault, QWidget *parent)
    : QMainWindow(parent)
    , m_vault(vault)
{
    setWindowTitle(QStringLiteral("Codex"));
    setMinimumSize(1000, 700);
    resize(1200, 800);

    setupUi();
    setStyleSheet(style::appStyleSheet());

    connect(m_vault, &VaultManager::noteSaved, this, &MainWindow::onNoteSaved);

    refreshSidebar();

    // Check vault password on startup
    codex::Config cfg;
    auto configPath = m_vault->absPath(vault_layout::FILE_CONFIG);
    if (cfg.load(configPath) && cfg.hasPassword() && cfg.vaultLocked()) {
        m_vaultUnlocked = false;
        applyLockState();
        unlockVault();
    } else {
        m_vaultUnlocked = true;
    }
}

MainWindow::~MainWindow()
{
    if (m_vault && m_vault->isOpen()) {
        auto configPath = m_vault->absPath(vault_layout::FILE_CONFIG);
        codex::Config cfg;
        cfg.setVaultPath(m_vault->vaultRoot());
        cfg.save(configPath);
    }
}

void MainWindow::setupUi()
{
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(splitter);

    auto *leftPanel = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    m_searchBar = new SearchBar(leftPanel);

    auto *exportBtn = new QPushButton(QStringLiteral("Export"), leftPanel);
    exportBtn->setToolTip(QStringLiteral("Export current note"));
    auto *exportMenu = new QMenu(this);
    exportMenu->addAction(QStringLiteral("HTML (.html)"), this, [this]() {
        doExport(codex::ExportManager::Html);
    });
    exportMenu->addAction(QStringLiteral("Markdown (.md)"), this, [this]() {
        doExport(codex::ExportManager::Markdown);
    });
    exportMenu->addAction(QStringLiteral("Plain Text (.txt)"), this, [this]() {
        doExport(codex::ExportManager::PlainText);
    });
    exportBtn->setMenu(exportMenu);

    m_sidebar = new Sidebar(m_vault, leftPanel);

    leftLayout->addWidget(m_searchBar);
    leftLayout->addWidget(exportBtn);
    leftLayout->addWidget(m_sidebar, 1);

    m_editor = new Editor;
    m_editor->setVaultRoot(m_vault->vaultRoot());

    auto *rightPanel = new QWidget;
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    m_rightTabs = new QTabWidget(rightPanel);
    m_backlinksPanel = new BacklinksPanel;
    m_tagsPanel = new TagsPanel;

    m_rightTabs->addTab(m_backlinksPanel, QStringLiteral("Backlinks"));
    m_rightTabs->addTab(m_tagsPanel, QStringLiteral("Tags"));

    rightLayout->addWidget(m_rightTabs);

    splitter->addWidget(leftPanel);
    splitter->addWidget(m_editor);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    splitter->setStretchFactor(2, 1);
    splitter->setSizes({250, 700, 250});

    auto *toolbar = addToolBar(QStringLiteral("Main"));
    toolbar->setMovable(false);
    setupToolbar(toolbar);

    m_fileInfo = new FileInfoPanel;
    m_statusLabel = new QLabel(QStringLiteral("Ready"));
    statusBar()->addWidget(m_fileInfo, 1);
    statusBar()->addPermanentWidget(m_statusLabel);

    connect(m_sidebar, &Sidebar::noteSelected, this, &MainWindow::onNoteSelected);
    connect(m_sidebar, &Sidebar::folderSelected, this, &MainWindow::onFolderSelected);
    connect(m_sidebar, &Sidebar::newNoteRequested, this, &MainWindow::newNoteInFolder);
    connect(m_sidebar, &Sidebar::newNotebookRequested, this, &MainWindow::newNotebook);
    connect(m_sidebar, &Sidebar::renameRequested, this, &MainWindow::renameNote);
    connect(m_sidebar, &Sidebar::duplicateRequested, this, &MainWindow::duplicateNote);
    connect(m_sidebar, &Sidebar::deleteRequested, this, &MainWindow::deletePath);
    connect(m_sidebar, &Sidebar::restoreRequested, this, &MainWindow::restoreNote);
    connect(m_editor, &Editor::fileSaved, this, &MainWindow::onNoteSaved);
    connect(m_editor, &Editor::cursorPositionChanged, this, &MainWindow::onEditorCursorChanged);
    connect(m_editor, &Editor::wikiLinkClicked, this, &MainWindow::onWikiLinkClicked);
    connect(m_editor, &Editor::textChanged, this, &MainWindow::onEditorTextChanged);
    connect(m_searchBar, &SearchBar::searchRequested, this, &MainWindow::onSearchRequested);
    connect(m_backlinksPanel, &BacklinksPanel::backlinkClicked, this, &MainWindow::onBacklinkClicked);
    connect(m_tagsPanel, &TagsPanel::tagClicked, this, &MainWindow::onTagClicked);
    connect(m_tagsPanel, &TagsPanel::tagAddRequested, this, [this](const std::string &tag) {
        if (!m_editor->hasFile()) return;
        // Insert #tag at cursor position in the editor
        auto cursor = m_editor->textCursor();
        cursor.insertText(QStringLiteral("#%1 ").arg(QString::fromStdString(tag)));
        m_editor->setTextCursor(cursor);
        m_editor->setFocus();
    });
}

void MainWindow::setupToolbar(QToolBar *toolbar)
{
    auto *newAction = toolbar->addAction(QStringLiteral("New Note"));
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::newNote);

    auto *nbAction = toolbar->addAction(QStringLiteral("New Notebook"));
    connect(nbAction, &QAction::triggered, this, [this]() {
        newNotebook(vault_layout::DIR_NOTES);
    });

    auto *deleteAction = toolbar->addAction(QStringLiteral("Delete"));
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteNote);

    auto *modeAction = toolbar->addAction(QStringLiteral("Reading View"));
    modeAction->setCheckable(true);
    modeAction->setShortcut(QKeySequence(Qt::Key_F5));
    connect(modeAction, &QAction::toggled, this, [this](bool checked) {
        m_editor->setMode(checked ? Editor::Preview : Editor::Source);
        if (auto *action = qobject_cast<QAction*>(sender())) {
            action->setText(checked ? QStringLiteral("Source") : QStringLiteral("Reading View"));
        }
    });

    toolbar->addSeparator();

    auto *vaultAction = toolbar->addAction(QStringLiteral("Vault"));
    auto *vaultMenu = new QMenu(this);
    vaultMenu->addAction(QStringLiteral("New Vault..."), this, &MainWindow::newVault);
    vaultMenu->addAction(QStringLiteral("Open Vault..."), this, &MainWindow::changeVault);
    vaultMenu->addAction(QStringLiteral("Rename Vault..."), this, &MainWindow::renameVault);
    vaultMenu->addAction(QStringLiteral("Delete Vault..."), this, [this]() {
        deleteVault();
    });
    vaultMenu->addSeparator();
    vaultMenu->addAction(QStringLiteral("New Template..."), this, &MainWindow::newTemplate);
    vaultMenu->addSeparator();
    vaultMenu->addAction(QStringLiteral("Settings..."), this, &MainWindow::openSettings);
    vaultAction->setMenu(vaultMenu);

    auto *securityAction = toolbar->addAction(QStringLiteral("Security"));
    auto *securityMenu = new QMenu(this);
    securityMenu->addAction(QStringLiteral("Set Password..."), this, &MainWindow::setVaultPassword);
    securityMenu->addAction(QStringLiteral("Change Password..."), this, &MainWindow::changeVaultPassword);
    securityMenu->addAction(QStringLiteral("Remove Password..."), this, &MainWindow::removeVaultPassword);
    securityMenu->addSeparator();
    securityMenu->addAction(QStringLiteral("Lock Vault"), this, &MainWindow::lockVault);
    securityMenu->addAction(QStringLiteral("Unlock Vault"), this, &MainWindow::unlockVault);
    securityAction->setMenu(securityMenu);

    toolbar->addSeparator();

    auto *helpAction = toolbar->addAction(QStringLiteral("Help"));
    connect(helpAction, &QAction::triggered, this, &MainWindow::showHelp);
}

void MainWindow::onNoteSelected(const std::filesystem::path &path)
{
    if (!m_vault || !m_vault->isOpen())
        return;

    if (!m_vaultUnlocked) {
        unlockVault();
        if (!m_vaultUnlocked) return;
    }

    auto noteOpt = m_vault->openNote(path);
    if (!noteOpt.has_value())
        return;

    const auto &note = noteOpt.value();
    m_currentNote = note.path;
    m_editor->loadFile(m_vault->absPath(note.path));
    m_statusLabel->setText(QStringLiteral("Opened: %1").arg(
        QString::fromStdString(note.path.string())));

    updatePanels();
}

void MainWindow::onNoteSaved(const std::filesystem::path &path)
{
    m_statusLabel->setText(QStringLiteral("Saved: %1").arg(
        QString::fromStdString(path.string())));
    refreshSidebar();
    updatePanels();
}

void MainWindow::onFolderSelected(const std::filesystem::path &folder)
{
    auto folderStr = QString::fromStdString(folder.string());

    // If Journal folder is clicked, auto-create today's journal entry
    if (folder == vault_layout::DIR_JOURNAL) {
        auto t = std::time(nullptr);
        auto *lt = std::localtime(&t);
        auto dateStr = QStringLiteral("%1-%2-%3")
            .arg(lt->tm_year + 1900, 4, 10, QLatin1Char('0'))
            .arg(lt->tm_mon + 1, 2, 10, QLatin1Char('0'))
            .arg(lt->tm_mday, 2, 10, QLatin1Char('0'));

        auto journalPath = std::filesystem::path(vault_layout::DIR_JOURNAL) / (dateStr.toStdString() + ".md");

        // Check if today's entry exists
        auto noteOpt = m_vault->openNote(journalPath);
        if (!noteOpt) {
            // Create it from template or default
            auto templatePath = std::filesystem::path(vault_layout::DIR_TEMPLATES) / "Journal.md";
            std::string content;
            auto tmplNote = m_vault->openNote(templatePath);
            if (tmplNote) {
                content = tmplNote->content;
                // Replace {{date}} placeholder
                auto pos = content.find("{{date}}");
                if (pos != std::string::npos)
                    content.replace(pos, 8, dateStr.toStdString());
            } else {
                content = "# " + dateStr.toStdString() + "\n\n";
            }

            Note newNote;
            newNote.path = journalPath;
            newNote.content = content;
            newNote.title = dateStr.toStdString();
            newNote.isJournal = true;

            // Write directly
            std::ofstream f(m_vault->absPath(journalPath));
            if (f) {
                f << content;
                f.close();
                m_statusLabel->setText(QStringLiteral("Journal created: %1").arg(dateStr));
                refreshSidebar();
            }
        }

        onNoteSelected(journalPath);
        return;
    }

    m_statusLabel->setText(QStringLiteral("Folder: %1").arg(folderStr));
}

// ── Text Changes ──

void MainWindow::onEditorTextChanged()
{
    // Status bar updates, backlinks/tags sync handled by save
}

// ── Export ──

void MainWindow::doExport(codex::ExportManager::Format fmt)
{
    if (m_currentNote.empty()) return;
    auto dir = QFileDialog::getExistingDirectory(this,
        QStringLiteral("Export to..."),
        QString::fromStdString(m_vault->vaultRoot().string()));
    if (dir.isEmpty()) return;
    auto note = m_vault->openNote(m_currentNote);
    if (note.has_value()) {
        codex::ExportManager em(m_vault->vaultRoot());
        if (em.exportNote(*note, dir.toStdString(), fmt)) {
            QString fmtName;
            switch (fmt) {
                case codex::ExportManager::Html: fmtName = QStringLiteral("HTML"); break;
                case codex::ExportManager::Markdown: fmtName = QStringLiteral("Markdown"); break;
                case codex::ExportManager::PlainText: fmtName = QStringLiteral("Text"); break;
            }
            m_statusLabel->setText(QStringLiteral("Exported %1 to: %2").arg(fmtName, dir));
        }
    }
}

// ── New Note ──

void MainWindow::newNote()
{
    auto folder = m_sidebar->selectedFolder();
    newNoteInFolder(folder);
}

void MainWindow::newNoteInFolder(const std::filesystem::path &folder)
{
    if (!m_vault || !m_vault->isOpen())
        return;

    // Dialog with title and optional template
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("New Note"));
    auto *form = new QFormLayout(&dlg);

    auto *titleEdit = new QLineEdit(&dlg);
    titleEdit->setPlaceholderText(QStringLiteral("Note title"));
    form->addRow(QStringLiteral("Title:"), titleEdit);

    // Template selector
    auto *templateCombo = new QComboBox(&dlg);
    templateCombo->addItem(QStringLiteral("(none)"));
    auto notes = m_vault->listNotes();
    for (const auto &n : notes) {
        if (n.isTemplate) {
            templateCombo->addItem(QString::fromStdString(n.title));
        }
    }
    if (templateCombo->count() > 1)
        form->addRow(QStringLiteral("Template:"), templateCombo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);

    if (dlg.exec() != QDialog::Accepted)
        return;

    auto title = titleEdit->text().trimmed();
    if (title.isEmpty())
        return;

    auto targetFolder = folder;
    if (targetFolder.empty())
        targetFolder = vault_layout::DIR_NOTES;

    // If template selected, use it
    auto templateTitle = templateCombo->currentText();
    if (templateTitle != QStringLiteral("(none)")) {
        // Find template note
        for (const auto &n : notes) {
            if (n.isTemplate && n.title == templateTitle.toStdString()) {
                auto tmplNote = m_vault->openNote(n.path);
                if (tmplNote) {
                    auto content = tmplNote->content;
                    // Replace {{title}} placeholder
                    auto pos = content.find("{{title}}");
                    if (pos != std::string::npos)
                        content.replace(pos, 9, title.toStdString());

                    // Create note with content
                    auto filename = util::sanitizeFilename(title.toStdString());
                    auto notePath = targetFolder / (filename + ".md");
                    auto full = m_vault->absPath(notePath);

                    int counter = 1;
                    while (std::filesystem::exists(full)) {
                        notePath = targetFolder / (filename + "_" + std::to_string(counter) + ".md");
                        full = m_vault->absPath(notePath);
                        counter++;
                    }

                    std::ofstream f(full);
                    if (f) {
                        f << content;
                        f.close();
                        m_statusLabel->setText(QStringLiteral("Created from template: %1").arg(title));
                        refreshSidebar();

                        // Select the new note
                        auto noteItem = m_vault->openNote(notePath);
                        if (noteItem.has_value()) {
                            m_currentNote = noteItem->path;
                            m_editor->loadFile(m_vault->absPath(noteItem->path));
                            updatePanels();
                        }
                    }
                    return;
                }
            }
        }
    }

    // No template - create blank note
    if (m_vault->createNote(title.toStdString(), targetFolder)) {
        // Find the newly created note and open it
        auto filename = util::sanitizeFilename(title.toStdString());
        auto notePath = targetFolder / (filename + ".md");
        refreshSidebar();
        m_sidebar->selectNote(notePath);
        m_statusLabel->setText(QStringLiteral("Created: %1").arg(title));
    }
}

// ── New Template ──

void MainWindow::newTemplate()
{
    if (!m_vault || !m_vault->isOpen()) return;

    auto name = QInputDialog::getText(this,
        QStringLiteral("New Template"),
        QStringLiteral("Template name:"));
    if (name.isEmpty()) return;

    auto filename = util::sanitizeFilename(name.trimmed().toStdString());
    auto templatePath = std::filesystem::path(vault_layout::DIR_TEMPLATES) / (filename + ".md");
    if (std::filesystem::exists(m_vault->absPath(templatePath))) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("A template with that name already exists."));
        return;
    }

    auto content = "# " + name.trimmed().toStdString() + "\n\n";
    content += "{{title}}\n\n";

    std::ofstream f(m_vault->absPath(templatePath));
    if (f) {
        f << content;
        f.close();
        refreshSidebar();
        m_statusLabel->setText(QStringLiteral("Template created: %1").arg(name));

        auto note = m_vault->openNote(templatePath);
        if (note.has_value()) {
            m_currentNote = note->path;
            m_editor->loadFile(m_vault->absPath(note->path));
            updatePanels();
        }
    }
}

// ── New Notebook ──

void MainWindow::newNotebook(const std::filesystem::path &parentFolder)
{
    if (!m_vault || !m_vault->isOpen())
        return;

    bool ok = false;
    auto name = QInputDialog::getText(this,
        QStringLiteral("New Notebook"),
        QStringLiteral("Notebook name:"),
        QLineEdit::Normal, QString(), &ok);

    if (!ok || name.trimmed().isEmpty())
        return;

    auto folderPath = parentFolder / name.trimmed().toStdString();
    if (m_vault->createFolder(folderPath)) {
        refreshSidebar();
        m_statusLabel->setText(QStringLiteral("Notebook created: %1").arg(name));
    } else {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Failed to create notebook."));
    }
}

// ── Rename ──

void MainWindow::renameNote(const std::filesystem::path &path)
{
    if (!m_vault || !m_vault->isOpen())
        return;

    auto oldTitle = path.stem().string();
    bool ok = false;
    auto newTitle = QInputDialog::getText(this,
        QStringLiteral("Rename Note"),
        QStringLiteral("New title:"),
        QLineEdit::Normal,
        QString::fromStdString(oldTitle), &ok);

    if (!ok || newTitle.trimmed().isEmpty() || newTitle.trimmed().toStdString() == oldTitle)
        return;

    if (m_vault->renameNote(path, newTitle.trimmed().toStdString())) {
        // If the renamed note is currently open, update
        if (m_currentNote == path) {
            auto newPath = path.parent_path() / (newTitle.trimmed().toStdString() + ".md");
            m_currentNote = newPath;
            m_editor->loadFile(m_vault->absPath(newPath));
        }
        refreshSidebar();
        m_statusLabel->setText(QStringLiteral("Renamed to: %1").arg(newTitle));
    } else {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Failed to rename. A note with that name may already exist."));
    }
}

// ── Duplicate ──

void MainWindow::duplicateNote(const std::filesystem::path &path)
{
    if (!m_vault || !m_vault->isOpen())
        return;

    if (m_vault->duplicateNote(path)) {
        refreshSidebar();
        m_statusLabel->setText(QStringLiteral("Duplicated"));
    }
}

// ── Delete / Restore ──

void MainWindow::deleteNote()
{
    if (!m_vault || !m_vault->isOpen() || m_currentNote.empty())
        return;

    auto path = m_currentNote;
    bool inTrash = (path.parent_path() == vault_layout::DIR_TRASH);

    QString msg;
    if (inTrash)
        msg = QStringLiteral("Permanently delete '%1'?").arg(QString::fromStdString(path.stem().string()));
    else
        msg = QStringLiteral("Move '%1' to Trash?").arg(QString::fromStdString(path.stem().string()));

    auto reply = QMessageBox::question(this,
        inTrash ? QStringLiteral("Delete Permanently") : QStringLiteral("Delete Note"),
        msg, QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        deletePath(path);
    }
}

void MainWindow::deletePath(const std::filesystem::path &path)
{
    if (!m_vault || !m_vault->isOpen())
        return;

    bool inTrash = (path.parent_path() == vault_layout::DIR_TRASH);

    if (inTrash) {
        m_vault->permanentlyDelete(path);
    } else {
        m_vault->deleteNote(path);
    }

    if (m_currentNote == path) {
        m_currentNote.clear();
        m_editor->clear();
        m_editor->setModified(false);
        m_fileInfo->clear();
        m_backlinksPanel->clear();
        m_tagsPanel->clear();
    }

    m_statusLabel->setText(inTrash ? QStringLiteral("Note permanently deleted")
                                   : QStringLiteral("Note moved to Trash"));
    refreshSidebar();
}

void MainWindow::restoreNote(const std::filesystem::path &path)
{
    if (!m_vault || !m_vault->isOpen())
        return;

    if (m_vault->restoreNote(path)) {
        m_statusLabel->setText(QStringLiteral("Note restored"));
        refreshSidebar();
    } else {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Failed to restore. A note with the same name may already exist."));
    }
}

// ── Search / Backlinks / Tags ──

void MainWindow::onSearchRequested(const std::string &query)
{
    if (!m_vault || !m_vault->isOpen())
        return;

    if (query.empty()) {
        refreshSidebar();
        return;
    }

    SearchEngine engine;
    engine.rebuild(m_vault->vaultRoot());
    auto results = engine.query(query);

    m_sidebar->clear();
    auto *resultsRoot = new QTreeWidgetItem(m_sidebar, {
        QStringLiteral("Search Results (%1)").arg(static_cast<int>(results.size()))
    });
    resultsRoot->setExpanded(true);

    for (const auto &r : results) {
        auto display = QString::fromStdString(r.title);
        auto *item = new QTreeWidgetItem(resultsRoot, {display});
        item->setData(0, Qt::UserRole, QString::fromStdString(r.notePath.string()));
        item->setToolTip(0, QString::fromStdString(r.snippet));
    }
}

void MainWindow::onBacklinkClicked(const std::filesystem::path &path)
{
    onNoteSelected(path);
}

void MainWindow::onWikiLinkClicked(const QString &target)
{
    if (!m_vault || !m_vault->isOpen())
        return;

    auto resolved = m_vault->resolveWikiLink(target.toStdString());
    if (resolved) {
        onNoteSelected(*resolved);
        return;
    }

    auto reply = QMessageBox::question(this,
        QStringLiteral("Create Note"),
        QStringLiteral("Note \"%1\" does not exist. Create it?").arg(target),
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        auto title = target.toStdString();
        std::filesystem::path folder = vault_layout::DIR_NOTES;
        if (!m_currentNote.empty())
            folder = m_currentNote.parent_path();
        if (m_vault->createNote(title, folder)) {
            auto newPath = folder / (title + ".md");
            refreshSidebar();
            onNoteSelected(newPath);
        }
    }
}

void MainWindow::onTagClicked(const std::string &tag)
{
    if (!m_vault || !m_vault->isOpen())
        return;

    TagIndexer indexer;
    indexer.rebuild(m_vault->vaultRoot());
    auto paths = indexer.notesWithTag(tag);

    m_sidebar->clear();
    auto *root = new QTreeWidgetItem(m_sidebar, {
        QStringLiteral("Tag: #%1 (%2)").arg(
            QString::fromStdString(tag),
            QString::number(static_cast<int>(paths.size())))
    });
    root->setExpanded(true);

    for (const auto &p : paths) {
        auto display = QString::fromStdString(p.stem().string());
        auto *item = new QTreeWidgetItem(root, {display});
        item->setData(0, Qt::UserRole, QString::fromStdString(p.string()));
    }
}

void MainWindow::onEditorCursorChanged(int line, int col)
{
    if (!m_editor->hasFile()) return;
    m_fileInfo->update(
        line, col,
        m_editor->wordCount(),
        m_editor->characterCount(),
        m_editor->currentFile().stem().string()
    );
}

void MainWindow::changeVault()
{
    if (!m_vault) return;

    auto dir = QFileDialog::getExistingDirectory(this,
        QStringLiteral("Open Vault"),
        QString::fromStdString(m_vault->vaultRoot().string()));

    if (dir.isEmpty())
        return;

    auto newPath = std::filesystem::path(dir.toStdString());

    // Verify/init the new path with a test manager first
    codex::VaultManager testVault;
    if (!testVault.ensureVault(newPath)) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Cannot open or create vault at:\n%1").arg(dir));
        return;
    }

    // Apply the switch
    m_vault->closeVault();
    if (!m_vault->ensureVault(newPath)) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Failed to switch to vault at:\n%1").arg(dir));
        return;
    }

    m_currentNote.clear();
    m_editor->clear();
    m_editor->setModified(false);
    m_editor->setVaultRoot(newPath);
    m_fileInfo->clear();
    m_backlinksPanel->clear();
    m_tagsPanel->clear();
    setWindowTitle(QStringLiteral("Codex \u2014 %1").arg(
        QString::fromStdString(newPath.string())));
    refreshSidebar();
    m_statusLabel->setText(QStringLiteral("Switched to vault: %1").arg(dir));

    // Check vault password
    codex::Config vaultCfg;
    auto vaultConfigPath = m_vault->absPath(vault_layout::FILE_CONFIG);
    if (vaultCfg.load(vaultConfigPath) && vaultCfg.hasPassword()) {
        m_vaultUnlocked = false;
        vaultCfg.setVaultLocked(true);
        vaultCfg.save(vaultConfigPath);
        applyLockState();
        unlockVault();
    } else {
        m_vaultUnlocked = true;
    }

    if (m_vaultUnlocked) {
        // Open the Welcome note if it exists
        auto welcomePath = std::filesystem::path(vault_layout::DIR_NOTES) / "Welcome.md";
        auto noteOpt = m_vault->openNote(welcomePath);
        if (noteOpt.has_value()) {
            m_currentNote = noteOpt->path;
            m_editor->loadFile(m_vault->absPath(noteOpt->path));
            updatePanels();
        }
    }

    codex::Config cfg;
    cfg.setVaultPath(newPath);
    std::filesystem::create_directories(util::configDir());
    cfg.save(util::defaultConfigPath());
}

void MainWindow::newVault()
{
    if (!m_vault) return;

    auto parentDir = QFileDialog::getExistingDirectory(this,
        QStringLiteral("Choose parent folder for new vault"),
        QString::fromStdString(m_vault->isOpen() ? m_vault->vaultRoot().parent_path().string() : ""));

    if (parentDir.isEmpty())
        return;

    bool ok = false;
    auto vaultName = QInputDialog::getText(this,
        QStringLiteral("New Vault"),
        QStringLiteral("Vault name:"),
        QLineEdit::Normal, QStringLiteral("MyVault"), &ok);

    if (!ok || vaultName.trimmed().isEmpty())
        return;

    auto newPath = std::filesystem::path(parentDir.toStdString()) / vaultName.trimmed().toStdString();

    if (std::filesystem::exists(newPath)) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("A file or folder with that name already exists."));
        return;
    }

    m_vault->closeVault();
    if (!m_vault->createVault(newPath)) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Failed to create vault at:\n%1").arg(QString::fromStdString(newPath.string())));
        return;
    }

    m_vaultUnlocked = true;
    m_currentNote.clear();
    m_editor->clear();
    m_editor->setModified(false);
    m_editor->setVaultRoot(newPath);
    m_fileInfo->clear();
    m_backlinksPanel->clear();
    m_tagsPanel->clear();
    applyLockState();
    refreshSidebar();
    m_statusLabel->setText(QStringLiteral("New vault created: %1").arg(
        QString::fromStdString(newPath.string())));

    codex::Config cfg;
    cfg.setVaultPath(newPath);
    std::filesystem::create_directories(util::configDir());
    cfg.save(util::defaultConfigPath());

    // Open the welcome note
    auto welcomePath = std::filesystem::path(vault_layout::DIR_NOTES) / "Welcome.md";
    auto noteOpt = m_vault->openNote(welcomePath);
    if (noteOpt.has_value()) {
        m_currentNote = noteOpt->path;
        m_editor->loadFile(m_vault->absPath(noteOpt->path));
        updatePanels();
    }
}

void MainWindow::deleteVault()
{
    if (!m_vault || !m_vault->isOpen())
        return;

    auto vaultPath = m_vault->vaultRoot();
    auto vaultName = QString::fromStdString(vaultPath.filename().string());

    auto reply = QMessageBox::warning(this,
        QStringLiteral("Delete Vault"),
        QStringLiteral("Permanently delete the entire vault \"%1\" and all its data?\n\n"
                       "This cannot be undone. All notes, media, and configuration will be lost.")
            .arg(vaultName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    m_currentNote.clear();
    m_editor->clear();
    m_editor->setModified(false);
    m_fileInfo->clear();
    m_backlinksPanel->clear();
    m_tagsPanel->clear();

    m_vault->closeVault();

    std::error_code ec;
    std::filesystem::remove_all(vaultPath, ec);
    if (ec) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Failed to delete vault at:\n%1\n%2")
                .arg(QString::fromStdString(vaultPath.string()),
                     QString::fromStdString(ec.message())));
        return;
    }

    m_statusLabel->setText(QStringLiteral("Vault deleted: %1").arg(
        QString::fromStdString(vaultPath.string())));
    setWindowTitle(QStringLiteral("Codex \u2014 No vault"));

    // Create a new empty vault at a location chosen by the user
    auto parentDir = QFileDialog::getExistingDirectory(this,
        QStringLiteral("Choose a location for a new vault"),
        QString::fromStdString(vaultPath.parent_path().string()));

    if (parentDir.isEmpty()) {
        // User cancelled - create a temporary vault to keep the app usable
        auto tmpPath = std::filesystem::temp_directory_path() / "codex_recovery";
        m_vault->createVault(tmpPath);
        m_editor->setVaultRoot(tmpPath);
        refreshSidebar();
        m_statusLabel->setText(QStringLiteral("Recovery vault created at: %1")
            .arg(QString::fromStdString(tmpPath.string())));
        return;
    }

    bool ok = false;
    auto newName = QInputDialog::getText(this,
        QStringLiteral("New Vault"),
        QStringLiteral("Vault name:"),
        QLineEdit::Normal, vaultName, &ok);

    auto newPath = std::filesystem::path(parentDir.toStdString())
                 / (ok && !newName.trimmed().isEmpty() ? newName.trimmed().toStdString() : vaultName.toStdString());

    if (std::filesystem::exists(newPath)) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("A file or folder with that name already exists."));
        // Fall back to a recovery vault
        newPath = std::filesystem::temp_directory_path() / "codex_recovery";
    }

    if (!m_vault->createVault(newPath)) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Failed to create new vault at:\n%1").arg(QString::fromStdString(newPath.string())));
        return;
    }

    m_editor->setVaultRoot(newPath);
    setWindowTitle(QStringLiteral("Codex \u2014 %1").arg(
        QString::fromStdString(newPath.string())));
    refreshSidebar();
    m_statusLabel->setText(QStringLiteral("New vault created at: %1").arg(
        QString::fromStdString(newPath.string())));

    codex::Config cfg;
    cfg.setVaultPath(newPath);
    std::filesystem::create_directories(util::configDir());
    cfg.save(util::defaultConfigPath());

    // Open the welcome note
    auto welcomePath = std::filesystem::path(vault_layout::DIR_NOTES) / "Welcome.md";
    auto noteOpt = m_vault->openNote(welcomePath);
    if (noteOpt.has_value()) {
        m_currentNote = noteOpt->path;
        m_editor->loadFile(m_vault->absPath(noteOpt->path));
        updatePanels();
    }
}

void MainWindow::setVaultPassword()
{
    if (!m_vault || !m_vault->isOpen()) return;

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Set Vault Password"));
    auto *form = new QFormLayout(&dlg);

    auto *pwdEdit = new QLineEdit(&dlg);
    pwdEdit->setEchoMode(QLineEdit::Password);
    pwdEdit->setPlaceholderText(QStringLiteral("Enter new password"));
    form->addRow(QStringLiteral("Password:"), pwdEdit);

    auto *confirmEdit = new QLineEdit(&dlg);
    confirmEdit->setEchoMode(QLineEdit::Password);
    confirmEdit->setPlaceholderText(QStringLiteral("Confirm password"));
    form->addRow(QStringLiteral("Confirm:"), confirmEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);

    if (dlg.exec() != QDialog::Accepted) return;

    auto pwd = pwdEdit->text();
    if (pwd.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Password cannot be empty."));
        return;
    }
    if (pwd != confirmEdit->text()) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Passwords do not match."));
        return;
    }

    auto hash = Config::hashPassword(pwd);
    auto configPath = m_vault->absPath(vault_layout::FILE_CONFIG);
    codex::Config cfg;
    cfg.load(configPath);
    cfg.setPasswordHash(hash);
    cfg.setVaultLocked(false);
    cfg.save(configPath);
    m_vaultUnlocked = true;

    m_statusLabel->setText(QStringLiteral("Vault password set."));
}

void MainWindow::changeVaultPassword()
{
    if (!m_vault || !m_vault->isOpen()) return;

    auto configPath = m_vault->absPath(vault_layout::FILE_CONFIG);
    codex::Config cfg;
    cfg.load(configPath);
    if (!cfg.hasPassword()) {
        QMessageBox::information(this, QStringLiteral("No Password"),
            QStringLiteral("No password is currently set for this vault.\nUse \"Set Password...\" first."));
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Change Vault Password"));
    auto *form = new QFormLayout(&dlg);

    auto *oldPwdEdit = new QLineEdit(&dlg);
    oldPwdEdit->setEchoMode(QLineEdit::Password);
    oldPwdEdit->setPlaceholderText(QStringLiteral("Current password"));
    form->addRow(QStringLiteral("Current:"), oldPwdEdit);

    auto *newPwdEdit = new QLineEdit(&dlg);
    newPwdEdit->setEchoMode(QLineEdit::Password);
    newPwdEdit->setPlaceholderText(QStringLiteral("New password"));
    form->addRow(QStringLiteral("New:"), newPwdEdit);

    auto *confirmEdit = new QLineEdit(&dlg);
    confirmEdit->setEchoMode(QLineEdit::Password);
    confirmEdit->setPlaceholderText(QStringLiteral("Confirm new password"));
    form->addRow(QStringLiteral("Confirm:"), confirmEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);

    if (dlg.exec() != QDialog::Accepted) return;

    auto oldPwd = oldPwdEdit->text();
    if (Config::hashPassword(oldPwd) != cfg.passwordHash()) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Current password is incorrect."));
        return;
    }

    auto newPwd = newPwdEdit->text();
    if (newPwd.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("New password cannot be empty."));
        return;
    }
    if (newPwd != confirmEdit->text()) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Passwords do not match."));
        return;
    }

    cfg.setPasswordHash(Config::hashPassword(newPwd));
    cfg.setVaultLocked(false);
    cfg.save(configPath);
    m_vaultUnlocked = true;

    m_statusLabel->setText(QStringLiteral("Vault password changed."));
}

void MainWindow::removeVaultPassword()
{
    if (!m_vault || !m_vault->isOpen()) return;

    auto configPath = m_vault->absPath(vault_layout::FILE_CONFIG);
    codex::Config cfg;
    cfg.load(configPath);
    if (!cfg.hasPassword()) {
        QMessageBox::information(this, QStringLiteral("No Password"),
            QStringLiteral("No password is currently set for this vault."));
        return;
    }

    auto reply = QMessageBox::question(this, QStringLiteral("Remove Password"),
        QStringLiteral("Are you sure you want to remove the vault password?\n"
                       "The vault will no longer be protected."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    cfg.setPasswordHash({});
    cfg.setVaultLocked(false);
    cfg.save(configPath);
    m_vaultUnlocked = true;

    m_statusLabel->setText(QStringLiteral("Vault password removed."));
}

void MainWindow::lockVault()
{
    if (!m_vault || !m_vault->isOpen()) return;

    auto configPath = m_vault->absPath(vault_layout::FILE_CONFIG);
    codex::Config cfg;
    cfg.load(configPath);
    if (!cfg.hasPassword()) {
        QMessageBox::information(this, QStringLiteral("No Password"),
            QStringLiteral("No password is set for this vault.\nUse \"Set Password...\" first."));
        return;
    }

    m_currentNote.clear();
    m_editor->clear();
    m_editor->setModified(false);
    m_fileInfo->clear();
    m_backlinksPanel->clear();
    m_tagsPanel->clear();
    m_vaultUnlocked = false;
    cfg.setVaultLocked(true);
    cfg.save(configPath);
    applyLockState();
    m_statusLabel->setText(QStringLiteral("Vault locked."));
}

void MainWindow::unlockVault()
{
    if (!m_vault || !m_vault->isOpen()) return;

    auto configPath = m_vault->absPath(vault_layout::FILE_CONFIG);
    codex::Config cfg;
    cfg.load(configPath);
    if (!cfg.hasPassword()) {
        m_vaultUnlocked = true;
        return;
    }

    auto pwd = QInputDialog::getText(this, QStringLiteral("Unlock Vault"),
        QStringLiteral("Enter vault password:"), QLineEdit::Password);

    if (pwd.isEmpty()) return;

    if (Config::hashPassword(pwd) != cfg.passwordHash()) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Incorrect password."));
        return;
    }

    m_vaultUnlocked = true;
    cfg.setVaultLocked(false);
    cfg.save(configPath);
    applyLockState();
    m_statusLabel->setText(QStringLiteral("Vault unlocked."));
}

void MainWindow::applyLockState()
{
    if (m_vaultUnlocked) {
        setWindowTitle(QStringLiteral("Codex \u2014 %1").arg(
            QString::fromStdString(m_vault->vaultRoot().string())));
        m_editor->setVisible(true);
        m_sidebar->setVisible(true);
        m_rightTabs->setVisible(true);
    } else {
        m_editor->clear();
        m_sidebar->clear();
        m_backlinksPanel->clear();
        m_tagsPanel->clear();
        m_fileInfo->clear();
        m_editor->setVisible(false);
        m_sidebar->setVisible(false);
        m_rightTabs->setVisible(false);
        setWindowTitle(QStringLiteral("Codex \u2014 [LOCKED] %1").arg(
            QString::fromStdString(m_vault->vaultRoot().string())));
        m_statusLabel->setText(QStringLiteral("Vault is locked. Use Settings > Unlock Vault to unlock."));
    }
}

void MainWindow::renameVault()
{
    if (!m_vault || !m_vault->isOpen())
        return;

    auto oldPath = m_vault->vaultRoot();
    auto oldName = QString::fromStdString(oldPath.filename().string());

    bool ok = false;
    auto newName = QInputDialog::getText(this,
        QStringLiteral("Rename Vault"),
        QStringLiteral("New vault name:"),
        QLineEdit::Normal, oldName, &ok);

    if (!ok || newName.trimmed().isEmpty() || newName.trimmed() == oldName)
        return;

    auto newPath = oldPath.parent_path() / newName.trimmed().toStdString();


    if (std::filesystem::exists(newPath)) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("A file or folder with that name already exists."));
        return;
    }

    try {
        std::filesystem::rename(oldPath, newPath);
    } catch (...) {
        QMessageBox::warning(this, QStringLiteral("Error"),
            QStringLiteral("Failed to rename vault."));
        return;
    }

    m_vault->closeVault();
    if (m_vault->openVault(newPath)) {
        setWindowTitle(QStringLiteral("Codex — %1").arg(
            QString::fromStdString(newPath.string())));
        refreshSidebar();
        m_statusLabel->setText(QStringLiteral("Vault renamed to: %1").arg(newName));
    }
}

void MainWindow::showHelp()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Codex Help - Markdown Syntax"));
    dlg.resize(600, 550);

    auto *layout = new QVBoxLayout(&dlg);
    auto *browser = new QTextBrowser(&dlg);
    browser->setOpenExternalLinks(true);
    browser->setStyleSheet(QStringLiteral(
        "QTextBrowser { background: #1B1B1B; color: #D8D8D8; border: none; }"
        "a { color: #8A8A8A; }"
    ));

    auto helpText = QStringLiteral(R"(
<h2 style="color:#8A8A8A;">Markdown Syntax Reference</h2>

<h3 style="color:#8A8A8A;">Headings</h3>
<pre><code># Heading 1
## Heading 2
### Heading 3
</code></pre>

<h3 style="color:#8A8A8A;">Text Formatting</h3>
<pre><code>**bold**  __bold__
*italic*  _italic_
~~strikethrough~~
&lt;u&gt;underline&lt;/u&gt;
`inline code`
</code></pre>

<h3 style="color:#8A8A8A;">Lists</h3>
<pre><code>- Unordered item
* Unordered item
+ Unordered item
1. Ordered item
2. Ordered item
- [ ] Unchecked task
- [x] Checked task
</code></pre>

<h3 style="color:#8A8A8A;">Links &amp; Wiki Links</h3>
<pre><code>[Link text](https://example.com)
[[Wiki Link to another note]]
</code></pre>

<h3 style="color:#8A8A8A;">Images &amp; Media</h3>
<pre><code>![Alt text](path/to/image.png)
&lt;video src="video.mp4" controls&gt;&lt;/video&gt;
&lt;audio src="audio.mp3" controls&gt;&lt;/audio&gt;
</code></pre>

<h3 style="color:#8A8A8A;">Code Blocks</h3>
<pre><code>```
code block
multiple lines
```
</code></pre>

<h3 style="color:#8A8A8A;">Blockquotes</h3>
<pre><code>&gt; Quoted text
&gt; Multiple lines
</code></pre>

<h3 style="color:#8A8A8A;">Horizontal Rule</h3>
<pre><code>---
</code></pre>

<h3 style="color:#8A8A8A;">Alignment</h3>
<pre><code>&lt;center&gt;Centered text&lt;/center&gt;
&lt;p align="right"&gt;Right-aligned&lt;/p&gt;
</code></pre>

<h3 style="color:#8A8A8A;">Tags</h3>
<pre><code>#tag  #project/feature
</code></pre>

<h3 style="color:#8A8A8A;">Checkboxes</h3>
<pre><code>Click on [ ] or [x] in the editor to toggle.
Rendered as ☐ ☑ in preview.
</code></pre>

<h3 style="color:#8A8A8A;">Keyboard Shortcuts</h3>
<pre><code>Ctrl+S      Save
Ctrl+N      New note
F5          Toggle Source/Reading View
Escape      Back to Source from Reading View
Tab         Insert 4 spaces
</code></pre>
)");

    browser->setHtml(helpText);

    auto *closeBtn = new QPushButton(QStringLiteral("Close"), &dlg);
    closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #8A8A8A; color: #1B1B1B; padding: 6px 20px;"
        "border: none; border-radius: 4px; font-weight: bold; }"
    ));
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    layout->addWidget(browser);
    layout->addWidget(closeBtn, 0, Qt::AlignCenter);
    dlg.exec();
}

void MainWindow::openSettings()
{
    codex::ConfigData current;
    current.vaultPath = m_vault->vaultRoot();
    current.mediaInsert = MediaInsert::CopyToVault;
    current.autosaveIntervalMs = 3000;
    current.fontSize = 13;

    SettingsDialog dialog(current, this);
    if (dialog.exec() == QDialog::Accepted) {
        auto newConfig = dialog.result();
        if (newConfig.vaultPath != m_vault->vaultRoot()) {
            m_vault->closeVault();
            if (m_vault->ensureVault(newConfig.vaultPath)) {
                m_currentNote.clear();
                m_editor->clear();
                m_editor->setVaultRoot(newConfig.vaultPath);
                m_fileInfo->clear();
                m_backlinksPanel->clear();
                m_tagsPanel->clear();

                // Check password on the new vault
                codex::Config newCfg;
                auto vaultConfigPath = m_vault->absPath(vault_layout::FILE_CONFIG);
                if (newCfg.load(vaultConfigPath) && newCfg.hasPassword()) {
                    m_vaultUnlocked = false;
                    newCfg.setVaultLocked(true);
                    newCfg.save(vaultConfigPath);
                    applyLockState();
                    unlockVault();
                } else {
                    m_vaultUnlocked = true;
                }

                refreshSidebar();
                m_statusLabel->setText(QStringLiteral("Switched vault"));
            }
        }
    }
}

void MainWindow::refreshSidebar()
{
    m_sidebar->refresh();
}

void MainWindow::updatePanels()
{
    if (m_updatingPanels)
        return;
    m_updatingPanels = true;

    if (m_currentNote.empty() || !m_vault->isOpen()) {
        m_backlinksPanel->clear();
        m_tagsPanel->clear();
        m_updatingPanels = false;
        return;
    }

    m_editor->saveFile();

    BacklinkManager blm;
    auto blConfigPath = m_vault->absPath(vault_layout::FILE_BACKLINKS);
    blm.rebuild(m_vault->vaultRoot());
    auto backlinks = blm.backlinksTo(m_currentNote);
    m_backlinksPanel->showBacklinks(backlinks);
    blm.save(blConfigPath);

    auto noteOpt = m_vault->openNote(m_currentNote);
    if (noteOpt.has_value()) {
        m_tagsPanel->showTags(noteOpt->tags);
        m_fileInfo->update(
            m_editor->cursorLine(),
            m_editor->cursorColumn(),
            m_editor->wordCount(),
            m_editor->characterCount(),
            m_currentNote.stem().string()
        );
    } else {
        m_tagsPanel->clear();
    }

    m_updatingPanels = false;
}

} // namespace codex
