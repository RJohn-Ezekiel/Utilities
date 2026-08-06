#include "ui/MetadataDialog.h"

#include "metadata/MetadataManager.h"
#include "artwork/ArtworkManager.h"

#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QImageReader>
#include <QGroupBox>

namespace phonio {

namespace {
constexpr int kPreviewSize = 220;
}

MetadataDialog::MetadataDialog(const Track& track, MetadataManager* metadata, ArtworkManager* artwork,
                               QWidget* parent)
    : QDialog(parent)
    , m_track(track)
    , m_metadata(metadata)
    , m_artwork(artwork)
    , m_title(new QLineEdit(track.title, this))
    , m_artist(new QLineEdit(track.artist, this))
    , m_album(new QLineEdit(track.album, this))
    , m_albumArtist(new QLineEdit(track.albumArtist, this))
    , m_genre(new QLineEdit(track.genre, this))
    , m_year(new QSpinBox(this))
    , m_trackNumber(new QSpinBox(this))
    , m_discNumber(new QSpinBox(this))
    , m_composer(new QLineEdit(track.composer, this))
    , m_comment(new QTextEdit(this))
    , m_coverPreview(new QLabel(this))
{
    setWindowTitle(tr("Edit Metadata - %1").arg(track.displayTitle()));
    setMinimumWidth(560);

    m_year->setRange(0, 9999);
    m_year->setValue(track.year);
    m_year->setSpecialValueText(tr("Unknown"));
    m_trackNumber->setRange(0, 9999);
    m_trackNumber->setValue(track.trackNumber);
    m_trackNumber->setSpecialValueText(tr("Unknown"));
    m_discNumber->setRange(0, 99);
    m_discNumber->setValue(track.discNumber);
    m_discNumber->setSpecialValueText(tr("Unknown"));
    m_comment->setPlainText(track.comment);
    m_comment->setMaximumHeight(80);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(16);

    auto* body = new QHBoxLayout;
    body->setSpacing(20);

    // Left: cover art
    auto* coverBox = new QVBoxLayout;
    coverBox->setSpacing(8);
    m_coverPreview->setFixedSize(kPreviewSize, kPreviewSize);
    m_coverPreview->setScaledContents(true);
    coverBox->addWidget(m_coverPreview, 0, Qt::AlignHCenter);
    auto* replaceButton = new QPushButton(tr("Replace Cover"), this);
    auto* removeButton = new QPushButton(tr("Remove Cover"), this);
    coverBox->addWidget(replaceButton);
    coverBox->addWidget(removeButton);
    coverBox->addStretch();
    body->addLayout(coverBox);

    // Right: form
    auto* form = new QFormLayout;
    form->setSpacing(8);
    form->addRow(tr("Title"), m_title);
    form->addRow(tr("Artist"), m_artist);
    form->addRow(tr("Album"), m_album);
    form->addRow(tr("Album Artist"), m_albumArtist);
    form->addRow(tr("Genre"), m_genre);
    auto* numbersRow = new QHBoxLayout;
    numbersRow->setSpacing(8);
    numbersRow->addWidget(m_year);
    numbersRow->addWidget(m_trackNumber);
    numbersRow->addWidget(m_discNumber);
    form->addRow(tr("Year / Track / Disc"), numbersRow);
    form->addRow(tr("Composer"), m_composer);
    form->addRow(tr("Comment"), m_comment);
    body->addLayout(form, 1);
    root->addLayout(body);

    // Buttons
    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    auto* cancelButton = new QPushButton(tr("Cancel"), this);
    auto* saveButton = new QPushButton(tr("Save"), this);
    saveButton->setObjectName(QStringLiteral("accentButton"));
    buttons->addWidget(cancelButton);
    buttons->addWidget(saveButton);
    root->addLayout(buttons);

    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveButton, &QPushButton::clicked, this, &MetadataDialog::save);
    connect(replaceButton, &QPushButton::clicked, this, &MetadataDialog::replaceCover);
    connect(removeButton, &QPushButton::clicked, this, &MetadataDialog::removeCover);
    connect(m_coverPreview, &QLabel::linkActivated, this, [this](const QString&) { replaceCover(); });

    loadArtwork();
}

void MetadataDialog::loadArtwork()
{
    QPixmap pixmap = m_artwork->artworkFor(m_track);
    if (pixmap.isNull())
        pixmap = m_artwork->placeholder(kPreviewSize);
    m_coverPreview->setPixmap(pixmap);
}

void MetadataDialog::replaceCover()
{
    const QString file = QFileDialog::getOpenFileName(this, tr("Choose Cover Image"), QString(),
                                                      tr("Images (*.jpg *.jpeg *.png)"));
    if (file.isEmpty())
        return;
    QImageReader reader(file);
    const QImage image = reader.read();
    if (image.isNull()) {
        QMessageBox::warning(this, tr("Invalid Image"), tr("Could not load the selected image."));
        return;
    }
    m_newCover = image;
    m_coverDirty = true;
    m_removeCover = false;
    m_coverPreview->setPixmap(QPixmap::fromImage(image).scaled(
        kPreviewSize, kPreviewSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
}

void MetadataDialog::removeCover()
{
    m_removeCover = true;
    m_coverDirty = true;
    m_coverPreview->setPixmap(m_artwork->placeholder(kPreviewSize));
}

void MetadataDialog::save()
{
    Track updated = m_track;
    updated.title = m_title->text().trimmed();
    updated.artist = m_artist->text().trimmed();
    updated.album = m_album->text().trimmed();
    updated.albumArtist = m_albumArtist->text().trimmed();
    updated.genre = m_genre->text().trimmed();
    updated.year = m_year->value();
    updated.trackNumber = m_trackNumber->value();
    updated.discNumber = m_discNumber->value();
    updated.composer = m_composer->text().trimmed();
    updated.comment = m_comment->toPlainText();

    QString error;
    if (!m_metadata->writeTags(updated.filePath, updated, &error)) {
        QMessageBox::critical(this, tr("Save Failed"), error);
        return;
    }
    if (m_coverDirty) {
        if (m_removeCover) {
            if (!m_metadata->removeArtwork(updated.filePath, &error)) {
                QMessageBox::critical(this, tr("Failed to Remove Cover"), error);
                return;
            }
        } else if (!m_newCover.isNull()) {
            if (!m_metadata->writeArtwork(updated.filePath, m_newCover, &error)) {
                QMessageBox::critical(this, tr("Failed to Write Cover"), error);
                return;
            }
        }
    }

    // Re-read the file so audio properties and tags are consistent.
    auto fresh = m_metadata->read(updated.filePath);
    if (fresh.ok) {
        Track reread = fresh.track;
        reread.rating = m_track.rating;
        reread.playCount = m_track.playCount;
        reread.lastPlayedMs = m_track.lastPlayedMs;
        reread.favorite = m_track.favorite;
        reread.lyricsPath = m_track.lyricsPath;
        updated = reread;
    }
    accept();
    emit metadataSaved(updated);
}

} // namespace phonio
