#pragma once

#include "data/PrayerLibrary.h"

#include <QWidget>
#include <QString>

class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QSplitter;
class QTextBrowser;
class QComboBox;
class QPushButton;
class QLabel;
class QScrollBar;

// Prayer Mode: searchable prayer library rendered Latin | English side by side.
class PrayerModeWidget : public QWidget {
    Q_OBJECT
public:
    explicit PrayerModeWidget(QWidget* parent = nullptr);

    void setLibrary(PrayerLibrary* library);

    [[nodiscard]] int currentIndex() const;
    void showPrayer(int index);
    void selectByText(const QString& text);

public slots:
    void refresh();

signals:
    void prayerSelected(int index);
    void copyAllRequested(const QString& text);

private slots:
    void onSearchChanged(const QString& text);
    void onCategoryChanged(const QString& category);
    void onListSelectionChanged();
    void onPrev();
    void onNext();
    void onCopy();
    void onFontLarger();
    void onFontSmaller();
    void onNew();
    void onEdit();
    void onDelete();

private:
    void rebuildList();
    void updateReader();
    void updateStatus();
    void syncScroll(QScrollBar* from, QScrollBar* to);
    static int findRowById(const QVector<Prayer>& list, const QString& id);

    PrayerLibrary* library_ = nullptr;
    QVector<Prayer> currentList_;

    QLineEdit* searchBox_;
    QComboBox* categoryCombo_;
    QListWidget* list_;
    QTextBrowser* latinView_;
    QTextBrowser* englishView_;
    QSplitter* readerSplitter_;
    QLabel* titleLabel_;
    QLabel* statusLabel_;
    QPushButton* prevBtn_;
    QPushButton* nextBtn_;
    QPushButton* copyBtn_;
    QPushButton* newBtn_;
    QPushButton* editBtn_;
    QPushButton* deleteBtn_;

    bool syncingScroll_ = false;
    int baseFontSize_ = 16;
};
