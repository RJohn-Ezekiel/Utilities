#pragma once

#include "core/Types.h"

#include <QDialog>

class QPlainTextEdit;
class QPushButton;
class QLabel;

namespace phonio {

class LyricsManager;
class PlaybackController;

// LRC lyrics editor: raw text editing, timestamp insertion at the current
// playhead, live parse validation, save-to-file and attach-to-track.
class LyricsEditorDialog : public QDialog
{
    Q_OBJECT

public:
    LyricsEditorDialog(const Track& track, LyricsManager* lyrics,
                       PlaybackController* controller, QWidget* parent = nullptr);

private slots:
    void insertTimeAtCursor();
    void validate();
    void save();

private:
    QString targetPath() const;

    Track m_track;
    LyricsManager* m_lyrics;
    PlaybackController* m_controller;

    QPlainTextEdit* m_editor;
    QPushButton* m_insertTimeButton;
    QPushButton* m_saveButton;
    QLabel* m_statusLabel;
    QString m_contentAtOpen;
};

} // namespace phonio
