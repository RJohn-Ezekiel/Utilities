#pragma once

#include <QLineEdit>
#include <QTimer>
#include <QWidget>
#include <filesystem>

namespace codex {

class SearchBar : public QLineEdit {
    Q_OBJECT
public:
    explicit SearchBar(QWidget *parent = nullptr);

signals:
    void searchRequested(const std::string &query);

private slots:
    void onTextChanged(const QString &text);

private:
    QTimer *m_debounceTimer;
};

} // namespace codex
