#include "metadata/MetadataManager.h"

#include <QFileInfo>
#include <QImageReader>
#include <QBuffer>

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <taglib/mp4file.h>
#include <taglib/mp4tag.h>
#include <taglib/mp4coverart.h>
#include <taglib/oggfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/xiphcomment.h>
#include <taglib/opusfile.h>
#include <taglib/tpropertymap.h>
#include <taglib/flacpicture.h>

#include <cstring>

namespace phonio {

namespace {

constexpr const char* kSupportedExtensions[] = {
    "mp3", "flac", "m4a", "mp4", "ogg", "opus", "wav", "aac"
};

QString utf8(const std::string& s)
{
    return QString::fromUtf8(s.c_str(), static_cast<int>(s.size()));
}

QString utf8(const char* s)
{
    return QString::fromUtf8(s ? s : "");
}

QByteArray imageToJpeg(const QImage& image)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG", 90);
    return bytes;
}

bool hasExtension(const QString& filePath, const char* ext)
{
    return QFileInfo(filePath).suffix().compare(QLatin1String(ext), Qt::CaseInsensitive) == 0;
}

QImage decodePicture(const QByteArray& data, const QByteArray& mimeType)
{
    QImage image;
    if (mimeType == QByteArrayLiteral("image/")) // TagLib flac generic picture
        image.loadFromData(data);
    else
        image.loadFromData(data, mimeType.isEmpty() ? nullptr : mimeType.constData());
    if (image.isNull())
        image.loadFromData(data);
    return image;
}

QImage readArtworkMp3(const QString& filePath)
{
    TagLib::MPEG::File file(filePath.toLocal8Bit().constData());
    if (!file.isOpen() || !file.ID3v2Tag())
        return {};
    const auto frames = file.ID3v2Tag()->frameList("APIC");
    if (frames.isEmpty())
        return {};
    const auto* pic = static_cast<const TagLib::ID3v2::AttachedPictureFrame*>(frames.front());
    const auto data = pic->picture();
    const QByteArray mime = QByteArray(pic->mimeType().toCString(true));
    return decodePicture(QByteArray(data.data(), static_cast<int>(data.size())), mime);
}

QImage readArtworkFlac(const QString& filePath)
{
    TagLib::FLAC::File file(filePath.toLocal8Bit().constData());
    if (!file.isOpen())
        return {};
    const auto pictures = file.pictureList();
    for (const auto* pic : pictures) {
        const auto data = pic->data();
        const QByteArray bytes(data.data(), static_cast<int>(data.size()));
        const std::string mimeStd = pic->mimeType().to8Bit(true);
        const QByteArray mime(mimeStd.c_str(), static_cast<int>(mimeStd.size()));
        QImage image = decodePicture(bytes, mime);
        if (!image.isNull())
            return image;
    }
    return {};
}

QImage readArtworkMp4(const QString& filePath)
{
    TagLib::MP4::File file(filePath.toLocal8Bit().constData());
    if (!file.isOpen() || !file.tag())
        return {};
    const auto& items = file.tag()->itemMap();
    const auto it = items.find("covr");
    if (it == items.end())
        return {};
    const auto covers = it->second.toCoverArtList();
    for (const auto& cover : covers) {
        const QByteArray bytes(cover.data().data(), static_cast<int>(cover.data().size()));
        QImage image = decodePicture(bytes, QByteArray());
        if (!image.isNull())
            return image;
    }
    return {};
}

QImage readArtworkOgg(const QString& filePath)
{
    TagLib::Ogg::Vorbis::File file(filePath.toLocal8Bit().constData());
    if (!file.isOpen() || !file.tag())
        return {};
    const auto& fields = file.tag()->fieldListMap();
    const auto it = fields.find("METADATA_BLOCK_PICTURE");
    if (it == fields.end())
        return {};
    const std::string base64Std = it->second.front().to8Bit(true);
    const QByteArray data = QByteArray::fromBase64(
        QByteArray(base64Std.c_str(), static_cast<int>(base64Std.size())));
    if (data.isEmpty())
        return {};
    // Try to parse as FLAC picture block, else raw image.
    TagLib::FLAC::Picture pic;
    if (pic.parse(data.constData()) != 0) {
        QImage image = decodePicture(data, QByteArray());
        return image;
    }
    const auto img = pic.data();
    const QByteArray bytes(img.data(), static_cast<int>(img.size()));
    const std::string mimeStd = pic.mimeType().to8Bit(true);
    const QByteArray mime(mimeStd.c_str(), static_cast<int>(mimeStd.size()));
    return decodePicture(bytes, mime);
}

} // namespace

MetadataManager::MetadataManager(QObject* parent)
    : QObject(parent)
{
}

bool MetadataManager::isSupportedFile(const QString& filePath) const
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    for (const char* ext : kSupportedExtensions) {
        if (suffix == QLatin1String(ext))
            return true;
    }
    return false;
}

MetadataManager::ReadResult MetadataManager::read(const QString& filePath) const
{
    ReadResult result;
    const QByteArray pathBytes = filePath.toLocal8Bit();

    TagLib::FileRef fileRef(pathBytes.constData());
    if (fileRef.isNull() || !fileRef.audioProperties() || !fileRef.tag()) {
        result.error = QStringLiteral("Unreadable or unsupported file: %1").arg(filePath);
        return result;
    }

    const TagLib::Tag* tag = fileRef.tag();
    const TagLib::AudioProperties* props = fileRef.audioProperties();

    result.ok = true;
    Track& t = result.track;
    t.filePath = filePath;
    t.title = utf8(tag->title().to8Bit(true));
    t.artist = utf8(tag->artist().to8Bit(true));
    t.album = utf8(tag->album().to8Bit(true));
    t.genre = utf8(tag->genre().to8Bit(true));
    t.year = tag->year();
    t.trackNumber = tag->track();
    t.comment = utf8(tag->comment().to8Bit(true));
    t.albumArtist = utf8(tag->properties()["ALBUMARTIST"].toString().to8Bit(true));
    t.composer = utf8(tag->properties()["COMPOSER"].toString().to8Bit(true));
    t.discNumber = tag->properties()["DISCNUMBER"].toString().toInt();
    t.durationMs = props->length() * 1000;
    t.bitrate = props->bitrate();
    t.sampleRate = props->sampleRate();
    t.fileType = QFileInfo(filePath).suffix().toLower();

    if (hasExtension(filePath, "mp3"))
        result.artwork = readArtworkMp3(filePath);
    else if (hasExtension(filePath, "flac"))
        result.artwork = readArtworkFlac(filePath);
    else if (hasExtension(filePath, "m4a") || hasExtension(filePath, "mp4"))
        result.artwork = readArtworkMp4(filePath);
    else if (hasExtension(filePath, "ogg"))
        result.artwork = readArtworkOgg(filePath);

    return result;
}

bool MetadataManager::writeTags(const QString& filePath, const Track& track, QString* error) const
{
    TagLib::FileRef fileRef(filePath.toLocal8Bit().constData(), true);
    if (fileRef.isNull() || !fileRef.tag()) {
        if (error)
            *error = tr("Cannot open file for writing.");
        return false;
    }
    TagLib::Tag* tag = fileRef.tag();
    tag->setTitle(track.title.toUtf8().constData());
    tag->setArtist(track.artist.toUtf8().constData());
    tag->setAlbum(track.album.toUtf8().constData());
    tag->setGenre(track.genre.toUtf8().constData());
    tag->setYear(static_cast<unsigned>(track.year));
    tag->setTrack(static_cast<unsigned>(track.trackNumber));
    tag->setComment(track.comment.toUtf8().constData());

    TagLib::PropertyMap props = fileRef.file()->properties();
    props.replace("ALBUMARTIST", TagLib::StringList(TagLib::String(track.albumArtist.toUtf8().constData(), TagLib::String::UTF8)));
    props.replace("COMPOSER", TagLib::StringList(TagLib::String(track.composer.toUtf8().constData(), TagLib::String::UTF8)));
    if (track.discNumber > 0)
        props.replace("DISCNUMBER", TagLib::StringList(TagLib::String::number(track.discNumber)));
    else
        props.erase("DISCNUMBER");
    fileRef.file()->setProperties(props);

    const bool ok = fileRef.file()->save();
    if (!ok && error)
        *error = tr("Failed to write tags to %1.").arg(filePath);
    return ok;
}

bool MetadataManager::writeArtwork(const QString& filePath, const QImage& image, QString* error) const
{
    if (hasExtension(filePath, "mp3"))
        return writeArtworkMp3(filePath, image, false, error);
    if (hasExtension(filePath, "flac"))
        return writeArtworkFlac(filePath, image, false, error);
    if (hasExtension(filePath, "m4a") || hasExtension(filePath, "mp4"))
        return writeArtworkMp4(filePath, image, false, error);
    if (error)
        *error = tr("Embedding artwork is not supported for this format.");
    return false;
}

bool MetadataManager::removeArtwork(const QString& filePath, QString* error) const
{
    if (hasExtension(filePath, "mp3"))
        return writeArtworkMp3(filePath, {}, true, error);
    if (hasExtension(filePath, "flac"))
        return writeArtworkFlac(filePath, {}, true, error);
    if (hasExtension(filePath, "m4a") || hasExtension(filePath, "mp4"))
        return writeArtworkMp4(filePath, {}, true, error);
    if (error)
        *error = tr("Removing embedded artwork is not supported for this format.");
    return false;
}

bool MetadataManager::artworkSupported(const QString& filePath) const
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    return suffix == QLatin1String("mp3") || suffix == QLatin1String("flac")
        || suffix == QLatin1String("m4a") || suffix == QLatin1String("mp4");
}

bool MetadataManager::writeArtworkMp3(const QString& filePath, const QImage& image, bool remove, QString* error)
{
    TagLib::MPEG::File file(filePath.toLocal8Bit().constData());
    if (!file.isOpen()) {
        if (error)
            *error = tr("Cannot open file for writing.");
        return false;
    }
    TagLib::ID3v2::Tag* tag = file.ID3v2Tag(true);
    const auto existing = tag->frameList("APIC");
    for (auto* frame : existing)
        tag->removeFrame(frame);

    if (!remove) {
        const QByteArray jpeg = imageToJpeg(image);
        if (jpeg.isEmpty()) {
            if (error)
                *error = tr("Failed to encode cover art.");
            return false;
        }
        auto* pic = new TagLib::ID3v2::AttachedPictureFrame;
        pic->setMimeType("image/jpeg");
        pic->setPicture(TagLib::ByteVector(jpeg.constData(), jpeg.size()));
        pic->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
        pic->setDescription("Cover");
        tag->addFrame(pic);
    }
    if (!file.save())
    {
        if (error)
            *error = tr("Failed to save changes to %1.").arg(filePath);
        return false;
    }
    return true;
}

bool MetadataManager::writeArtworkFlac(const QString& filePath, const QImage& image, bool remove, QString* error)
{
    TagLib::FLAC::File file(filePath.toLocal8Bit().constData());
    if (!file.isOpen()) {
        if (error)
            *error = tr("Cannot open file for writing.");
        return false;
    }
    for (auto* pic : file.pictureList())
        file.removePicture(pic);

    if (!remove) {
        const QByteArray jpeg = imageToJpeg(image);
        if (jpeg.isEmpty()) {
            if (error)
                *error = tr("Failed to encode cover art.");
            return false;
        }
        auto* pic = new TagLib::FLAC::Picture;
        pic->setMimeType("image/jpeg");
        pic->setType(TagLib::FLAC::Picture::FrontCover);
        pic->setData(TagLib::ByteVector(jpeg.constData(), jpeg.size()));
        file.addPicture(pic);
    }
    if (!file.save()) {
        if (error)
            *error = tr("Failed to save changes to %1.").arg(filePath);
        return false;
    }
    return true;
}

bool MetadataManager::writeArtworkMp4(const QString& filePath, const QImage& image, bool remove, QString* error)
{
    TagLib::MP4::File file(filePath.toLocal8Bit().constData());
    if (!file.isOpen() || !file.tag()) {
        if (error)
            *error = tr("Cannot open file for writing.");
        return false;
    }
    auto& items = const_cast<TagLib::MP4::ItemMap&>(file.tag()->itemMap());
    if (remove) {
        items.erase("covr");
    } else {
        const QByteArray jpeg = imageToJpeg(image);
        if (jpeg.isEmpty()) {
            if (error)
                *error = tr("Failed to encode cover art.");
            return false;
        }
        TagLib::MP4::CoverArtList covers;
        covers.append(TagLib::MP4::CoverArt(TagLib::MP4::CoverArt::JPEG,
                                            TagLib::ByteVector(jpeg.constData(), jpeg.size())));
        items.insert("covr", covers);
    }    if (!file.save()) {
        if (error)
            *error = tr("Failed to save changes to %1.").arg(filePath);
        return false;
    }
    return true;
}

} // namespace phonio
