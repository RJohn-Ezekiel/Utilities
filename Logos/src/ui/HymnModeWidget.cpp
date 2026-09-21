#include "HymnModeWidget.h"
#include "HymnEditDialog.h"
#include "ui/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <QApplication>
#include <QClipboard>
#include <QScrollBar>

#include <algorithm>

namespace {
QString hymnHtml(const QString& text, int fontSize)
{
    QStringList out;
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) {
            out.append(QStringLiteral("<br>"));
        } else {
            out.append(QStringLiteral("<p>%1</p>").arg(line.toHtmlEscaped()));
        }
    }
    return QStringLiteral("<html><head><style>"
                          "body { color: %1; font-size: %2px; line-height: 1.8; font-family: 'Gentium', serif; }"
                          "p { margin: 0.3em 0; }"
                          "</style></head><body>%3</body></html>")
        .arg(Theme::secondaryText.name(), QString::number(fontSize), out.join(QString()));
}
} // namespace

HymnModeWidget::HymnModeWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto* bar = new QHBoxLayout;
    searchBox_ = new QLineEdit;
    searchBox_->setPlaceholderText(QStringLiteral("Search hymns (Latin or English)..."));
    prevBtn_ = new QPushButton(QStringLiteral("◀"));
    prevBtn_->setToolTip(QStringLiteral("Previous hymn"));
    nextBtn_ = new QPushButton(QStringLiteral("▶"));
    nextBtn_->setToolTip(QStringLiteral("Next hymn"));
    favBtn_ = new QPushButton(QStringLiteral("★"));
    favBtn_->setToolTip(QStringLiteral("Toggle favourite"));
    copyBtn_ = new QPushButton(QStringLiteral("Copy"));
    copyBtn_->setToolTip(QStringLiteral("Copy both columns to clipboard"));
    newBtn_ = new QPushButton(QStringLiteral("+ New"));
    newBtn_->setToolTip(QStringLiteral("Add a new hymn"));
    editBtn_ = new QPushButton(QStringLiteral("Edit…"));
    editBtn_->setToolTip(QStringLiteral("Modify the selected hymn"));
    deleteBtn_ = new QPushButton(QStringLiteral("Delete"));
    deleteBtn_->setToolTip(QStringLiteral("Remove the selected hymn"));

    bar->addWidget(searchBox_, 1);
    bar->addWidget(prevBtn_);
    bar->addWidget(nextBtn_);
    bar->addWidget(favBtn_);
    bar->addWidget(copyBtn_);
    bar->addWidget(newBtn_);
    bar->addWidget(editBtn_);
    bar->addWidget(deleteBtn_);
    root->addLayout(bar);

    titleLabel_ = new QLabel;
    titleLabel_->setStyleSheet(QStringLiteral("color: %1; font-size: 15px; font-weight: 600;")
                                   .arg(Theme::primaryText.name()));
    metaLabel_ = new QLabel;
    metaLabel_->setStyleSheet(QStringLiteral("color: %1; font-size: 12px;")
                                  .arg(Theme::secondaryText.name()));
    root->addWidget(titleLabel_);
    root->addWidget(metaLabel_);

    auto* body = new QSplitter(Qt::Horizontal);
    list_ = new QListWidget;
    list_->setMinimumWidth(220);
    list_->setMaximumWidth(380);
    list_->setStyleSheet(QStringLiteral("QListWidget { background: %1; border: 1px solid %2; }")
                             .arg(Theme::sidebar.name(), Theme::borders.name()));

    latinView_ = new QTextBrowser;
    englishView_ = new QTextBrowser;
    latinView_->setStyleSheet(QStringLiteral("QTextBrowser { background: %1; border: 1px solid %2; }")
                                  .arg(Theme::sidebar.name(), Theme::borders.name()));
    englishView_->setStyleSheet(QStringLiteral("QTextBrowser { background: %1; border: 1px solid %2; }")
                                    .arg(Theme::sidebar.name(), Theme::borders.name()));

    readerSplitter_ = new QSplitter(Qt::Horizontal);
    readerSplitter_->addWidget(latinView_);
    readerSplitter_->addWidget(englishView_);
    readerSplitter_->setStretchFactor(0, 1);
    readerSplitter_->setStretchFactor(1, 1);

    body->addWidget(list_);
    body->addWidget(readerSplitter_);
    body->setStretchFactor(0, 0);
    body->setStretchFactor(1, 1);
    root->addWidget(body, 1);

    // Scroll the Latin and English panes together (proportional, so shorter
    // text stays in sync with longer text). A guard prevents the two-way
    // connection from feeding back into itself and jittering/sticking.
    auto* latinBar = latinView_->verticalScrollBar();
    auto* englishBar = englishView_->verticalScrollBar();
    connect(latinBar, &QScrollBar::valueChanged, this,
            [this, latinBar, englishBar](int) { syncScroll(latinBar, englishBar); });
    connect(englishBar, &QScrollBar::valueChanged, this,
            [this, latinBar, englishBar](int) { syncScroll(englishBar, latinBar); });

    connect(searchBox_, &QLineEdit::textChanged, this, &HymnModeWidget::onSearchChanged);
    connect(list_, &QListWidget::itemSelectionChanged, this, &HymnModeWidget::onListSelectionChanged);
    connect(prevBtn_, &QPushButton::clicked, this, &HymnModeWidget::onPrev);
    connect(nextBtn_, &QPushButton::clicked, this, &HymnModeWidget::onNext);
    connect(favBtn_, &QPushButton::clicked, this, &HymnModeWidget::onToggleFavourite);
    connect(copyBtn_, &QPushButton::clicked, this, &HymnModeWidget::onCopy);
    connect(newBtn_, &QPushButton::clicked, this, &HymnModeWidget::onNew);
    connect(editBtn_, &QPushButton::clicked, this, &HymnModeWidget::onEdit);
    connect(deleteBtn_, &QPushButton::clicked, this, &HymnModeWidget::onDelete);
}

void HymnModeWidget::setLibrary(HymnLibrary* library)
{
    library_ = library;
    if (library_) {
        library_->setReloadCallback([this]() {
            const int keep = currentIndex();
            refresh();
            if (keep >= 0 && keep < list_->count()) {
                list_->setCurrentRow(keep);
            }
        });
    }
    refresh();
}

void HymnModeWidget::setFavouriteStorage(std::function<bool(const QString&)> isFavourite,
                                         std::function<void(const QString&, bool)> setFavourite)
{
    isFavourite_ = std::move(isFavourite);
    setFavourite_ = std::move(setFavourite);
}

void HymnModeWidget::refresh()
{
    if (!library_) return;
    onSearchChanged(searchBox_->text());
}

int HymnModeWidget::currentIndex() const
{
    return list_->currentRow();
}

void HymnModeWidget::showHymn(int index)
{
    if (index < 0 || index >= currentList_.size()) return;
    list_->setCurrentRow(index);
}

void HymnModeWidget::selectByText(const QString& text)
{
    const QString q = text.trimmed().toLower();
    if (q.isEmpty()) return;
    for (int i = 0; i < currentList_.size(); ++i) {
        const Hymn& h = currentList_.at(i);
        if (h.title.toLower().contains(q) || h.latin.toLower().contains(q) ||
            h.english.toLower().contains(q) ||
            (h.number > 0 && q == QString::number(h.number))) {
            list_->setCurrentRow(i);
            return;
        }
    }
}

void HymnModeWidget::onSearchChanged(const QString&)
{
    if (!library_) return;
    currentList_ = library_->search(searchBox_->text());
    rebuildList();
}

void HymnModeWidget::rebuildList()
{
    list_->blockSignals(true);
    list_->clear();
    for (const Hymn& h : currentList_) {
        QString label = h.displayTitle();
        if (h.number > 0) label = QStringLiteral("%1. %2").arg(h.number).arg(label);
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, h.id);
        if (isFavourite_ && isFavourite_(h.id)) {
            item->setText(QStringLiteral("★ ") + item->text());
        }
        list_->addItem(item);
    }
    list_->blockSignals(false);
    updateStatus();
    if (list_->count() > 0) {
        list_->setCurrentRow(0);
    } else {
        latinView_->setHtml(QStringLiteral("<p style='color:%1'>No hymns match.</p>").arg(Theme::secondaryText.name()));
        englishView_->setHtml(QString());
        titleLabel_->clear();
        metaLabel_->clear();
    }
}

void HymnModeWidget::onListSelectionChanged()
{
    updateReader();
    updateStatus();
    emit hymnSelected(list_->currentRow());
}

void HymnModeWidget::updateReader()
{
    const int row = list_->currentRow();
    if (row < 0 || row >= currentList_.size()) {
        latinView_->setHtml(QString());
        englishView_->setHtml(QString());
        titleLabel_->clear();
        metaLabel_->clear();
        return;
    }

    const Hymn& h = currentList_.at(row);
    titleLabel_->setText(h.displayTitle());

    QStringList meta;
    if (!h.tune.isEmpty()) meta.append(QStringLiteral("Tune: %1").arg(h.tune));
    if (!h.author.isEmpty()) meta.append(QStringLiteral("Author: %1").arg(h.author));
    if (!h.composer.isEmpty()) meta.append(QStringLiteral("Composer: %1").arg(h.composer));
    if (!h.category.isEmpty()) meta.append(h.category);
    metaLabel_->setText(meta.join(QStringLiteral("   ·   ")));

    latinView_->setHtml(hymnHtml(h.latin, baseFontSize_));
    englishView_->setHtml(hymnHtml(h.english, baseFontSize_));
}

void HymnModeWidget::updateStatus()
{
    const int row = list_->currentRow();
    QString text;
    if (row >= 0 && !currentList_.isEmpty()) {
        text = QStringLiteral("%1 / %2").arg(row + 1).arg(currentList_.size());
    } else {
        text = QStringLiteral("%1 hymns").arg(currentList_.size());
    }
    // Favourite state on the toggle button.
    const bool fav = row >= 0 && isFavourite_ && isFavourite_(currentList_.at(row).id);
    favBtn_->setStyleSheet(fav ? QStringLiteral("color: %1;").arg(Theme::accent().name())
                               : QStringLiteral("color: %1;").arg(Theme::secondaryText.name()));
}

void HymnModeWidget::onPrev()
{
    const int row = list_->currentRow();
    if (row > 0) list_->setCurrentRow(row - 1);
}

void HymnModeWidget::onNext()
{
    const int row = list_->currentRow();
    if (row >= 0 && row + 1 < list_->count()) list_->setCurrentRow(row + 1);
}

void HymnModeWidget::onToggleFavourite()
{
    const int row = list_->currentRow();
    if (row < 0 || row >= currentList_.size() || !setFavourite_) return;
    const Hymn& h = currentList_.at(row);
    const bool now = !(isFavourite_ && isFavourite_(h.id));
    setFavourite_(h.id, now);
    rebuildList();
    list_->setCurrentRow(row);
    updateStatus();
}

void HymnModeWidget::onCopy()
{
    const int row = list_->currentRow();
    if (row < 0 || row >= currentList_.size()) return;
    const Hymn& h = currentList_.at(row);
    QString text = h.displayTitle();
    if (!h.latin.trimmed().isEmpty()) text += QStringLiteral("\n\n") + h.latin;
    if (!h.english.trimmed().isEmpty()) text += QStringLiteral("\n\n---\n\n") + h.english;
    QApplication::clipboard()->setText(text);
}

void HymnModeWidget::onFontLarger()
{
    baseFontSize_ = std::min(baseFontSize_ + 1, 28);
    updateReader();
}

void HymnModeWidget::onFontSmaller()
{
    baseFontSize_ = std::max(baseFontSize_ - 1, 10);
    updateReader();
}

void HymnModeWidget::onNew()
{
    if (!library_) return;
    const Hymn created = HymnEditDialog::newHymn(this);
    if (!created.isValid()) return;
    const QString id = created.id;
    if (!library_->addHymn(created)) {
        QMessageBox::warning(this, QStringLiteral("Could not save"),
                             QStringLiteral("Failed to write %1\n%2")
                                 .arg(library_->filePath(), library_->lastError()));
    }
    const int keep = currentIndex();
    refresh();
    const int row = selectedRowById(currentList_, id);
    list_->setCurrentRow(row >= 0 ? row : keep);
    updateStatus();
}

void HymnModeWidget::onEdit()
{
    if (!library_) return;
    const int row = list_->currentRow();
    if (row < 0 || row >= currentList_.size()) return;
    const Hymn edited = HymnEditDialog::editHymn(this, currentList_.at(row));
    if (!edited.isValid()) return;
    if (!library_->updateHymn(edited)) {
        QMessageBox::warning(this, QStringLiteral("Could not save"),
                             QStringLiteral("Failed to write %1\n%2")
                                 .arg(library_->filePath(), library_->lastError()));
    }
    const int keep = list_->currentRow();
    refresh();
    const int newRow = selectedRowById(currentList_, edited.id);
    list_->setCurrentRow(newRow >= 0 ? newRow : keep);
    updateStatus();
}

void HymnModeWidget::onDelete()
{
    if (!library_) return;
    const int row = list_->currentRow();
    if (row < 0 || row >= currentList_.size()) return;
    const Hymn& h = currentList_.at(row);
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, QStringLiteral("Delete hymn"),
        QStringLiteral("Remove \u201C%1\u201D from the hymn library?").arg(h.displayTitle()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    library_->removeHymn(h.id);
    const int keep = std::min(row, list_->count() - 1);
    refresh();
    list_->setCurrentRow(keep);
    updateStatus();
}

void HymnModeWidget::syncScroll(QScrollBar* from, QScrollBar* to)
{
    if (syncingScroll_) return;
    if (!from || !to || from->maximum() <= 0) return;
    syncingScroll_ = true;
    const int target = qRound(double(from->value()) * double(to->maximum()) / double(from->maximum()));
    to->setValue(std::min(target, to->maximum()));
    syncingScroll_ = false;
}

int HymnModeWidget::selectedRowById(const QVector<Hymn>& list, const QString& id)
{
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i).id == id) return i;
    }
    return -1;
}
