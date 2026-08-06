#include "KJVImporter.h"
#include "BookNameMatcher.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <map>
#include <string>

std::unique_ptr<Bible> KJVImporter::load(const std::filesystem::path& path)
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

    const QJsonArray verses = doc.object()["verses"].toArray();

    struct RawVerse {
        int number;
        std::string text;
    };

    struct RawChapter {
        int number;
        std::vector<RawVerse> verses;
    };

    // book number → {book name, chapters}
    std::map<int, std::pair<std::string, std::map<int, std::vector<RawVerse>>>> bookMap;
    const auto& matcher = BookNameMatcher::instance();

    for (const QJsonValue& val : verses) {
        const QJsonObject obj = val.toObject();
        const int bookNum = obj["book"].toInt();
        const int chNum = obj["chapter"].toInt();
        const int vsNum = obj["verse"].toInt();
        const std::string text = obj["text"].toString().toStdString();

        auto& [name, chapters] = bookMap[bookNum];
        if (name.empty()) {
            std::string rawName = obj["book_name"].toString().toStdString();
            auto canonical = matcher.find(rawName);
            name = canonical.value_or(rawName);
        }

        chapters[chNum].push_back({vsNum, text});
    }

    auto bible = std::make_unique<Bible>();
    bible->setName("King James Version");
    bible->setShortName("KJV");

    for (auto& [bookNum, pair] : bookMap) {
        auto& [bookName, chapters] = pair;

        Book book;
        book.name = bookName;
        book.number = bookNum;

        for (auto& [chNum, rawVerses] : chapters) {
            Chapter ch;
            ch.number = chNum;
            for (auto& rv : rawVerses) {
                ch.verses.push_back({rv.number, rv.text});
            }
            book.chapters.push_back(std::move(ch));
        }

        bible->addBook(std::move(book));
    }

    return bible;
}
