#pragma once

#include "data/PrayerLibrary.h"

#include <QDialog>

class QLineEdit;
class QPlainTextEdit;

// Add / edit form for a single prayer. Returns the edited prayer via
// `result()` when accepted; the id is preserved so updates target the
// same entry.
class PrayerEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit PrayerEditDialog(QWidget* parent = nullptr);

    void setPrayer(const Prayer& prayer);
    [[nodiscard]] Prayer prayer() const;

    static Prayer editPrayer(QWidget* parent, const Prayer& prayer);
    static Prayer newPrayer(QWidget* parent);

private:
    QLineEdit* titleEdit_;
    QLineEdit* categoryEdit_;
    QLineEdit* authorEdit_;
    QLineEdit* sourceEdit_;
    QLineEdit* tagsEdit_;
    QPlainTextEdit* latinEdit_;
    QPlainTextEdit* englishEdit_;
    QPlainTextEdit* notesEdit_;
    Prayer base_;
};
