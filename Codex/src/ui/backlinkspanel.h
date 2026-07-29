#pragma once

#include <QListWidget>
#include <QWidget>
#include <filesystem>
#include <vector>

namespace codex {

class BacklinksPanel : public QListWidget {
    Q_OBJECT
public:
    explicit BacklinksPanel(QWidget *parent = nullptr);

    void showBacklinks(const std::vector<std::filesystem::path> &paths);
    void clear();

signals:
    void backlinkClicked(const std::filesystem::path &path);

private:
    void onItemClicked(QListWidgetItem *item);
};

} // namespace codex
