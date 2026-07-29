#pragma once

#include <QLabel>
#include <QWidget>
#include <string>

namespace codex {

class FileInfoPanel : public QWidget {
    Q_OBJECT
public:
    explicit FileInfoPanel(QWidget *parent = nullptr);

    void update(int line, int col, int words, int chars, const std::string &filename);
    void clear();

private:
    QLabel *m_fileLabel;
    QLabel *m_posLabel;
    QLabel *m_wordCountLabel;
};

} // namespace codex
