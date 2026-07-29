#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

#include "models/Task.h"

namespace chronos {

class SessionCompleteDialog : public QDialog {
    Q_OBJECT

public:
    struct Result {
        enum Action { ProceedToBreak, SkipBreak };
        Action action = ProceedToBreak;
        bool markTaskCompleted = false;
        QString note;
    };

    explicit SessionCompleteDialog(int durationMinutes,
                                   const QString& taskTitle,
                                   QWidget* parent = nullptr);

    Result result() const;

private:
    QLineEdit* m_noteEdit = nullptr;
    QPushButton* m_proceedBtn = nullptr;
    QPushButton* m_skipBtn = nullptr;
    QPushButton* m_markDoneBtn = nullptr;
    Result m_result;
};

} // namespace chronos
