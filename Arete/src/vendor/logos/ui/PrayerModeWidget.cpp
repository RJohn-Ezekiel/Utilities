#include "PrayerModeWidget.h"
#include "PrayerEditDialog.h"
#include "ui/Theme.h"

#include <QComboBox>
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
QString htmlFor(const QString& text, int fontSize)
{
    // Plain multi-line text -> simple HTML with a highlighted first line.
    QStringList paragraphs;
    const QStringList lines = text.split(QLatin1Char('\n'));
    bool first = true;
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) {
            paragraphs.append(QStringLiteral("<br>"));
            continue;
        }
        if (first) {
            paragraphs.append(QStringLiteral("<p class=\"lead\">%1</p>")
                                  .arg(line.toHtmlEscaped()));
            first = false;
        } else {
            paragraphs.append(QStringLiteral("<p>%1</p>").arg(line.toHtmlEscaped()));
        }
    }
    return QStringLiteral("<html><head><style>"
                          "body { color: %1; font-size: %2px; line-height: 1.7; font-family: 'Gentium', serif; }"
                          "p { margin: 0.4em 0; }"
                          "p.lead { color: %3; font-weight: 600; }"
                          "</style></head><body>%4</body></html>")
        .arg(Theme::secondaryText.name(), QString::number(fontSize),
             Theme::primaryText.name(), paragraphs.join(QString()));
}
} // namespace

PrayerModeWidget::PrayerModeWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // Toolbar: search, category, navigation, copy.
    auto* bar = new QHBoxLayout;
    searchBox_ = new QLineEdit;
    searchBox_->setPlaceholderText(QStringLiteral("Search prayers (Latin or English)..."));
    categoryCombo_ = new QComboBox;
    categoryCombo_->addItem(QStringLiteral("All"));
    prevBtn_ = new QPushButton(QStringLiteral("◀"));
    prevBtn_->setToolTip(QStringLiteral("Previous prayer"));
    nextBtn_ = new QPushButton(QStringLiteral("▶"));
    nextBtn_->setToolTip(QStringLiteral("Next prayer"));
    copyBtn_ = new QPushButton(QStringLiteral("Copy"));
    copyBtn_->setToolTip(QStringLiteral("Copy both columns to clipboard"));
    newBtn_ = new QPushButton(QStringLiteral("+ New"));
    newBtn_->setToolTip(QStringLiteral("Add a new prayer"));
    editBtn_ = new QPushButton(QStringLiteral("Edit…"));
    editBtn_->setToolTip(QStringLiteral("Modify the selected prayer"));
    deleteBtn_ = new QPushButton(QStringLiteral("Delete"));
    deleteBtn_->setToolTip(QStringLiteral("Remove the selected prayer"));

    bar->addWidget(searchBox_, 1);
    bar->addWidget(categoryCombo_);
    bar->addWidget(prevBtn_);
    bar->addWidget(nextBtn_);
    bar->addWidget(copyBtn_);
    bar->addWidget(newBtn_);
    bar->addWidget(editBtn_);
    bar->addWidget(deleteBtn_);
    root->addLayout(bar);

    // Title line.
    titleLabel_ = new QLabel;
    titleLabel_->setStyleSheet(QStringLiteral("color: %1; font-size: 15px; font-weight: 600;")
                                   .arg(Theme::primaryText.name()));
    root->addWidget(titleLabel_);

    // Body: list | reader.
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

    statusLabel_ = new QLabel;
    statusLabel_->setStyleSheet(QStringLiteral("color: %1;").arg(Theme::secondaryText.name()));
    root->addWidget(statusLabel_);

    connect(searchBox_, &QLineEdit::textChanged, this, &PrayerModeWidget::onSearchChanged);
    connect(categoryCombo_, &QComboBox::currentTextChanged, this, &PrayerModeWidget::onCategoryChanged);
    connect(list_, &QListWidget::itemSelectionChanged, this, &PrayerModeWidget::onListSelectionChanged);
    connect(prevBtn_, &QPushButton::clicked, this, &PrayerModeWidget::onPrev);
    connect(nextBtn_, &QPushButton::clicked, this, &PrayerModeWidget::onNext);
    connect(copyBtn_, &QPushButton::clicked, this, &PrayerModeWidget::onCopy);
    connect(newBtn_, &QPushButton::clicked, this, &PrayerModeWidget::onNew);
    connect(editBtn_, &QPushButton::clicked, this, &PrayerModeWidget::onEdit);
    connect(deleteBtn_, &QPushButton::clicked, this, &PrayerModeWidget::onDelete);
}

void PrayerModeWidget::setLibrary(PrayerLibrary* library)
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

void PrayerModeWidget::refresh()
{
    if (!library_) return;

    categoryCombo_->blockSignals(true);
    const QString current = categoryCombo_->currentText();
    categoryCombo_->clear();
    categoryCombo_->addItem(QStringLiteral("All"));
    categoryCombo_->addItems(library_->categories());
    const int idx = categoryCombo_->findText(current);
    categoryCombo_->setCurrentIndex(idx > 0 ? idx : 0);
    categoryCombo_->blockSignals(false);

    onSearchChanged(searchBox_->text());
}

int PrayerModeWidget::currentIndex() const
{
    return list_->currentRow();
}

void PrayerModeWidget::showPrayer(int index)
{
    if (index < 0 || index >= currentList_.size()) return;
    list_->setCurrentRow(index);
}

void PrayerModeWidget::selectByText(const QString& text)
{
    const QString q = text.trimmed().toLower();
    if (q.isEmpty()) return;
    for (int i = 0; i < currentList_.size(); ++i) {
        const Prayer& p = currentList_.at(i);
        if (p.title.toLower().contains(q) || p.latin.toLower().contains(q) ||
            p.english.toLower().contains(q)) {
            list_->setCurrentRow(i);
            return;
        }
    }
}

void PrayerModeWidget::onSearchChanged(const QString&)
{
    if (!library_) return;
    currentList_ = library_->search(searchBox_->text());
    rebuildList();
}

void PrayerModeWidget::onCategoryChanged(const QString& category)
{
    if (!library_) return;
    if (category == QStringLiteral("All")) {
        currentList_ = library_->search(searchBox_->text());
    } else {
        currentList_ = library_->byCategory(category);
    }
    rebuildList();
}

void PrayerModeWidget::rebuildList()
{
    list_->blockSignals(true);
    list_->clear();
    for (const Prayer& p : currentList_) {
        auto* item = new QListWidgetItem(p.title.isEmpty() ? p.category : p.title);
        item->setData(Qt::UserRole, p.id);
        list_->addItem(item);
    }
    list_->blockSignals(false);
    updateStatus();
    if (list_->count() > 0) {
        list_->setCurrentRow(0);
    } else {
        latinView_->setHtml(QStringLiteral("<p style='color:%1'>No prayers match.</p>").arg(Theme::secondaryText.name()));
        englishView_->setHtml(QString());
        titleLabel_->clear();
    }
}

void PrayerModeWidget::onListSelectionChanged()
{
    updateReader();
    updateStatus();
    emit prayerSelected(list_->currentRow());
}

void PrayerModeWidget::updateReader()
{
    const int row = list_->currentRow();
    if (row < 0 || row >= currentList_.size()) {
        latinView_->setHtml(QString());
        englishView_->setHtml(QString());
        titleLabel_->clear();
        return;
    }

    const Prayer& p = currentList_.at(row);
    QString title = p.title;
    if (!p.category.isEmpty()) title += QStringLiteral("  —  ") + p.category;
    titleLabel_->setText(title);

    latinView_->setHtml(htmlFor(p.latin, baseFontSize_));
    englishView_->setHtml(htmlFor(p.english, baseFontSize_));
}

void PrayerModeWidget::updateStatus()
{
    const int row = list_->currentRow();
    QString text;
    if (row >= 0 && !currentList_.isEmpty()) {
        const Prayer& p = currentList_.at(row);
        text = QStringLiteral("%1 / %2").arg(row + 1).arg(currentList_.size());
        QString meta;
        if (!p.author.isEmpty()) meta += QStringLiteral(" · ") + p.author;
        if (!p.source.isEmpty()) meta += QStringLiteral(" · ") + p.source;
        text += meta;
        if (!p.tags.isEmpty()) text += QStringLiteral("  [") + p.tags.join(QStringLiteral(", ")) + QStringLiteral("]");
    } else {
        text = QStringLiteral("%1 prayers").arg(currentList_.size());
    }
    statusLabel_->setText(text);
}

void PrayerModeWidget::onPrev()
{
    const int row = list_->currentRow();
    if (row > 0) list_->setCurrentRow(row - 1);
}

void PrayerModeWidget::onNext()
{
    const int row = list_->currentRow();
    if (row >= 0 && row + 1 < list_->count()) list_->setCurrentRow(row + 1);
}

void PrayerModeWidget::onCopy()
{
    const int row = list_->currentRow();
    if (row < 0 || row >= currentList_.size()) return;
    const Prayer& p = currentList_.at(row);
    QString text;
    if (!p.latin.trimmed().isEmpty()) {
        text += p.title + QStringLiteral("\n\n") + p.latin;
    }
    if (!p.english.trimmed().isEmpty()) {
        if (!text.isEmpty()) text += QStringLiteral("\n\n---\n\n");
        text += p.english;
    }
    QApplication::clipboard()->setText(text);
    emit copyAllRequested(text);
}

void PrayerModeWidget::onFontLarger()
{
    baseFontSize_ = std::min(baseFontSize_ + 1, 28);
    updateReader();
}

void PrayerModeWidget::onFontSmaller()
{
    baseFontSize_ = std::max(baseFontSize_ - 1, 10);
    updateReader();
}

void PrayerModeWidget::onNew()
{
    if (!library_) return;
    const Prayer created = PrayerEditDialog::newPrayer(this);
    if (!created.isValid()) return;
    const QString id = created.id;
    if (!library_->addPrayer(created)) {
        QMessageBox::warning(this, QStringLiteral("Could not save"),
                             QStringLiteral("Failed to write %1\n%2")
                                 .arg(library_->filePath(), library_->lastError()));
    }
    const int keep = currentIndex();
    refresh();
    const int row = findRowById(currentList_, id);
    list_->setCurrentRow(row >= 0 ? row : keep);
}

void PrayerModeWidget::onEdit()
{
    if (!library_) return;
    const int row = list_->currentRow();
    if (row < 0 || row >= currentList_.size()) return;
    const Prayer edited = PrayerEditDialog::editPrayer(this, currentList_.at(row));
    if (!edited.isValid()) return;
    if (!library_->updatePrayer(edited)) {
        QMessageBox::warning(this, QStringLiteral("Could not save"),
                             QStringLiteral("Failed to write %1\n%2")
                                 .arg(library_->filePath(), library_->lastError()));
    }
    const int keep = list_->currentRow();
    refresh();
    const int newRow = findRowById(currentList_, edited.id);
    list_->setCurrentRow(newRow >= 0 ? newRow : keep);
}

void PrayerModeWidget::onDelete()
{
    if (!library_) return;
    const int row = list_->currentRow();
    if (row < 0 || row >= currentList_.size()) return;
    const Prayer& p = currentList_.at(row);
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, QStringLiteral("Delete prayer"),
        QStringLiteral("Remove \u201C%1\u201D from the prayer library?").arg(p.title),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) return;
    library_->removePrayer(p.id);
    const int keep = std::min(row, list_->count() - 1);
    refresh();
    list_->setCurrentRow(keep);
}

void PrayerModeWidget::syncScroll(QScrollBar* from, QScrollBar* to)
{
    if (syncingScroll_) return;
    if (!from || !to || from->maximum() <= 0) return;
    syncingScroll_ = true;
    const int target = qRound(double(from->value()) * double(to->maximum()) / double(from->maximum()));
    to->setValue(std::min(target, to->maximum()));
    syncingScroll_ = false;
}

int PrayerModeWidget::findRowById(const QVector<Prayer>& list, const QString& id)
{
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i).id == id) return i;
    }
    return -1;
}
