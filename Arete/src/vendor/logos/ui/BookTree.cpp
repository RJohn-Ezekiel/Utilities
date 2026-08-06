#include "BookTree.h"
#include "Theme.h"
#include "core/Bible.h"

#include <QHeaderView>

BookTree::BookTree(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderHidden(true);
    setRootIsDecorated(true);
    setAnimated(false);
    setIndentation(16);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setupStyle();

    connect(this, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int) {
        if (!item || !item->parent())
            return;
        // Child item (chapter) was clicked
        auto* parentItem = item->parent();
        std::string bookName = parentItem->text(0).toStdString();
        bool ok = false;
        int chapter = item->text(0).toInt(&ok);
        if (ok) {
            Reference ref;
            ref.book = bookName;
            ref.chapter = chapter;
            emit chapterSelected(ref);
        }
    });

    connect(this, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int) {
        if (!item || item->parent())
            return;
        // Top-level item (book) was clicked
        emit bookSelected(item->text(0).toStdString());
    });
}

void BookTree::setupStyle()
{
    setStyleSheet(QStringLiteral(
        "QTreeWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  font-size: 13px;"
        "}"
        "QTreeWidget::item {"
        "  padding: 2px 4px;"
        "}"
        "QTreeWidget::item:selected {"
        "  background-color: %3;"
        "}"
        "QTreeWidget::item:hover {"
        "  background-color: %4;"
        "}"
    ).arg(Theme::sidebar.name(), Theme::primaryText.name(),
          Theme::selected.name(), Theme::hover.name()));
}

void BookTree::populate(const Bible* bible)
{
    clear();
    bible_ = bible;
    if (!bible)
        return;

    for (const auto& book : bible->books()) {
        auto* bookItem = new QTreeWidgetItem(this);
        bookItem->setText(0, QString::fromStdString(book.name));
        bookItem->setFlags(bookItem->flags() & ~Qt::ItemIsSelectable);

        for (const auto& ch : book.chapters) {
            auto* chItem = new QTreeWidgetItem(bookItem);
            chItem->setText(0, QString::number(ch.number));
            chItem->setSizeHint(0, QSize(0, 22));
        }
    }
}
