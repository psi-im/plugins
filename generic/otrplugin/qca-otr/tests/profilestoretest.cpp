#include "qca-otr/profilestore.h"

#include <QTemporaryDir>
#include <QTest>
#include <QtCrypto>

class ProfileStoreTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void identityAndInstanceTagPersist();
    void fingerprintTrustLifecycle();
    void duplicateRecordsNormalizeLastWins();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void ProfileStoreTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
    QVERIFY(QCA::isSupported("sha1"));
    QVERIFY(QCA::isSupported("pkey"));
}

void ProfileStoreTest::cleanupTestCase()
{
    delete initializer_;
}

void ProfileStoreTest::identityAndInstanceTagPersist()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QcaOtr::ProfileStore store(dir.path(), "test-protocol");
    QString error;
    QVERIFY2(store.load(&error), qPrintable(error));
    QVERIFY2(store.ensureIdentity("account-a", &error), qPrintable(error));
    const QByteArray fingerprint = store.identityFingerprint("account-a");
    QCOMPARE(fingerprint.size(), 20);

    bool created = false;
    const quint32 tag = store.ensureInstanceTag("account-a", &created, &error);
    QVERIFY2(tag >= 0x00000100, qPrintable(error));
    QVERIFY(created);

    QcaOtr::ProfileStore reread(dir.path(), "test-protocol");
    QVERIFY2(reread.load(&error), qPrintable(error));
    QCOMPARE(reread.identityFingerprint("account-a"), fingerprint);
    bool found = false;
    QCOMPARE(reread.instanceTag("account-a", &found), tag);
    QVERIFY(found);

    created = true;
    QCOMPARE(reread.ensureInstanceTag("account-a", &created, &error), tag);
    QVERIFY(!created);
}

void ProfileStoreTest::fingerprintTrustLifecycle()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QcaOtr::ProfileStore store(dir.path(), "test-protocol");
    QString error;
    QVERIFY2(store.load(&error), qPrintable(error));

    const QByteArray fingerprint = QByteArray::fromHex("00112233445566778899aabbccddeeff00112233");
    bool created = false;
    QVERIFY2(store.rememberFingerprint("romeo@example.net/resource",
                                       "account-a",
                                       fingerprint,
                                       {},
                                       &created,
                                       &error),
             qPrintable(error));
    QVERIFY(created);

    const QcaOtr::FingerprintRecord *record =
        store.fingerprint("romeo@example.net/resource", "account-a", fingerprint);
    QVERIFY(record);
    QVERIFY(record->trust.isEmpty());

    QVERIFY2(store.setFingerprintTrust("romeo@example.net/resource",
                                       "account-a",
                                       fingerprint,
                                       "verified-by-user",
                                       &error),
             qPrintable(error));
    record = store.fingerprint("romeo@example.net/resource", "account-a", fingerprint);
    QVERIFY(record);
    QCOMPARE(record->trust, QByteArray("verified-by-user"));

    QcaOtr::ProfileStore reread(dir.path(), "test-protocol");
    QVERIFY2(reread.load(&error), qPrintable(error));
    record = reread.fingerprint("romeo@example.net/resource", "account-a", fingerprint);
    QVERIFY(record);
    QCOMPARE(record->trust, QByteArray("verified-by-user"));

    QVERIFY2(reread.removeFingerprint("romeo@example.net/resource", "account-a", fingerprint, &error),
             qPrintable(error));
    QVERIFY(!reread.fingerprint("romeo@example.net/resource", "account-a", fingerprint));
}

void ProfileStoreTest::duplicateRecordsNormalizeLastWins()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QcaOtr::ProfileStore seed(dir.path(), "test-protocol");
    QString error;
    QVERIFY2(seed.load(&error), qPrintable(error));
    QVERIFY2(seed.ensureIdentity("account-b", &error), qPrintable(error));

    QcaOtr::ProfileData profile = seed.data();
    QCOMPARE(profile.privateKeys.size(), 1);
    profile.privateKeys.append(profile.privateKeys.first());

    QcaOtr::InstanceTagRecord oldTag;
    oldTag.account = "account-b";
    oldTag.protocol = "test-protocol";
    oldTag.instanceTag = 0x12345678;
    QcaOtr::InstanceTagRecord newTag = oldTag;
    newTag.instanceTag = 0x87654321;
    profile.instanceTags = {oldTag, newTag};

    const QByteArray duplicateValue = QByteArray::fromHex("00112233445566778899aabbccddeeff00112233");
    QcaOtr::FingerprintRecord oldFingerprint;
    oldFingerprint.username = "romeo@example.net";
    oldFingerprint.account = "account-b";
    oldFingerprint.protocol = "test-protocol";
    oldFingerprint.fingerprint = duplicateValue;
    oldFingerprint.trust = "old";
    QcaOtr::FingerprintRecord newFingerprint = oldFingerprint;
    newFingerprint.trust = "new";

    QcaOtr::FingerprintRecord first;
    first.username = "alpha@example.net";
    first.account = "account-a";
    first.protocol = "test-protocol";
    first.fingerprint = QByteArray::fromHex("102132435465768798a9bacbdcedfe0f10213243");

    QcaOtr::FingerprintRecord last;
    last.username = "zulu@example.net";
    last.account = "account-z";
    last.protocol = "test-protocol";
    last.fingerprint = QByteArray::fromHex("ffeeddccbbaa99887766554433221100ffeeddcc");

    // Deliberately unsorted and duplicated. The later duplicate must win.
    profile.fingerprints = {last, oldFingerprint, first, newFingerprint};
    QVERIFY2(QcaOtr::Persistence::saveProfile(dir.path(), profile, &error), qPrintable(error));

    QcaOtr::ProfileStore store(dir.path(), "test-protocol");
    QVERIFY2(store.load(&error), qPrintable(error));

    const QcaOtr::ProfileData &normalized = store.data();
    QCOMPARE(normalized.privateKeys.size(), 1);
    QCOMPARE(normalized.instanceTags.size(), 1);
    QCOMPARE(normalized.fingerprints.size(), 3);

    bool found = false;
    QCOMPARE(store.instanceTag("account-b", &found), quint32(0x87654321));
    QVERIFY(found);

    const QcaOtr::FingerprintRecord *record = store.fingerprint("romeo@example.net", "account-b", duplicateValue);
    QVERIFY(record);
    QCOMPARE(record->trust, QByteArray("new"));

    // Hash iteration order is intentionally irrelevant. The DTO used by tests
    // and serialization is deterministic.
    QCOMPARE(normalized.fingerprints.at(0).username, QByteArray("alpha@example.net"));
    QCOMPARE(normalized.fingerprints.at(1).username, QByteArray("romeo@example.net"));
    QCOMPARE(normalized.fingerprints.at(2).username, QByteArray("zulu@example.net"));

    QVERIFY2(store.save(&error), qPrintable(error));
    QcaOtr::ProfileData persisted;
    QVERIFY2(QcaOtr::Persistence::loadProfile(dir.path(), &persisted, &error), qPrintable(error));
    QCOMPARE(persisted.privateKeys.size(), 1);
    QCOMPARE(persisted.instanceTags.size(), 1);
    QCOMPARE(persisted.fingerprints.size(), 3);
}

QTEST_GUILESS_MAIN(ProfileStoreTest)
#include "profilestoretest.moc"
