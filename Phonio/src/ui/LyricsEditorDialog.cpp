#include "ui/LyricsEditorDialog.h"

#include "lyrics/LyricsManager.h"
#include "lyrics/LyricsParser.h"
#include "player/PlaybackController.h"

#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QSaveFile>
#include <QFont>
#include <QMessageBox>

namespace phonio {

namespace {
QString lrcTimeStamp(qint64 ms)
{
    const qint64 minutes = ms / 60000;
    const qint64 seconds = (ms % 60000) / 1000;
    const qint64 cents = (ms % 1000) / 10;
    return QStringLiteral("[%1:%2.%3]")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'))
        .arg(cents, 2, 10, QLatin1Char('0'));
}
}

LyricsEditorDialog::LyricsEditorDialog(const Track& track, LyricsManager* lyrics,
                                       PlaybackController* controller, QWidget* parent)
    : QDialog(parent)
    , m_track(track)
    , m_lyrics(lyrics)
    , m_controller(controller)
{
    setWindowTitle(tr("Edit Lyrics - %1").arg(track.displayTitle()));
    setMinimumSize(560, 420);

    m_editor = new QPlainTextEdit(this);
    QFont mono(QStringLiteral("JetBrains Mono"));
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(11);
    m_editor->setFont(mono);
    m_editor->setPlaceholderText(tr("Paste or type LRC lyrics here.\n"
                                    "Example: [00:12.34]First line"));

    QFile file(targetPath());
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_contentAtOpen = QString::fromUtf8(file.readAll());
        m_editor->setPlainText(m_contentAtOpen);
    }
    m_editor->setFocus();

    m_insertTimeButton = new QPushButton(tr("Insert Time"), this);
    m_insertTimeButton->setToolTip(tr("Insert [mm:ss.xx] at the cursor using the current playhead."));
    m_saveButton = new QPushButton(tr("Save"), this);
    m_saveButton->setDefault(true);
    auto* cancelButton = new QPushButton(tr("Cancel"), this);

    m_statusLabel = new QLabel(this);

    auto* buttons = new QHBoxLayout;
    buttons->addWidget(m_insertTimeButton);
    buttons->addStretch(1);
    buttons->addWidget(m_statusLabel);
    buttons->addStretch(1);
    buttons->addWidget(cancelButton);
    buttons->addWidget(m_saveButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_editor, 1);
    layout->addLayout(buttons);

    connect(m_insertTimeButton, &QPushButton::clicked, this, &LyricsEditorDialog::insertTimeAtCursor);
    connect(m_saveButton, &QPushButton::clicked, this, &LyricsEditorDialog::save);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_editor, &QPlainTextEdit::textChanged, this, &LyricsEditorDialog::validate);

    validate();
}

QString LyricsEditorDialog::targetPath() const
{
    if (!m_track.lyricsPath.isEmpty())
        return m_track.lyricsPath;
    return LyricsManager::autoLyricsPath(m_track.filePath);
}

void LyricsEditorDialog::insertTimeAtCursor()
{
    qint64 positionMs = 0;
    if (m_controller && m_controller->currentTrackId() == m_track.id)
        positionMs = m_controller->player()->positionMs();

    m_editor->insertPlainText(lrcTimeStamp(positionMs) + QLatin1Char(' '));
    m_editor->setFocus();
}

void LyricsEditorDialog::validate()
{
    const auto doc = LyricsParser::parse(m_editor->toPlainText());
    const bool empty = m_editor->toPlainText().trimmed().isEmpty();
    if (empty) {
        m_statusLabel->setText(tr("Empty - will create a new file"));
    } else if (doc.isEmpty()) {
        m_statusLabel->setText(tr("No synchronized lines parsed yet"));
    } else {
        m_statusLabel->setText(tr("%1 line(s)").arg(doc.lines.size()));
    }
    m_saveButton->setEnabled(!empty);
}

void LyricsEditorDialog::save()
{
    const QString path = targetPath();
    const QString content = m_editor->toPlainText();

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Save Failed"), tr("Could not open the lyrics file for writing:\n%1").arg(path));
        return;
    }
    file.write(content.toUtf8());
    if (!file.commit()) {
        QMessageBox::warning(this, tr("Save Failed"), tr("Could not write the lyrics file:\n%1").arg(path));
        return;
    }

    m_contentAtOpen = content;
    m_lyrics->attachLyrics(m_track, path, false);

    if (m_controller && m_controller->currentTrackId() == m_track.id)
        m_lyrics->loadLyricsFor(m_track);

    accept();
}

} // namespace phonio
