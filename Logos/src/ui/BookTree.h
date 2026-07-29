#pragma once

#include "core/Reference.h"

#include <QTreeWidget>
#include <QVector>

class Bible;

class BookTree : public QTreeWidget {
    Q_OBJECT
public:
    explicit BookTree(QWidget* parent = nullptr);

    void populate(const Bible* bible);

signals:
    void bookSelected(const std::string& bookName);
    void chapterSelected(const Reference& ref);

private:
    void setupStyle();
    const Bible* bible_ = nullptr;
};
