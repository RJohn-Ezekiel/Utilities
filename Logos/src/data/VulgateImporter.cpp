#include "VulgateImporter.h"
#include "BookNameMatcher.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

std::unique_ptr<Bible> VulgateImporter::load(const std::filesystem::path& path)
{
    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::ReadOnly))
        return nullptr;

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return nullptr;

    const QJsonObject root = doc.object();
    const QJsonArray booksArray = root["books"].toArray();

    auto bible = std::make_unique<Bible>();
    bible->setName("Clementine Vulgate");
    bible->setShortName("Vulg");

    const auto& matcher = BookNameMatcher::instance();

    for (const QJsonValue& bookVal : booksArray) {
        const QJsonObject bookObj = bookVal.toObject();
        const QJsonArray chaptersArray = bookObj["chapters"].toArray();

        Book book;
        std::string rawName = bookObj["name"].toString().toStdString();
        auto canonical = matcher.find(rawName);
        book.name = canonical.value_or(rawName);

        book.number = static_cast<int>(bible->bookCount()) + 1;

        for (const QJsonValue& chVal : chaptersArray) {
            const QJsonObject chObj = chVal.toObject();
            const QJsonArray versesArray = chObj["verses"].toArray();

            Chapter ch;
            ch.number = chObj["chapter"].toInt();

            for (const QJsonValue& vVal : versesArray) {
                const QJsonObject vObj = vVal.toObject();
                Verse v;
                v.number = vObj["verse"].toInt();
                v.text = vObj["text"].toString().toStdString();
                ch.verses.push_back(std::move(v));
            }

            book.chapters.push_back(std::move(ch));
        }

        bible->addBook(std::move(book));
    }

    return bible;
}
