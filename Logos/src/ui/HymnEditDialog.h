#pragma once

#include "data/HymnLibrary.h"

#include <QDialog>

class QLineEdit;
class QPlainTextEdit;
class QSpinBox;

// Add / edit form for a single hymn. Returns the edited hymn via
// `hymn()` when accepted; the id is preserved so updates target the
// same entry.
class HymnEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit HymnEditDialog(QWidget* parent = nullptr);

    void setHymn(const Hymn& hymn);
    [[nodiscard]] Hymn hymn() const;

    static Hymn editHymn(QWidget* parent, const Hymn& hymn);
    static Hymn newHymn(QWidget* parent);

private:
    QSpinBox* numberEdit_;
    QLineEdit* titleEdit_;
    QLineEdit* tuneEdit_;
    QLineEdit* authorEdit_;
    QLineEdit* composerEdit_;
    QLineEdit* categoryEdit_;
    QPlainTextEdit* latinEdit_;
    QPlainTextEdit* englishEdit_;
    QPlainTextEdit* notesEdit_;
    Hymn base_;
};
