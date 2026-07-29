#include "BookNameMatcher.h"

#include <algorithm>
#include <cctype>

namespace {

std::string toLower(std::string_view s)
{
    std::string result;
    result.reserve(s.size());
    for (char c : s)
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return result;
}

std::string normalizeSpace(std::string_view s)
{
    std::string result;
    result.reserve(s.size());
    bool wasSpace = true;
    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!wasSpace)
                result.push_back(' ');
            wasSpace = true;
        } else {
            result.push_back(c);
            wasSpace = false;
        }
    }
    if (!result.empty() && result.back() == ' ')
        result.pop_back();
    return result;
}

} // namespace

BookNameMatcher::BookNameMatcher()
{
    addBook("Genesis", {"genesis", "gen"});
    addBook("Exodus", {"exodus", "exo", "ex"});
    addBook("Leviticus", {"leviticus", "lev"});
    addBook("Numbers", {"numbers", "num", "nu"});
    addBook("Deuteronomy", {"deuteronomy", "deut", "deu", "dt"});
    addBook("Joshua", {"joshua", "josh", "jos"});
    addBook("Judges", {"judges", "judg", "jdg"});
    addBook("Ruth", {"ruth", "rut"});
    addBook("1 Samuel", {"1 samuel", "i samuel", "1 sam", "i sam", "1samuel", "1sam", "1 sm"});
    addBook("2 Samuel", {"2 samuel", "ii samuel", "2 sam", "ii sam", "2samuel", "2sam", "2 sm"});
    addBook("1 Kings", {"1 kings", "i kings", "1 kgs", "i kgs", "1kings", "1kgs"});
    addBook("2 Kings", {"2 kings", "ii kings", "2 kgs", "ii kgs", "2kings", "2kgs"});
    addBook("1 Chronicles", {"1 chronicles", "i chronicles", "1 chron", "i chron", "1chr", "1 chron"});
    addBook("2 Chronicles", {"2 chronicles", "ii chronicles", "2 chron", "ii chron", "2chr", "2 chron"});
    addBook("Ezra", {"ezra", "ezr"});
    addBook("Nehemiah", {"nehemiah", "neh"});
    addBook("Esther", {"esther", "est", "esth"});
    addBook("Job", {"job"});
    addBook("Psalms", {"psalms", "psalm", "ps", "psa", "pslm"});
    addBook("Proverbs", {"proverbs", "prov", "pro", "prv"});
    addBook("Ecclesiastes", {"ecclesiastes", "ecc", "eccles"});
    addBook("Song of Solomon", {"song of solomon", "song of sol", "song", "sos", "canticle of canticles", "canticles"});
    addBook("Isaiah", {"isaiah", "isa", "is"});
    addBook("Jeremiah", {"jeremiah", "jer", "jr"});
    addBook("Lamentations", {"lamentations", "lam"});
    addBook("Ezekiel", {"ezekiel", "ezek", "ez"});
    addBook("Daniel", {"daniel", "dan", "dn"});
    addBook("Hosea", {"hosea", "hos"});
    addBook("Joel", {"joel"});
    addBook("Amos", {"amos"});
    addBook("Obadiah", {"obadiah", "obad"});
    addBook("Jonah", {"jonah"});
    addBook("Micah", {"micah", "mic"});
    addBook("Nahum", {"nahum", "nah"});
    addBook("Habakkuk", {"habakkuk", "hab"});
    addBook("Zephaniah", {"zephaniah", "zeph", "zep"});
    addBook("Haggai", {"haggai", "hag"});
    addBook("Zechariah", {"zechariah", "zech", "zec"});
    addBook("Malachi", {"malachi", "mal"});
    addBook("Matthew", {"matthew", "matt", "mt"});
    addBook("Mark", {"mark", "mk", "mrk"});
    addBook("Luke", {"luke", "lk"});
    addBook("John", {"john", "jn", "jhn"});
    addBook("Acts", {"acts"});
    addBook("Romans", {"romans", "rom", "ro"});
    addBook("1 Corinthians", {"1 corinthians", "i corinthians", "1 cor", "i cor", "1cor", "1corinthians"});
    addBook("2 Corinthians", {"2 corinthians", "ii corinthians", "2 cor", "ii cor", "2cor", "2corinthians"});
    addBook("Galatians", {"galatians", "gal"});
    addBook("Ephesians", {"ephesians", "eph"});
    addBook("Philippians", {"philippians", "phil", "php", "phi"});
    addBook("Colossians", {"colossians", "col"});
    addBook("1 Thessalonians", {"1 thessalonians", "i thessalonians", "1 thess", "i thess", "1thess", "1thessalonians"});
    addBook("2 Thessalonians", {"2 thessalonians", "ii thessalonians", "2 thess", "ii thess", "2thess", "2thessalonians"});
    addBook("1 Timothy", {"1 timothy", "i timothy", "1 tim", "i tim", "1tim", "1timothy"});
    addBook("2 Timothy", {"2 timothy", "ii timothy", "2 tim", "ii tim", "2tim", "2timothy"});
    addBook("Titus", {"titus", "tit"});
    addBook("Philemon", {"philemon", "philem", "phm"});
    addBook("Hebrews", {"hebrews", "heb"});
    addBook("James", {"james", "jas"});
    addBook("1 Peter", {"1 peter", "i peter", "1 pet", "i pet", "1pet", "1peter"});
    addBook("2 Peter", {"2 peter", "ii peter", "2 pet", "ii pet", "2pet", "2peter"});
    addBook("1 John", {"1 john", "i john", "1 jn", "i jn", "1john", "1jhn"});
    addBook("2 John", {"2 john", "ii john", "2 jn", "ii jn", "2john", "2jhn"});
    addBook("3 John", {"3 john", "iii john", "3 jn", "iii jn", "3john", "3jhn"});
    addBook("Jude", {"jude"});
    addBook("Revelation", {"revelation", "rev", "revelation of john", "apocalypse"});

    // Vulgate-specific aliases (extra books)
    addBook("Tobit", {"tobit", "tobias"});
    addBook("Judith", {"judith", "jdt"});
    addBook("Wisdom", {"wisdom", "wisdom of solomon", "wis"});
    addBook("Sirach", {"sirach", "ecclesiasticus", "sir"});
    addBook("Baruch", {"baruch", "bar"});
    addBook("1 Maccabees", {"1 maccabees", "i maccabees", "1 macc", "i macc"});
    addBook("2 Maccabees", {"2 maccabees", "ii maccabees", "2 macc", "ii macc"});
    addBook("Prayer of Manasses", {"prayer of manasses", "prayer of manasseh"});
    addBook("1 Esdras", {"1 esdras", "i esdras"});
    addBook("2 Esdras", {"2 esdras", "ii esdras"});
    addBook("Additional Psalm", {"additional psalm", "psalm 151", "ps 151"});
    addBook("Laodiceans", {"laodiceans"});
}

void BookNameMatcher::addAlias(std::string_view canonical, std::string_view alias)
{
    std::string key = normalizeSpace(toLower(alias));
    aliasMap_[key] = std::string(canonical);
}

void BookNameMatcher::addBook(std::string_view canonical, std::vector<std::string> aliases)
{
    canonicalNames_.emplace_back(canonical);
    for (auto& alias : aliases)
        addAlias(canonical, alias);
    // Also add the canonical name itself
    addAlias(canonical, canonical);
}

std::optional<std::string_view> BookNameMatcher::find(std::string_view input) const
{
    std::string key = normalizeSpace(toLower(input));
    auto it = aliasMap_.find(key);
    if (it != aliasMap_.end())
        return it->second;
    return std::nullopt;
}

const BookNameMatcher& BookNameMatcher::instance()
{
    static const BookNameMatcher matcher;
    return matcher;
}
