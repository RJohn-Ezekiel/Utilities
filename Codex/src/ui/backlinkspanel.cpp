#include "ui/backlinkspanel.h"

namespace codex {

BacklinksPanel::BacklinksPanel(QWidget *parent)
    : QListWidget(parent)
{
    connect(this, &QListWidget::itemClicked, this, &BacklinksPanel::onItemClicked);
}

void BacklinksPanel::showBacklinks(const std::vector<std::filesystem::path> &paths)
{
    clear();
    for (const auto &p : paths) {
        auto display = QString::fromStdString(p.stem().string());
        auto *item = new QListWidgetItem(display, this);
        item->setData(Qt::UserRole, QString::fromStdString(p.string()));
    }

    if (paths.empty()) {
        auto *item = new QListWidgetItem(QStringLiteral("(none)"), this);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    }
}

void BacklinksPanel::clear()
{
    QListWidget::clear();
}

void BacklinksPanel::onItemClicked(QListWidgetItem *item)
{
    auto pathStr = item->data(Qt::UserRole).toString();
    if (!pathStr.isEmpty())
        emit backlinkClicked(std::filesystem::path(pathStr.toStdString()));
}

} // namespace codex
