#pragma once

#include "core/Reference.h"

#include <QPlainTextEdit>
#include <QPushButton>
#include <QWidget>
#include <QLabel>

#include <string>
#include <string_view>

class NoteStorage;

class NotesPanel : public QWidget {
    Q_OBJECT
public:
    explicit NotesPanel(QWidget* parent = nullptr);

    void setStorage(NoteStorage* storage);
    void setCurrentReference(const Reference& ref);

signals:
    void noteChanged(const std::string& refKey);

private slots:
    void saveNote();

private:
    void setupUI();
    void loadNote();

    NoteStorage* storage_ = nullptr;
    Reference currentRef_;
    QLabel* header_;
    QPlainTextEdit* editor_;
    QPushButton* saveBtn_;
    QLabel* emptyLabel_;
};
