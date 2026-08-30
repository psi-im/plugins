#include "qca-otr/ake.h"
#include "qca-otr/persistence.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QtCrypto>

namespace {

const char LibotrPrivateKey[] = R"KEY((privkeys
 (account
(name otrtest3)
(protocol prpl-aim)
(private-key
 (dsa
  (p #00BB4C57669E50E4C35F8E4CA84855CF2C83EE75C4F44B4BB4A7E88590D394D7A738E82EE97892E5051CE45E200741E18D423137AA8E6679B1CFAB4FF11D45D8C9CBDE388D30FC800B4879713E3C57BA48A92FE135BB9AF265F770B706FB9A04802244D12CBFFD97ACE5C73FCE88C2B716B4B22B994CD6429A7E16D9B6D1874137#)
  (q #00C40DA63B679A80FC31BF49A68503BB39754D0A45#)
  (g #6C0A48BEA859587D6677306D1777A2A0635470F149A86EB64EA62EAAA4C21ECE4375ACD016B776E3AD3411C18BB3FF37F963FCEBB8820FF8838AFA6FCD1B39558DAB78450AE2ED9457DEDBDCE13DF5A6B20A738D2973D375D360C044AF7F0204CCC372098F0B6460963274B1EA0B5FEC93571A15F5C03DCDF54EE83BB198F363#)
  (y #00AB2C8A82F020DB99EF5B7A8330EC43E0D5EBD623FEB67D1B046D88FACA01D8E31E4D7865DC62D4DA58CF8BC7FF4B57C203A9F7F5C85DAB1B63D63299EF13AD89AAA7E6638C9DBC42D096408936C9F0382224CFB5C1528DCC8C7F2554CB4CA2FF3C3239BC921F1C690295DD9AE69C8EF5BBD8E58A8FAA8BB9D5F88463CAECEE7B#)
  (x #7824B713A4E5FA6D6C69172196648CD4657A1ED1#)
  )
 )
 )
)
)KEY";

QByteArray fingerprint(const QcaOtr::PrivateKeyRecord &record)
{
    return QcaOtr::dsaPublicKeyFingerprint(QcaOtr::dsaPublicKey(record.key));
}

} // namespace

class PersistenceTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void legacyProtocolId();
    void privateKeysParseAndRoundTrip();
    void fingerprintsParseAndRoundTrip();
    void instanceTagsParseAndRoundTrip();
    void malformedStoresAreRejected();
    void atomicFileRoundTrip();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void PersistenceTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
}

void PersistenceTest::cleanupTestCase()
{
    delete initializer_;
}

void PersistenceTest::legacyProtocolId()
{
    QCOMPARE(QByteArray(QcaOtr::LegacyPsiProtocolId), QByteArray("prpl-jabber"));
}

void PersistenceTest::privateKeysParseAndRoundTrip()
{
    QList<QcaOtr::PrivateKeyRecord> records;
    QString error;
    QVERIFY2(QcaOtr::Persistence::parsePrivateKeys(LibotrPrivateKey, &records, &error), qPrintable(error));
    QCOMPARE(records.size(), 1);
    QCOMPARE(records.first().account, QByteArray("otrtest3"));
    QCOMPARE(records.first().protocol, QByteArray("prpl-aim"));
    QCOMPARE(fingerprint(records.first()).toHex(), QByteArray("55b7b505e08e4f5d2473df28297ea6a52305e93a"));

    bool ok = false;
    const QByteArray serialized = QcaOtr::Persistence::serializePrivateKeys(records, &ok);
    QVERIFY(ok);
    QVERIFY(serialized.startsWith("(privkeys\n"));

    QList<QcaOtr::PrivateKeyRecord> roundTrip;
    QVERIFY2(QcaOtr::Persistence::parsePrivateKeys(serialized, &roundTrip, &error), qPrintable(error));
    QCOMPARE(roundTrip.size(), 1);
    QCOMPARE(roundTrip.first().account, records.first().account);
    QCOMPARE(roundTrip.first().protocol, records.first().protocol);
    QCOMPARE(roundTrip.first().key.domain.p, records.first().key.domain.p);
    QCOMPARE(roundTrip.first().key.domain.q, records.first().key.domain.q);
    QCOMPARE(roundTrip.first().key.domain.g, records.first().key.domain.g);
    QCOMPARE(roundTrip.first().key.x, records.first().key.x);
    QCOMPARE(fingerprint(roundTrip.first()), fingerprint(records.first()));
}

void PersistenceTest::fingerprintsParseAndRoundTrip()
{
    const QByteArray input =
        "romeo@example.net\taccount-a\tprpl-jabber\t00112233445566778899aabbccddeeff00112233\tverified\n"
        "juliet@example.net\taccount-b\tprpl-jabber\tffeeddccbbaa99887766554433221100ffeeddcc\tmanual\ttrust\n"
        "mercutio@example.net\taccount-c\tprpl-jabber\t0123456789abcdef0123456789abcdef01234567\n";

    QList<QcaOtr::FingerprintRecord> records;
    QString error;
    QVERIFY2(QcaOtr::Persistence::parseFingerprints(input, &records, &error), qPrintable(error));
    QCOMPARE(records.size(), 3);
    QCOMPARE(records.at(0).trust, QByteArray("verified"));
    QCOMPARE(records.at(1).trust, QByteArray("manual\ttrust"));
    QVERIFY(records.at(2).trust.isEmpty());

    bool ok = false;
    const QByteArray serialized = QcaOtr::Persistence::serializeFingerprints(records, &ok);
    QVERIFY(ok);
    QList<QcaOtr::FingerprintRecord> roundTrip;
    QVERIFY2(QcaOtr::Persistence::parseFingerprints(serialized, &roundTrip, &error), qPrintable(error));
    QCOMPARE(roundTrip.size(), records.size());
    for (int i = 0; i < records.size(); ++i) {
        QCOMPARE(roundTrip.at(i).username, records.at(i).username);
        QCOMPARE(roundTrip.at(i).account, records.at(i).account);
        QCOMPARE(roundTrip.at(i).protocol, records.at(i).protocol);
        QCOMPARE(roundTrip.at(i).fingerprint, records.at(i).fingerprint);
        QCOMPARE(roundTrip.at(i).trust, records.at(i).trust);
    }
}

void PersistenceTest::instanceTagsParseAndRoundTrip()
{
    const QByteArray input =
        "# WARNING! ignored by libotr\n"
        "account-a\tprpl-jabber\t00000100\n"
        "account-b\tprpl-jabber\t89abcdef\n";

    QList<QcaOtr::InstanceTagRecord> records;
    QString error;
    QVERIFY2(QcaOtr::Persistence::parseInstanceTags(input, &records, &error), qPrintable(error));
    QCOMPARE(records.size(), 2);
    QCOMPARE(records.at(0).instanceTag, quint32(0x00000100));
    QCOMPARE(records.at(1).instanceTag, quint32(0x89abcdef));

    bool ok = false;
    const QByteArray serialized = QcaOtr::Persistence::serializeInstanceTags(records, &ok);
    QVERIFY(ok);
    QVERIFY(serialized.startsWith("# WARNING!"));
    QList<QcaOtr::InstanceTagRecord> roundTrip;
    QVERIFY2(QcaOtr::Persistence::parseInstanceTags(serialized, &roundTrip, &error), qPrintable(error));
    QCOMPARE(roundTrip.size(), records.size());
    QCOMPARE(roundTrip.at(0).instanceTag, records.at(0).instanceTag);
    QCOMPARE(roundTrip.at(1).instanceTag, records.at(1).instanceTag);
}

void PersistenceTest::malformedStoresAreRejected()
{
    QList<QcaOtr::PrivateKeyRecord> keys;
    QList<QcaOtr::FingerprintRecord> fingerprints;
    QList<QcaOtr::InstanceTagRecord> tags;
    QString error;

    QVERIFY(!QcaOtr::Persistence::parsePrivateKeys("(privkeys (account (name a)))", &keys, &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!QcaOtr::Persistence::parseFingerprints(
        "user\taccount\tprpl-jabber\tnot-a-40-byte-hex-fingerprint\n", &fingerprints, &error));
    QVERIFY(!error.isEmpty());

    error.clear();
    QVERIFY(!QcaOtr::Persistence::parseInstanceTags("account\tprpl-jabber\t00000001\n", &tags, &error));
    QVERIFY(!error.isEmpty());
}

void PersistenceTest::atomicFileRoundTrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QList<QcaOtr::PrivateKeyRecord> keys;
    QString error;
    QVERIFY2(QcaOtr::Persistence::parsePrivateKeys(LibotrPrivateKey, &keys, &error), qPrintable(error));

    QcaOtr::FingerprintRecord fp;
    fp.username = "romeo@example.net";
    fp.account = "account-a";
    fp.protocol = QcaOtr::LegacyPsiProtocolId;
    fp.fingerprint = QByteArray::fromHex("00112233445566778899aabbccddeeff00112233");
    fp.trust = "verified";
    const QList<QcaOtr::FingerprintRecord> fingerprints{fp};

    QcaOtr::InstanceTagRecord tag;
    tag.account = "account-a";
    tag.protocol = QcaOtr::LegacyPsiProtocolId;
    tag.instanceTag = 0x12345678;
    const QList<QcaOtr::InstanceTagRecord> tags{tag};

    const QString keyPath = dir.filePath("otr.keys");
    const QString fingerprintPath = dir.filePath("otr.fingerprints");
    const QString tagPath = dir.filePath("otr.instags");
    QVERIFY2(QcaOtr::Persistence::writePrivateKeysFile(keyPath, keys, &error), qPrintable(error));
    QVERIFY2(QcaOtr::Persistence::writeFingerprintsFile(fingerprintPath, fingerprints, &error), qPrintable(error));
    QVERIFY2(QcaOtr::Persistence::writeInstanceTagsFile(tagPath, tags, &error), qPrintable(error));

    QList<QcaOtr::PrivateKeyRecord> loadedKeys;
    QList<QcaOtr::FingerprintRecord> loadedFingerprints;
    QList<QcaOtr::InstanceTagRecord> loadedTags;
    QVERIFY2(QcaOtr::Persistence::readPrivateKeysFile(keyPath, &loadedKeys, &error), qPrintable(error));
    QVERIFY2(QcaOtr::Persistence::readFingerprintsFile(fingerprintPath, &loadedFingerprints, &error), qPrintable(error));
    QVERIFY2(QcaOtr::Persistence::readInstanceTagsFile(tagPath, &loadedTags, &error), qPrintable(error));
    QCOMPARE(fingerprint(loadedKeys.first()), fingerprint(keys.first()));
    QCOMPARE(loadedFingerprints.first().trust, fp.trust);
    QCOMPARE(loadedTags.first().instanceTag, tag.instanceTag);

#ifdef Q_OS_UNIX
    const QFileDevice::Permissions permissions = QFile::permissions(keyPath);
    QVERIFY(permissions.testFlag(QFileDevice::ReadOwner));
    QVERIFY(permissions.testFlag(QFileDevice::WriteOwner));
    QVERIFY(!permissions.testFlag(QFileDevice::ReadGroup));
    QVERIFY(!permissions.testFlag(QFileDevice::ReadOther));
#endif
}

QTEST_GUILESS_MAIN(PersistenceTest)
#include "persistencetest.moc"
