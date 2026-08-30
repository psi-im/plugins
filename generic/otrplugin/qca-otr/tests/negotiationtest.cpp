#include "qca-otr/negotiation.h"

#include <QTest>

using namespace QcaOtr::Negotiation;

class NegotiationTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void buildsQueryTags();
    void buildsDefaultQueryMessage();
    void parsesQueryVersions();
    void matchesMalformedQueryHandling();
    void choosesBestVersion();
    void buildsWhitespaceTags();
    void detectsAndStripsWhitespaceTags();
    void consumesUnknownWhitespaceGroups();
};

void NegotiationTest::buildsQueryTags()
{
    QCOMPARE(queryTag(Version1), QByteArray("?OTR?"));
    QCOMPARE(queryTag(Version2), QByteArray("?OTRv2?"));
    QCOMPARE(queryTag(Version3), QByteArray("?OTRv3?"));
    QCOMPARE(queryTag(Version2 | Version3), QByteArray("?OTRv23?"));
    QCOMPARE(queryTag(Version1 | Version2 | Version3), QByteArray("?OTR?v23?"));
}

void NegotiationTest::buildsDefaultQueryMessage()
{
    const QByteArray message = defaultQueryMessage("Alice", Version2 | Version3);
    QVERIFY(message.startsWith("?OTRv23?\n<b>Alice</b> has requested an "));
    QVERIFY(message.contains("Off-the-Record private conversation"));
}

void NegotiationTest::parsesQueryVersions()
{
    bool isQuery = false;
    QCOMPARE(queryVersions("hello ?OTRv3? world", &isQuery), VersionMask(Version3));
    QVERIFY(isQuery);

    QCOMPARE(queryVersions("hello ?OTRv23? world", &isQuery),
             VersionMask(Version2 | Version3));
    QVERIFY(isQuery);

    QCOMPARE(queryVersions("hello ?OTR?v23? world", &isQuery),
             VersionMask(Version1 | Version2 | Version3));
    QVERIFY(isQuery);

    QCOMPARE(queryVersions("ordinary plaintext", &isQuery), VersionMask(0));
    QVERIFY(!isQuery);
}

void NegotiationTest::matchesMalformedQueryHandling()
{
    bool isQuery = false;

    // libotr treats ?OTRv as a Query Message and scans version digits until
    // '?' or the end of the string, so a missing closing '?' is accepted.
    QCOMPARE(queryVersions("?OTRv3", &isQuery), VersionMask(Version3));
    QVERIFY(isQuery);

    // Unknown version identifiers are ignored while known ones are retained.
    QCOMPARE(queryVersions("?OTRv243?", &isQuery),
             VersionMask(Version2 | Version3));
    QVERIFY(isQuery);

    QCOMPARE(queryVersions("?OTRv4?", &isQuery), VersionMask(0));
    QVERIFY(isQuery);

    // otrl_proto_message_type() only inspects the first ?OTR occurrence.
    QCOMPARE(queryVersions("?OTRbroken ?OTRv3?", &isQuery), VersionMask(0));
    QVERIFY(!isQuery);
}

void NegotiationTest::choosesBestVersion()
{
    QCOMPARE(bestVersion(Version1 | Version2 | Version3,
                         Version1 | Version2 | Version3),
             quint8(3));
    QCOMPARE(bestVersion(Version1 | Version2, Version1 | Version2 | Version3), quint8(2));
    QCOMPARE(bestVersion(Version1, Version1 | Version2 | Version3), quint8(1));
    QCOMPARE(bestVersion(Version2, NativeVersions), quint8(0));

    bool isQuery = false;
    QCOMPARE(queryBestVersion("?OTRv23?", NativeVersions, &isQuery), quint8(3));
    QVERIFY(isQuery);
}

void NegotiationTest::buildsWhitespaceTags()
{
    QCOMPARE(whitespaceTag(Version3).toHex(),
             QByteArray("200920200909090920092009200920202020090920200909"));
    QCOMPARE(whitespaceTag(Version1 | Version2 | Version3).toHex(),
             QByteArray("20092020090909092009200920092020"
                        "2009200920200920"
                        "2020090920200920"
                        "2020090920200909"));
}

void NegotiationTest::detectsAndStripsWhitespaceTags()
{
    QByteArray message = "hello";
    message.append(whitespaceTag(Version2 | Version3));
    message.append("world");

    const WhitespaceTag tag = detectWhitespaceTag(message);
    QVERIFY(tag.found);
    QCOMPARE(tag.start, 5);
    QCOMPARE(tag.end, 5 + 16 + 8 + 8);
    QCOMPARE(tag.versions, VersionMask(Version2 | Version3));
    QCOMPARE(whitespaceBestVersion(message, NativeVersions), quint8(3));

    VersionMask versions = 0;
    QVERIFY(stripWhitespaceTag(&message, &versions));
    QCOMPARE(versions, VersionMask(Version2 | Version3));
    QCOMPARE(message, QByteArray("helloworld"));

    QVERIFY(!stripWhitespaceTag(&message));
}

void NegotiationTest::consumesUnknownWhitespaceGroups()
{
    QByteArray message = "before";
    message.append(whitespaceTag(0));
    message.append(QByteArray(8, ' '));
    message.append(QByteArray::fromHex("2020090920200909"));
    message.append("after");

    const WhitespaceTag tag = detectWhitespaceTag(message);
    QVERIFY(tag.found);
    QCOMPARE(tag.versions, VersionMask(Version3));
    QCOMPARE(tag.end - tag.start, 16 + 8 + 8);

    QVERIFY(stripWhitespaceTag(&message));
    QCOMPARE(message, QByteArray("beforeafter"));
}

QTEST_MAIN(NegotiationTest)
#include "negotiationtest.moc"
