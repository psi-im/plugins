#include "libotrtest.h"
#include "qca-otr/ake.h"
#include "qca-otr/persistence.h"

#include <QTest>
#include <QtCrypto>

#include <cstdio>

namespace {

QByteArray fileContents(FILE *file)
{
    if (!file || std::fflush(file) != 0 || std::fseek(file, 0, SEEK_END) != 0)
        return {};
    const long size = std::ftell(file);
    if (size < 0 || std::fseek(file, 0, SEEK_SET) != 0)
        return {};

    QByteArray data;
    data.resize(static_cast<int>(size));
    if (size > 0 && std::fread(data.data(), 1, static_cast<size_t>(size), file) != static_cast<size_t>(size))
        return {};
    return data;
}

FILE *fileFromData(const QByteArray &data)
{
    FILE *file = std::tmpfile();
    if (!file)
        return nullptr;
    if (!data.isEmpty() && std::fwrite(data.constData(), 1, static_cast<size_t>(data.size()), file) != static_cast<size_t>(data.size())) {
        std::fclose(file);
        return nullptr;
    }
    std::rewind(file);
    return file;
}

QByteArray libotrFingerprint(OtrlUserState userState, const QByteArray &account, const QByteArray &protocol)
{
    unsigned char fingerprint[20];
    if (!otrl_privkey_fingerprint_raw(userState, fingerprint, account.constData(), protocol.constData()))
        return {};
    return QByteArray(reinterpret_cast<const char *>(fingerprint), 20);
}

} // namespace

class PersistenceInteropTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void privateKeysRoundTripWithLibotr();
    void fingerprintsRoundTripWithLibotr();
    void instanceTagsRoundTripWithLibotr();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void PersistenceInteropTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
    OTRL_INIT;
    QVERIFY2(QByteArray(otrl_version()).startsWith("4.1.1"), otrl_version());
}

void PersistenceInteropTest::cleanupTestCase()
{
    delete initializer_;
}

void PersistenceInteropTest::privateKeysRoundTripWithLibotr()
{
    const QByteArray account("migration@example.net");
    const QByteArray protocol(QcaOtr::LegacyPsiProtocolId);

    OtrlUserState original = otrl_userstate_create();
    QVERIFY(original);
    FILE *generated = std::tmpfile();
    QVERIFY(generated);
    QCOMPARE(otrl_privkey_generate_FILEp(original, generated, account.constData(), protocol.constData()), gcry_error_t(0));
    const QByteArray libotrStore = fileContents(generated);
    std::fclose(generated);
    QVERIFY(!libotrStore.isEmpty());

    const QByteArray originalFingerprint = libotrFingerprint(original, account, protocol);
    QCOMPARE(originalFingerprint.size(), 20);

    QList<QcaOtr::PrivateKeyRecord> nativeKeys;
    QString error;
    QVERIFY2(QcaOtr::Persistence::parsePrivateKeys(libotrStore, &nativeKeys, &error), qPrintable(error));
    QCOMPARE(nativeKeys.size(), 1);
    QCOMPARE(nativeKeys.first().account, account);
    QCOMPARE(nativeKeys.first().protocol, protocol);
    QCOMPARE(QcaOtr::dsaPublicKeyFingerprint(QcaOtr::dsaPublicKey(nativeKeys.first().key)), originalFingerprint);

    bool ok = false;
    const QByteArray nativeStore = QcaOtr::Persistence::serializePrivateKeys(nativeKeys, &ok);
    QVERIFY(ok);
    FILE *nativeFile = fileFromData(nativeStore);
    QVERIFY(nativeFile);

    OtrlUserState reread = otrl_userstate_create();
    QVERIFY(reread);
    QCOMPARE(otrl_privkey_read_FILEp(reread, nativeFile), gcry_error_t(0));
    std::fclose(nativeFile);
    QCOMPARE(libotrFingerprint(reread, account, protocol), originalFingerprint);

    otrl_userstate_free(reread);
    otrl_userstate_free(original);
}

void PersistenceInteropTest::fingerprintsRoundTripWithLibotr()
{
    const QByteArray user("romeo@example.net");
    const QByteArray account("account-a");
    const QByteArray protocol(QcaOtr::LegacyPsiProtocolId);
    const QByteArray fingerprint = QByteArray::fromHex("00112233445566778899aabbccddeeff00112233");
    const QByteArray trust("native-custom-trust");
    QByteArray mutableFingerprint = fingerprint;

    QcaOtr::FingerprintRecord nativeRecord;
    nativeRecord.username = user;
    nativeRecord.account = account;
    nativeRecord.protocol = protocol;
    nativeRecord.fingerprint = fingerprint;
    nativeRecord.trust = trust;
    bool ok = false;
    const QByteArray nativeStore = QcaOtr::Persistence::serializeFingerprints({nativeRecord}, &ok);
    QVERIFY(ok);

    OtrlUserState libotr = otrl_userstate_create();
    QVERIFY(libotr);
    FILE *nativeFile = fileFromData(nativeStore);
    QVERIFY(nativeFile);
    QCOMPARE(otrl_privkey_read_fingerprints_FILEp(libotr, nativeFile, nullptr, nullptr), gcry_error_t(0));
    std::fclose(nativeFile);

    ConnContext *context = otrl_context_find(libotr,
                                             user.constData(),
                                             account.constData(),
                                             protocol.constData(),
                                             OTRL_INSTAG_MASTER,
                                             0,
                                             nullptr,
                                             nullptr,
                                             nullptr);
    QVERIFY(context);
    Fingerprint *libotrFingerprintRecord = otrl_context_find_fingerprint(
        context, reinterpret_cast<unsigned char *>(mutableFingerprint.data()), 0, nullptr);
    QVERIFY(libotrFingerprintRecord);
    QCOMPARE(QByteArray(libotrFingerprintRecord->trust), trust);
    otrl_userstate_free(libotr);

    OtrlUserState writer = otrl_userstate_create();
    QVERIFY(writer);
    int added = 0;
    context = otrl_context_find(writer,
                                user.constData(),
                                account.constData(),
                                protocol.constData(),
                                OTRL_INSTAG_MASTER,
                                1,
                                &added,
                                nullptr,
                                nullptr);
    QVERIFY(context);
    libotrFingerprintRecord = otrl_context_find_fingerprint(
        context, reinterpret_cast<unsigned char *>(mutableFingerprint.data()), 1, nullptr);
    QVERIFY(libotrFingerprintRecord);
    otrl_context_set_trust(libotrFingerprintRecord, "libotr-custom-trust");

    FILE *written = std::tmpfile();
    QVERIFY(written);
    QCOMPARE(otrl_privkey_write_fingerprints_FILEp(writer, written), gcry_error_t(0));
    const QByteArray libotrStore = fileContents(written);
    std::fclose(written);

    QList<QcaOtr::FingerprintRecord> nativeRecords;
    QString error;
    QVERIFY2(QcaOtr::Persistence::parseFingerprints(libotrStore, &nativeRecords, &error), qPrintable(error));
    QCOMPARE(nativeRecords.size(), 1);
    QCOMPARE(nativeRecords.first().username, user);
    QCOMPARE(nativeRecords.first().account, account);
    QCOMPARE(nativeRecords.first().protocol, protocol);
    QCOMPARE(nativeRecords.first().fingerprint, fingerprint);
    QCOMPARE(nativeRecords.first().trust, QByteArray("libotr-custom-trust"));
    otrl_userstate_free(writer);
}

void PersistenceInteropTest::instanceTagsRoundTripWithLibotr()
{
    const QByteArray account("account-a");
    const QByteArray protocol(QcaOtr::LegacyPsiProtocolId);

    QcaOtr::InstanceTagRecord nativeRecord;
    nativeRecord.account = account;
    nativeRecord.protocol = protocol;
    nativeRecord.instanceTag = 0x12345678;
    bool ok = false;
    const QByteArray nativeStore = QcaOtr::Persistence::serializeInstanceTags({nativeRecord}, &ok);
    QVERIFY(ok);

    OtrlUserState libotr = otrl_userstate_create();
    QVERIFY(libotr);
    FILE *nativeFile = fileFromData(nativeStore);
    QVERIFY(nativeFile);
    QCOMPARE(otrl_instag_read_FILEp(libotr, nativeFile), gcry_error_t(0));
    std::fclose(nativeFile);
    OtrlInsTag *tag = otrl_instag_find(libotr, account.constData(), protocol.constData());
    QVERIFY(tag);
    QCOMPARE(tag->instag, otrl_instag_t(nativeRecord.instanceTag));
    otrl_userstate_free(libotr);

    OtrlUserState writer = otrl_userstate_create();
    QVERIFY(writer);
    FILE *written = std::tmpfile();
    QVERIFY(written);
    QCOMPARE(otrl_instag_generate_FILEp(writer, written, account.constData(), protocol.constData()), gcry_error_t(0));
    tag = otrl_instag_find(writer, account.constData(), protocol.constData());
    QVERIFY(tag);
    const quint32 generatedTag = tag->instag;
    const QByteArray libotrStore = fileContents(written);
    std::fclose(written);

    QList<QcaOtr::InstanceTagRecord> nativeRecords;
    QString error;
    QVERIFY2(QcaOtr::Persistence::parseInstanceTags(libotrStore, &nativeRecords, &error), qPrintable(error));
    QCOMPARE(nativeRecords.size(), 1);
    QCOMPARE(nativeRecords.first().account, account);
    QCOMPARE(nativeRecords.first().protocol, protocol);
    QCOMPARE(nativeRecords.first().instanceTag, generatedTag);
    otrl_userstate_free(writer);
}

QTEST_GUILESS_MAIN(PersistenceInteropTest)
#include "persistenceinteroptest.moc"
