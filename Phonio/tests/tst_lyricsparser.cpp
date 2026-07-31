#include <QtTest>

#include "lyrics/LyricsParser.h"

using namespace phonio;

class TestLyricsParser : public QObject
{
    Q_OBJECT

private slots:
    void parsesSimpleTimestamps();
    void parsesHundredthsAndTenths();
    void expandsMultipleTimestamps();
    void parsesMetadataTags();
    void appliesOffset();
    void sortsLinesByTime();
    void handlesEmptyAndGarbage();
    void findsActiveLine();
};

void TestLyricsParser::parsesSimpleTimestamps()
{
    const auto doc = LyricsParser::parse(QStringLiteral(
        "[00:01.00]Hello\n"
        "[00:05.50]World\n"));
    QCOMPARE(doc.lines.size(), 2);
    QCOMPARE(doc.lines.at(0).timeMs, 1000);
    QCOMPARE(doc.lines.at(0).text, QStringLiteral("Hello"));
    QCOMPARE(doc.lines.at(1).timeMs, 5500);
    QCOMPARE(doc.lines.at(1).text, QStringLiteral("World"));
}

void TestLyricsParser::parsesHundredthsAndTenths()
{
    const auto doc = LyricsParser::parse(QStringLiteral(
        "[00:02.5]Tenths\n"
        "[00:03.25]Hundredths\n"
        "[00:04.123]Millis\n"));
    QCOMPARE(doc.lines.at(0).timeMs, 2500);
    QCOMPARE(doc.lines.at(1).timeMs, 3250);
    QCOMPARE(doc.lines.at(2).timeMs, 4123);
}

void TestLyricsParser::expandsMultipleTimestamps()
{
    const auto doc = LyricsParser::parse(QStringLiteral(
        "[00:01.00][00:02.00][00:03.00]Repeat me\n"));
    QCOMPARE(doc.lines.size(), 3);
    QCOMPARE(doc.lines.at(0).timeMs, 1000);
    QCOMPARE(doc.lines.at(1).timeMs, 2000);
    QCOMPARE(doc.lines.at(2).timeMs, 3000);
    QCOMPARE(doc.lines.at(2).text, QStringLiteral("Repeat me"));
}

void TestLyricsParser::parsesMetadataTags()
{
    const auto doc = LyricsParser::parse(QStringLiteral(
        "[ti:My Song]\n"
        "[ar:My Artist]\n"
        "[al:My Album]\n"
        "[00:01.00]First\n"));
    QCOMPARE(doc.title, QStringLiteral("My Song"));
    QCOMPARE(doc.artist, QStringLiteral("My Artist"));
    QCOMPARE(doc.album, QStringLiteral("My Album"));
    QCOMPARE(doc.lines.size(), 1);
}

void TestLyricsParser::appliesOffset()
{
    const auto doc = LyricsParser::parse(QStringLiteral(
        "[offset:500]\n"
        "[00:01.00]Late\n"));
    QCOMPARE(doc.lines.at(0).timeMs, 1500);

    const auto early = LyricsParser::parse(QStringLiteral(
        "[offset:-1000]\n"
        "[00:01.00]Early\n"));
    QCOMPARE(early.lines.at(0).timeMs, 0); // clamped to zero
}

void TestLyricsParser::sortsLinesByTime()
{
    const auto doc = LyricsParser::parse(QStringLiteral(
        "[00:03.00]Third\n"
        "[00:01.00]First\n"
        "[00:02.00]Second\n"));
    QCOMPARE(doc.lines.at(0).text, QStringLiteral("First"));
    QCOMPARE(doc.lines.at(1).text, QStringLiteral("Second"));
    QCOMPARE(doc.lines.at(2).text, QStringLiteral("Third"));
}

void TestLyricsParser::handlesEmptyAndGarbage()
{
    QVERIFY(LyricsParser::parse(QString()).isEmpty());
    QVERIFY(LyricsParser::parse(QStringLiteral("no timestamps here")).isEmpty());
    QVERIFY(LyricsParser::parse(QStringLiteral("[not-a-time]text")).isEmpty());
    QVERIFY(LyricsParser::parse(QStringLiteral("Made by some tool\n[00:01.00]Line")).lines.size() == 1);
}

void TestLyricsParser::findsActiveLine()
{
    const auto doc = LyricsParser::parse(QStringLiteral(
        "[00:01.00]A\n"
        "[00:05.00]B\n"
        "[00:09.00]C\n"));
    QCOMPARE(LyricsParser::activeLine(doc, 0), -1);
    QCOMPARE(LyricsParser::activeLine(doc, 1000), 0);
    QCOMPARE(LyricsParser::activeLine(doc, 4999), 0);
    QCOMPARE(LyricsParser::activeLine(doc, 5000), 1);
    QCOMPARE(LyricsParser::activeLine(doc, 9000), 2);
    QCOMPARE(LyricsParser::activeLine(doc, 999999), 2);
}

QTEST_MAIN(TestLyricsParser)
#include "tst_lyricsparser.moc"
