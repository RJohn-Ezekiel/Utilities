#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <filesystem>
#include <vector>
#include <string>

namespace codex {

class TagsPanel : public QWidget {
    Q_OBJECT
public:
    explicit TagsPanel(QWidget *parent = nullptr);

    void showTags(const std::vector<std::string> &tags);
    void clear();

signals:
    void tagClicked(const std::string &tag);
    void tagAddRequested(const std::string &tag);

private:
    QVBoxLayout *m_layout;
    QLineEdit *m_tagInput;
    QWidget *m_tagContainer;
    QVBoxLayout *m_tagLayout;
    void onTagClick();
    void onAddTag();
};

} // namespace codex
