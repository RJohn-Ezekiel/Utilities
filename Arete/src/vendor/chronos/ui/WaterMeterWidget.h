#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>

namespace chronos {

class StorageManager;

class WaterMeterWidget : public QWidget {
    Q_OBJECT

public:
    explicit WaterMeterWidget(StorageManager* storage, QWidget* parent = nullptr);

    void refresh();

signals:
    void waterUpdated();

private:
    void addGlass();
    void removeGlass();

    StorageManager* m_storage;
    QLabel* m_descLabel;
    QLabel* m_countLabel;
    QLabel* m_unitLabel;
    QPushButton* m_minusBtn;
    QPushButton* m_plusBtn;
    int m_glasses = 0;
};

} // namespace chronos
