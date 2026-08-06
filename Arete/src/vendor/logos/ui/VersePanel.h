#pragma once

#include "core/SearchResult.h"

#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include <functional>

class VersePanel : public QWidget {
    Q_OBJECT
public:
    explicit VersePanel(QWidget* parent = nullptr);

    using VerseProvider = std::function<void()>;

    void setRandomProvider(VerseProvider provider);
    void setDailyProvider(VerseProvider provider);
    void displayVerse(const SearchResult& verse);

signals:
    void verseClicked(const std::string& book, int chapter, int verse);

private:
    void setupUI();

    QPushButton* randomBtn_;
    QPushButton* dailyBtn_;
    QLabel* verseLabel_;
    VerseProvider randomProvider_;
    VerseProvider dailyProvider_;
    SearchResult currentVerse_;
};
