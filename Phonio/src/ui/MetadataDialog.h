#pragma once

#include "core/Types.h"

#include <QDialog>

class QLineEdit;
class QSpinBox;
class QTextEdit;
class QPushButton;
class QLabel;

namespace phonio {

class MetadataManager;
class ArtworkManager;

// Musicolet-style metadata editor. Writes directly into audio tags via TagLib.
class MetadataDialog : public QDialog
{
    Q_OBJECT

public:
    MetadataDialog(const Track& track, MetadataManager* metadata, ArtworkManager* artwork,
                   QWidget* parent = nullptr);

signals:
    void metadataSaved(const Track& updated);

private:
    void loadArtwork();
    void replaceCover();
    void removeCover();
    void save();

    Track m_track;
    MetadataManager* m_metadata;
    ArtworkManager* m_artwork;

    QLineEdit* m_title;
    QLineEdit* m_artist;
    QLineEdit* m_album;
    QLineEdit* m_albumArtist;
    QLineEdit* m_genre;
    QSpinBox* m_year;
    QSpinBox* m_trackNumber;
    QSpinBox* m_discNumber;
    QLineEdit* m_composer;
    QTextEdit* m_comment;
    QLabel* m_coverPreview;
    bool m_coverDirty = false;
    QImage m_newCover;
    bool m_removeCover = false;
};

} // namespace phonio
