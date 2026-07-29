#pragma once

#include <QWidget>
#include <QTableWidget>

#include "storage/StorageManager.h"

namespace chronos {

class HistoryWidget : public QWidget {
    Q_OBJECT

public:
    explicit HistoryWidget(StorageManager* storage, QWidget* parent = nullptr);

    void refresh();

private:
    void setupUi();

    StorageManager* m_storage = nullptr;
    QTableWidget* m_table = nullptr;
};

} // namespace chronos
