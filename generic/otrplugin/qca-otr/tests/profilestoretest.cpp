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

QTEST_GUILESS_MAIN(ProfileStoreTest)
#include "profilestoretest.moc"
