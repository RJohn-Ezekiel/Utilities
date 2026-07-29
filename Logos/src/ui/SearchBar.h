#pragma once

#include <QLineEdit>
#include <QWidget>

#include <functional>
#include <string>

class SearchBar : public QWidget {
    Q_OBJECT
public:
    explicit SearchBar(QWidget* parent = nullptr);

    void setSearchCallback(std::function<void(std::string)> callback);
    void clear();

signals:
    void searchRequested(const std::string& query);

private:
    void setupUI();

    QLineEdit* input_;
    std::function<void(std::string)> callback_;
};
