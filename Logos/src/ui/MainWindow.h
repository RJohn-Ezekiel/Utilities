#pragma once

#include "core/Reference.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>
#include <string>
#include <vector>

class Bible;
class BookmarkStorage;
class HistoryStorage;
class NoteStorage;
class NotesPanel;
class ReaderPane;
class SearchBar;
class SearchService;
class Sidebar;
class SideBySideReader;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void loadBibles(const std::string& biblesPath = "Bibles");
    void navigateTo(const Reference& ref);

private slots:
    void onNavigate();
    void onSearch(const std::string& query);
    void onTabClose(int index);
    void onRandomVerse();
    void onDailyVerse();
    void onToggleSideBySide();
    void onToggleNotes();
    void onToggleSidebar();
    void openSettings();

private:
    void setupUI();
    void setupShortcuts();
    void setupConnections();
    void buildVerseHtml(ReaderPane* pane, const Bible* bible,
                        const Reference& ref);
    void addHistory(const Reference& ref);
    void restoreSettings();
    void saveSettings();
    void applySettings();
    int findTab(const Reference& ref) const;

    // Data
    std::vector<std::unique_ptr<Bible>> translations_;
    const Bible* activeBible_ = nullptr;

    // Storage
    std::unique_ptr<BookmarkStorage> bookmarkStorage_;
    std::unique_ptr<HistoryStorage> historyStorage_;
    std::unique_ptr<NoteStorage> noteStorage_;

    // Services
    std::unique_ptr<SearchService> searchService_;

    // Layout
    QSplitter* mainSplitter_;

    // Top bar
    QLineEdit* referenceInput_;
    QComboBox* translationCombo_;
    QPushButton* sideBySideBtn_;
    QPushButton* notesBtn_;
    QPushButton* sidebarBtn_;

    // Panels
    Sidebar* sidebar_;
    NotesPanel* notesPanel_;

    // Reader area
    QWidget* readerArea_;
    QVBoxLayout* readerAreaLayout_;
    QTabWidget* tabWidget_;
    SideBySideReader* sideBySideReader_;
    SearchBar* searchBar_;

    // Status
    QLabel* statusLabel_;

    // State
    bool sideBySideMode_ = false;
};
