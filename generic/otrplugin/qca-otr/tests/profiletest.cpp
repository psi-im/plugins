#include "qca-otr/profile.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QtCrypto>

namespace {

QcaOtr::DsaPrivateKey toyKey(int x)
{
    QcaOtr::DsaPrivateKey key;
    key.domain.p = QCA::BigInteger(23);
    key.domain.q = QCA::BigInteger(11);
    key.domain.g = QCA::BigInteger(2);
    key.x = QCA::BigInteger(x);
    return key;
}

bool writeFile(const QString &path, const QByteArray &data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

} // namespace

class ProfileTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void missingProfileIsEmpty();
    void profileRoundTripAndLookup();
    void malformedLineStoresKeepValidRecords();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void ProfileTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
}

void ProfileTest::cleanupTestCase()
{
    delete initializer_;
}

void ProfileTest::missingProfileIsEmpty()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QcaOtr::ProfileData profile;
    QString error;
    QVERIFY2(QcaOtr::Persistence::loadProfile(dir.path(), &profile, &error), qPrintable(error));
    QVERIFY(profile.privateKeys.isEmpty());
    QVERIFY(profile.fingerprints.isEmpty());
    QVERIFY(profile.instanceTags.isEmpty());
}

void ProfileTest::profileRoundTripAndLookup()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QcaOtr::ProfileData profile;

    QcaOtr::PrivateKeyRecord oldKey;
    oldKey.account = "account-a";
    oldKey.protocol = QcaOtr::LegacyPsiProtocolId;
    oldKey.key = toyKey(3);
    profile.privateKeys.append(oldKey);

    QcaOtr::PrivateKeyRecord replacementKey = oldKey;
    replacementKey.key = toyKey(4);
    profile.privateKeys.append(replacementKey);

    QcaOtr::PrivateKeyRecord otherKey;
    otherKey.account = "account-b";
    otherKey.protocol = QcaOtr::LegacyPsiProtocolId;
    otherKey.key = toyKey(5);
    profile.privateKeys.append(otherKey);

    QcaOtr::FingerprintRecord oldFingerprint;
    oldFingerprint.username = "romeo@example.net";
    oldFingerprint.account = "account-a";
    oldFingerprint.protocol = QcaOtr::LegacyPsiProtocolId;
    oldFingerprint.fingerprint = QByteArray::fromHex("00112233445566778899aabbccddeeff00112233");
    oldFingerprint.trust = "old-trust";
    profile.fingerprints.append(oldFingerprint);

    QcaOtr::FingerprintRecord replacementFingerprint = oldFingerprint;
    replacementFingerprint.trust = "custom-libotr-trust";
    profile.fingerprints.append(replacementFingerprint);

    QcaOtr::InstanceTagRecord oldTag;
    oldTag.account = "account-a";
    oldTag.protocol = QcaOtr::LegacyPsiProtocolId;
    oldTag.instanceTag = 0x11111111;
    profile.instanceTags.append(oldTag);

    QcaOtr::InstanceTagRecord replacementTag = oldTag;
    replacementTag.instanceTag = 0x22222222;
    profile.instanceTags.append(replacementTag);

    QString error;
    QVERIFY2(QcaOtr::Persistence::saveProfile(dir.path(), profile, &error), qPrintable(error));

    QcaOtr::ProfileData loaded;
    QVERIFY2(QcaOtr::Persistence::loadProfile(dir.path(), &loaded, &error), qPrintable(error));
    QCOMPARE(loaded.privateKeys.size(), 3);
    QCOMPARE(loaded.fingerprints.size(), 2);
    QCOMPARE(loaded.instanceTags.size(), 2);

    const QcaOtr::PrivateKeyRecord *key = QcaOtr::Persistence::findPrivateKey(loaded, "account-a");
    QVERIFY(key);
    QCOMPARE(key->key.x, QCA::BigInteger(4));

    const QcaOtr::FingerprintRecord *fingerprint = QcaOtr::Persistence::findFingerprint(
        loaded, "romeo@example.net", "account-a", oldFingerprint.fingerprint);
    QVERIFY(fingerprint);
    QCOMPARE(fingerprint->trust, QByteArray("custom-libotr-trust"));

    bool found = false;
    QCOMPARE(QcaOtr::Persistence::findInstanceTag(loaded, "account-a", QcaOtr::LegacyPsiProtocolId, &found),
             quint32(0x22222222));
    QVERIFY(found);

    found = true;
    QCOMPARE(QcaOtr::Persistence::findInstanceTag(loaded, "missing", QcaOtr::LegacyPsiProtocolId, &found), quint32(0));
    QVERIFY(!found);
}

void ProfileTest::malformedLineStoresKeepValidRecords()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QByteArray fingerprints =
        "romeo@example.net\taccount-a\tprpl-jabber\t00112233445566778899aabbccddeeff00112233\tfirst\n"
        "this is not a fingerprint record\n"
        "juliet@example.net\taccount-b\tprpl-jabber\tffeeddccbbaa99887766554433221100ffeeddcc\tsecond\n";
    const QByteArray instags =
        "# WARNING! legacy comment\n"
        "account-a\tprpl-jabber\t12345678\n"
        "broken instance tag line\n"
        "account-b\tprpl-jabber\t89abcdef\n";

    QVERIFY(writeFile(dir.filePath(QcaOtr::OtrFingerprintsFileName), fingerprints));
    QVERIFY(writeFile(dir.filePath(QcaOtr::OtrInstanceTagsFileName), instags));

    QcaOtr::ProfileData profile;
    QString error;
    QVERIFY2(QcaOtr::Persistence::loadProfile(dir.path(), &profile, &error), qPrintable(error));
    QVERIFY(profile.privateKeys.isEmpty());
    QCOMPARE(profile.fingerprints.size(), 2);
    QCOMPARE(profile.fingerprints.at(0).trust, QByteArray("first"));
    QCOMPARE(profile.fingerprints.at(1).trust, QByteArray("second"));
    QCOMPARE(profile.instanceTags.size(), 2);
    QCOMPARE(profile.instanceTags.at(0).instanceTag, quint32(0x12345678));
    QCOMPARE(profile.instanceTags.at(1).instanceTag, quint32(0x89abcdef));
}

QTEST_GUILESS_MAIN(ProfileTest)
#include "profiletest.moc"
