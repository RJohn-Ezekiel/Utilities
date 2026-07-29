#include "MainWindow.h"
#include "NotesPanel.h"
#include "ReaderPane.h"
#include "SearchBar.h"
#include "Sidebar.h"
#include "SideBySideReader.h"
#include "SettingsDialog.h"
#include "Theme.h"

#include "core/Bible.h"
#include "core/Reference.h"
#include "data/Loader.h"
#include "services/ReferenceParser.h"
#include "services/SearchService.h"
#include "services/VerseService.h"
#include "storage/BookmarkStorage.h"
#include "storage/HistoryStorage.h"
#include "storage/NoteStorage.h"

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QMessageBox>
#include <QSettings>

namespace {

constexpr auto SETTINGS_ORG = "BibleExplorer";
constexpr auto SETTINGS_APP = "BibleExplorer";

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Bible Explorer");
    setMinimumSize(800, 400);
    resize(1200, 750);

    bookmarkStorage_ = std::make_unique<BookmarkStorage>();
    historyStorage_ = std::make_unique<HistoryStorage>();
    noteStorage_ = std::make_unique<NoteStorage>();
    searchService_ = std::make_unique<SearchService>();

    setupUI();
    setupShortcuts();
    setupConnections();
    restoreSettings();
    applySettings();
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::setupUI()
{
    Theme::apply();

    auto* central = new QWidget;
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    setCentralWidget(central);

    // ── Toolbar ──
    auto* bar = new QWidget;
    bar->setFixedHeight(36);
    bar->setStyleSheet(QStringLiteral("background-color:%1; border-bottom:1px solid %2;")
        .arg(Theme::toolbar.name(), Theme::borders.name()));
    auto* barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(4, 2, 4, 2);
    barLayout->setSpacing(4);

    sidebarBtn_ = new QPushButton("Sidebar");
    sidebarBtn_->setFixedHeight(26);
    sidebarBtn_->setToolTip("Show/hide sidebar");
    barLayout->addWidget(sidebarBtn_);

    referenceInput_ = new QLineEdit;
    referenceInput_->setPlaceholderText("Reference (e.g., John 3:16)");
    referenceInput_->setFixedWidth(260);
    barLayout->addWidget(referenceInput_);

    auto* go = new QPushButton("Go");
    go->setFixedSize(44, 26);
    barLayout->addWidget(go);

    translationCombo_ = new QComboBox;
    translationCombo_->setFixedWidth(90);
    barLayout->addWidget(translationCombo_);

    sideBySideBtn_ = new QPushButton("Side-by-side");
    sideBySideBtn_->setFixedHeight(26);
    sideBySideBtn_->setToolTip("Compare two translations side by side");
    barLayout->addWidget(sideBySideBtn_);

    barLayout->addStretch();

    searchBar_ = new SearchBar;
    searchBar_->setFixedWidth(240);
    barLayout->addWidget(searchBar_);

    notesBtn_ = new QPushButton("Notes");
    notesBtn_->setFixedSize(60, 26);
    notesBtn_->setToolTip("Show/hide notes panel");
    barLayout->addWidget(notesBtn_);

    auto* settingsBtn = new QPushButton("Settings");
    settingsBtn->setFixedHeight(26);
    settingsBtn->setToolTip("Settings (Ctrl+,)");
    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::openSettings);
    barLayout->addWidget(settingsBtn);

    root->addWidget(bar);

    // ── Main splitter ──
    mainSplitter_ = new QSplitter(Qt::Horizontal);
    mainSplitter_->setHandleWidth(1);
    mainSplitter_->setChildrenCollapsible(true);
    root->addWidget(mainSplitter_, 1);

    // Sidebar
    sidebar_ = new Sidebar;
    sidebar_->setMinimumWidth(120);
    sidebar_->setMaximumWidth(350);
    mainSplitter_->addWidget(sidebar_);

    // Reader area
    readerArea_ = new QWidget;
    readerAreaLayout_ = new QVBoxLayout(readerArea_);
    readerAreaLayout_->setContentsMargins(0, 0, 0, 0);
    readerAreaLayout_->setSpacing(0);

    tabWidget_ = new QTabWidget;
    tabWidget_->setDocumentMode(true);
    tabWidget_->setTabsClosable(true);
    tabWidget_->setMovable(true);
    tabWidget_->setElideMode(Qt::ElideRight);

    sideBySideReader_ = new SideBySideReader;
    sideBySideReader_->hide();

    readerAreaLayout_->addWidget(tabWidget_, 1);
    readerAreaLayout_->addWidget(sideBySideReader_, 1);
    readerAreaLayout_->setStretchFactor(tabWidget_, 1);
    readerAreaLayout_->setStretchFactor(sideBySideReader_, 1);

    mainSplitter_->addWidget(readerArea_);

    // Notes panel
    notesPanel_ = new NotesPanel;
    notesPanel_->setMinimumWidth(120);
    notesPanel_->setMaximumWidth(350);
    notesPanel_->setStorage(noteStorage_.get());
    notesPanel_->hide();
    mainSplitter_->addWidget(notesPanel_);

    mainSplitter_->setStretchFactor(0, 0);
    mainSplitter_->setStretchFactor(1, 1);
    mainSplitter_->setStretchFactor(2, 0);

    // Status bar
    statusLabel_ = new QLabel("Ready");
    statusBar()->addWidget(statusLabel_, 1);
    statusBar()->setStyleSheet(QStringLiteral(
        "QStatusBar { background-color:%1; color:%2; border-top:1px solid %3; font-size:12px; padding:1px 8px; }"
    ).arg(Theme::statusBar.name(), Theme::secondaryText.name(), Theme::borders.name()));

    connect(referenceInput_, &QLineEdit::returnPressed, this, &MainWindow::onNavigate);
    connect(go, &QPushButton::clicked, this, &MainWindow::onNavigate);
}

void MainWindow::setupShortcuts()
{
    auto* sf = new QAction("Search", this);
    sf->setShortcut(QKeySequence("Ctrl+F"));
    sf->setShortcutContext(Qt::ApplicationShortcut);
    connect(sf, &QAction::triggered, this, [this]() { searchBar_->setFocus(); searchBar_->clear(); });
    addAction(sf);

    auto* sq = new QAction("Quit", this);
    sq->setShortcut(QKeySequence("Ctrl+Q"));
    sq->setShortcutContext(Qt::ApplicationShortcut);
    connect(sq, &QAction::triggered, qApp, &QApplication::quit);
    addAction(sq);

    auto* sr = new QAction("Focus Reference", this);
    sr->setShortcut(QKeySequence("Ctrl+L"));
    sr->setShortcutContext(Qt::ApplicationShortcut);
    connect(sr, &QAction::triggered, this, [this]() { referenceInput_->setFocus(); referenceInput_->selectAll(); });
    addAction(sr);

    auto* ss = new QAction("Settings", this);
    ss->setShortcut(QKeySequence("Ctrl+,"));
    ss->setShortcutContext(Qt::ApplicationShortcut);
    connect(ss, &QAction::triggered, this, &MainWindow::openSettings);
    addAction(ss);
}

void MainWindow::setupConnections()
{
    connect(sidebar_, &Sidebar::referenceSelected, this, &MainWindow::navigateTo);
    connect(tabWidget_, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabClose);
    connect(tabWidget_, &QTabWidget::currentChanged, this, [this](int i) {
        if (i >= 0) {
            auto* p = qobject_cast<ReaderPane*>(tabWidget_->widget(i));
            if (p) {
                auto ref = p->currentReference();
                notesPanel_->setCurrentReference(ref);
                if (ref.isValid()) {
                    statusLabel_->setText(QString::fromStdString(ref.toString()) + " | "
                        + QString::fromStdString(activeBible_->shortName()));
                }
            }
        }
    });
    connect(searchBar_, &SearchBar::searchRequested, this, &MainWindow::onSearch);
    connect(translationCombo_, &QComboBox::currentIndexChanged, this, [this](int i) {
        if (i >= 0 && i < static_cast<int>(translations_.size())) {
            activeBible_ = translations_[i].get();
            searchService_->buildIndex(*activeBible_);
        }
    });
    connect(sideBySideBtn_, &QPushButton::clicked, this, &MainWindow::onToggleSideBySide);
    connect(notesBtn_, &QPushButton::clicked, this, &MainWindow::onToggleNotes);
    connect(sidebarBtn_, &QPushButton::clicked, this, &MainWindow::onToggleSidebar);
    sidebar_->setRandomProvider([this]() { onRandomVerse(); });
    sidebar_->setDailyProvider([this]() { onDailyVerse(); });
}

void MainWindow::loadBibles(const std::string& biblesPath)
{
    statusLabel_->setText("Loading Bibles...");
    auto result = Loader::loadAll(biblesPath);

    if (result.translations.empty()) {
        statusLabel_->setText("Failed to load Bibles");
        QMessageBox::critical(this, "Error",
            "Failed to load any Bible translations.\n"
            "Check that Bibles/kjv.json and Bibles/vulg.json exist.");
        return;
    }

    for (auto& b : result.translations) {
        translationCombo_->addItem(QString::fromStdString(b->shortName()));
        translations_.push_back(std::move(b));
    }

    if (!translations_.empty()) {
        activeBible_ = translations_[0].get();
        searchService_->buildIndex(*activeBible_);
    }

    std::vector<Bible*> ptrs;
    for (const auto& b : translations_) ptrs.push_back(b.get());
    sidebar_->setBibles(ptrs);
    sidebar_->setBookmarkStorage(bookmarkStorage_.get());
    sidebar_->setHistoryStorage(historyStorage_.get());

    if (translations_.size() >= 2) {
        sideBySideReader_->setLeftBible(translations_[0].get());
        sideBySideReader_->setRightBible(translations_[1].get());
    }

    statusLabel_->setText("Ready");
}

void MainWindow::navigateTo(const Reference& ref)
{
    if (!ref.isValid() || !activeBible_) return;

    int existing = findTab(ref);
    if (existing >= 0) {
        tabWidget_->setCurrentIndex(existing);
        auto* pane = qobject_cast<ReaderPane*>(tabWidget_->widget(existing));
        if (pane)
            buildVerseHtml(pane, activeBible_, ref);
        notesPanel_->setCurrentReference(ref);
        addHistory(ref);
        statusLabel_->setText(QString::fromStdString(ref.toString()) + " | "
                              + QString::fromStdString(activeBible_->shortName()));
        if (sideBySideMode_)
            sideBySideReader_->displayReference(ref.book, ref.chapter, ref.verseStart, ref.verseEnd);
        sidebar_->refreshHistory();
        sidebar_->refreshBookmarks();
        return;
    }

    auto* pane = new ReaderPane;
    buildVerseHtml(pane, activeBible_, ref);

    QString title = QString::fromStdString(ref.toString());
    tabWidget_->addTab(pane, title);
    tabWidget_->setCurrentIndex(tabWidget_->count() - 1);

    notesPanel_->setCurrentReference(ref);
    addHistory(ref);

    if (sideBySideMode_) {
        sideBySideReader_->displayReference(ref.book, ref.chapter, ref.verseStart, ref.verseEnd);
    }

    statusLabel_->setText(QString::fromStdString(ref.toString()) + " | "
                          + QString::fromStdString(activeBible_->shortName()));
    sidebar_->refreshHistory();
    sidebar_->refreshBookmarks();
}

void MainWindow::buildVerseHtml(ReaderPane* pane, const Bible* bible,
                                const Reference& ref)
{
    if (!bible) { pane->setVerseText("No Bible loaded."); return; }

    const auto* book = bible->findBook(ref.book);
    if (!book) {
        pane->setVerseText("<div style='color:" + Theme::secondaryText.name().toStdString()
                           + "; padding:20px;'>Book not found: " + ref.book + "</div>");
        return;
    }

    const auto& ch = book->chapter(ref.chapter);
    if (ch.verses.empty()) {
        pane->setVerseText("<div style='color:" + Theme::secondaryText.name().toStdString()
                           + "; padding:20px;'>Chapter " + std::to_string(ref.chapter) + " not found.</div>");
        return;
    }

    int vs = ref.verseStart.value_or(1);
    int ve = ref.verseEnd.value_or(ref.isChapterOnly() ? ch.verses.back().number : vs);

    QString html;
    html += QStringLiteral(
        "<div style='font-size:%1px; line-height:%2; color:%3; "
        "font-family:%4; padding:8px 16px;'>"
    ).arg(14).arg(1.8).arg(Theme::primaryText.name())
     .arg("'JetBrains Mono', 'Cascadia Mono', 'Noto Sans Mono', monospace");

    html += QStringLiteral(
        "<div style='color:%1; font-size:14px; margin-bottom:4px; padding-bottom:2px; "
        "border-bottom:1px solid %2;'>%3 %4</div>"
    ).arg(Theme::primaryText.name(), Theme::borders.name())
     .arg(QString::fromStdString(ref.book)).arg(ref.chapter);

    for (const auto& v : ch.verses) {
        if (v.number < vs || v.number > ve) continue;
        html += QStringLiteral(
            "<span style='color:%1; font-weight:bold; font-size:11px;'>%2</span> "
            "<span>%3</span><br>"
        ).arg(Theme::secondaryText.name()).arg(v.number)
         .arg(QString::fromStdString(v.text));
    }
    html += "</div>";

    pane->setVerseText(html.toStdString());
    pane->displayVerses(ref.book, ref.chapter, vs, ve, bible->shortName());
}

void MainWindow::onNavigate()
{
    std::string s = referenceInput_->text().trimmed().toStdString();
    if (s.empty()) return;
    auto ref = ReferenceParser::parse(s);
    if (ref.has_value()) navigateTo(*ref);
    else statusLabel_->setText("Invalid reference: " + QString::fromStdString(s));
}

void MainWindow::onSearch(const std::string& q)
{
    if (!activeBible_ || q.empty()) return;

    // Try as a reference first (e.g. "John 3:16")
    auto ref = ReferenceParser::parse(q);
    if (ref.has_value()) {
        navigateTo(*ref);
        return;
    }

    // Fall back to full-text search
    auto results = searchService_->search(q, *activeBible_);
    if (results.empty()) {
        statusLabel_->setText(QStringLiteral("No results for \"%1\"").arg(QString::fromStdString(q)));
        return;
    }
    statusLabel_->setText(QStringLiteral("%1 result(s)")
        .arg(static_cast<int>(results.size())));
    Reference r{results[0].bookName, results[0].chapter, results[0].verse, {}};
    navigateTo(r);
}

void MainWindow::onTabClose(int i)
{
    auto* w = tabWidget_->widget(i);
    tabWidget_->removeTab(i);
    delete w;
    if (tabWidget_->count() == 0) notesPanel_->setCurrentReference({});
}

void MainWindow::onRandomVerse()
{
    if (!activeBible_) return;
    auto v = VerseService::randomVerse(*activeBible_);
    if (v.has_value()) {
        sidebar_->displayVerse(*v);
        navigateTo({v->bookName, v->chapter, v->verse, {}});
    }
}

void MainWindow::onDailyVerse()
{
    if (!activeBible_) return;
    auto v = VerseService::dailyVerse(*activeBible_);
    if (v.has_value()) {
        sidebar_->displayVerse(*v);
        navigateTo({v->bookName, v->chapter, v->verse, {}});
    }
}

void MainWindow::onToggleSideBySide()
{
    sideBySideMode_ = !sideBySideMode_;
    sideBySideBtn_->setText(sideBySideMode_ ? "Single" : "Side-by-side");

    if (sideBySideMode_) {
        tabWidget_->hide();
        sideBySideReader_->show();
        if (tabWidget_->currentWidget()) {
            auto* p = qobject_cast<ReaderPane*>(tabWidget_->currentWidget());
            if (p) {
                auto ref = p->currentReference();
                if (ref.isValid())
                    sideBySideReader_->displayReference(ref.book, ref.chapter, ref.verseStart, ref.verseEnd);
            }
        }
    } else {
        sideBySideReader_->hide();
        tabWidget_->show();
    }
}

void MainWindow::onToggleNotes()
{
    notesPanel_->setVisible(!notesPanel_->isVisible());
    notesBtn_->setText(notesPanel_->isVisible() ? "Hide" : "Notes");
    if (notesPanel_->isVisible() && tabWidget_->currentWidget()) {
        auto* p = qobject_cast<ReaderPane*>(tabWidget_->currentWidget());
        if (p) notesPanel_->setCurrentReference(p->currentReference());
    }
}

void MainWindow::onToggleSidebar()
{
    sidebar_->setVisible(!sidebar_->isVisible());
    sidebarBtn_->setText(sidebar_->isVisible() ? "Sidebar" : "Show");
}

void MainWindow::openSettings()
{
    SettingsDialog d(this);
    QSettings s(SETTINGS_ORG, SETTINGS_APP);
    d.setFontFamily(s.value("fontFamily", "JetBrains Mono").toString().toStdString());
    d.setFontSize(s.value("fontSize", 14).toInt());
    d.setLineSpacing(s.value("lineSpacing", 1.8).toDouble());
    d.setRememberLastPassage(s.value("rememberLast", true).toBool());

    if (d.exec() == QDialog::Accepted) {
        s.setValue("fontFamily", QString::fromStdString(d.fontFamily()));
        s.setValue("fontSize", d.fontSize());
        s.setValue("lineSpacing", d.lineSpacing());
        s.setValue("rememberLast", d.rememberLastPassage());
        if (d.clearHistoryRequested()) { historyStorage_->clear(); sidebar_->refreshHistory(); }
        applySettings();
    }
}

void MainWindow::addHistory(const Reference& ref) { historyStorage_->push(ref); }

int MainWindow::findTab(const Reference& ref) const
{
    auto t = QString::fromStdString(ref.toString());
    for (int i = 0; i < tabWidget_->count(); ++i)
        if (tabWidget_->tabText(i) == t) return i;
    return -1;
}

void MainWindow::restoreSettings()
{
    QSettings s(SETTINGS_ORG, SETTINGS_APP);
    if (s.value("rememberLast", true).toBool()) {
        auto last = s.value("lastReference").toString();
        if (!last.isEmpty()) {
            auto ref = ReferenceParser::parse(last.toStdString());
            if (ref.has_value())
                QMetaObject::invokeMethod(this, [this, r = *ref]() { navigateTo(r); }, Qt::QueuedConnection);
        }
    }
}

void MainWindow::saveSettings()
{
    QSettings s(SETTINGS_ORG, SETTINGS_APP);
    if (s.value("rememberLast", true).toBool() && tabWidget_->currentWidget()) {
        auto* p = qobject_cast<ReaderPane*>(tabWidget_->currentWidget());
        if (p) { auto ref = p->currentReference(); if (ref.isValid()) s.setValue("lastReference", QString::fromStdString(ref.toString())); }
    }
}

void MainWindow::applySettings()
{
    Theme::apply();
}
