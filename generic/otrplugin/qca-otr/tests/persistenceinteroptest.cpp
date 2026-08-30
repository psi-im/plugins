#include "libotrtest.h"
#include "qca-otr/ake.h"
#include "qca-otr/persistence.h"
#include "qca-otr/profile.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QtCrypto>

#include <cstdio>

namespace {

constexpr char PsiProtocolId[] = "prpl-jabber";

QByteArray fileContents(FILE *file)
{
    if (!file || std::fflush(file) != 0 || std::fseek(file, 0, SEEK_END) != 0)
        return {};
    const long size = std::ftell(file);
    if (size < 0 || size > INT_MAX || std::fseek(file, 0, SEEK_SET) != 0)
        return {};

    QByteArray data;
    data.resize(static_cast<int>(size));
    if (size > 0 && std::fread(data.data(), 1, static_cast<size_t>(size), file) != static_cast<size_t>(size))
        return {};
    return data;
}

QCA::SecureArray secureFileContents(FILE *file)
{
    if (!file || std::fflush(file) != 0 || std::fseek(file, 0, SEEK_END) != 0)
        return {};
    const long size = std::ftell(file);
    if (size < 0 || size > INT_MAX || std::fseek(file, 0, SEEK_SET) != 0)
        return {};

    QCA::SecureArray data(static_cast<int>(size));
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

FILE *fileFromData(const QCA::SecureArray &data)
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
    void completeProfileMigrationWithLibotr();

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
    const QByteArray protocol(PsiProtocolId);

    OtrlUserState original = otrl_userstate_create();
    QVERIFY(original);
    FILE *generated = std::tmpfile();
    QVERIFY(generated);
    QCOMPARE(otrl_privkey_generate_FILEp(original, generated, account.constData(), protocol.constData()), gcry_error_t(0));
    const QCA::SecureArray libotrStore = secureFileContents(generated);
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
    const QCA::SecureArray nativeStore = QcaOtr::Persistence::serializePrivateKeys(nativeKeys, &ok);
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
    const QByteArray protocol(PsiProtocolId);
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
    const QByteArray protocol(PsiProtocolId);

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

void PersistenceInteropTest::completeProfileMigrationWithLibotr()
{
    QTemporaryDir legacyDir;
    QTemporaryDir nativeDir;
    QVERIFY(legacyDir.isValid());
    QVERIFY(nativeDir.isValid());

    const QByteArray account("psi-account-a");
    const QByteArray protocol(PsiProtocolId);
    const QByteArray peer("juliet@example.net");
    const QByteArray peerFingerprint = QByteArray::fromHex("102132435465768798a9bacbdcedfe0f10213243");
    const QByteArray trust("psi-user-verified");
    QByteArray mutablePeerFingerprint = peerFingerprint;

    const QByteArray legacyKeysPath = QFile::encodeName(legacyDir.filePath(QcaOtr::OtrKeysFileName));
    const QByteArray legacyFingerprintsPath = QFile::encodeName(legacyDir.filePath(QcaOtr::OtrFingerprintsFileName));
    const QByteArray legacyInstagsPath = QFile::encodeName(legacyDir.filePath(QcaOtr::OtrInstanceTagsFileName));

    OtrlUserState legacy = otrl_userstate_create();
    QVERIFY(legacy);
    QCOMPARE(otrl_privkey_generate(legacy, legacyKeysPath.constData(), account.constData(), protocol.constData()), gcry_error_t(0));
    const QByteArray identityFingerprint = libotrFingerprint(legacy, account, protocol);
    QCOMPARE(identityFingerprint.size(), 20);

    int added = 0;
    ConnContext *context = otrl_context_find(legacy,
                                             peer.constData(),
                                             account.constData(),
                                             protocol.constData(),
                                             OTRL_INSTAG_MASTER,
                                             1,
                                             &added,
                                             nullptr,
                                             nullptr);
    QVERIFY(context);
    Fingerprint *peerRecord = otrl_context_find_fingerprint(
        context, reinterpret_cast<unsigned char *>(mutablePeerFingerprint.data()), 1, nullptr);
    QVERIFY(peerRecord);
    otrl_context_set_trust(peerRecord, trust.constData());
    QCOMPARE(otrl_privkey_write_fingerprints(legacy, legacyFingerprintsPath.constData()), gcry_error_t(0));
    QCOMPARE(otrl_instag_generate(legacy, legacyInstagsPath.constData(), account.constData(), protocol.constData()), gcry_error_t(0));
    OtrlInsTag *legacyTag = otrl_instag_find(legacy, account.constData(), protocol.constData());
    QVERIFY(legacyTag);
    const quint32 instanceTag = legacyTag->instag;
    otrl_userstate_free(legacy);

    QcaOtr::ProfileData migrated;
    QString error;
    QVERIFY2(QcaOtr::Persistence::loadProfile(legacyDir.path(), &migrated, &error), qPrintable(error));
    QCOMPARE(migrated.privateKeys.size(), 1);
    QCOMPARE(migrated.fingerprints.size(), 1);
    QCOMPARE(migrated.instanceTags.size(), 1);

    const QcaOtr::PrivateKeyRecord *nativeKey = QcaOtr::Persistence::findPrivateKey(migrated, account, protocol);
    QVERIFY(nativeKey);
    QCOMPARE(QcaOtr::dsaPublicKeyFingerprint(QcaOtr::dsaPublicKey(nativeKey->key)), identityFingerprint);
    const QcaOtr::FingerprintRecord *nativePeer =
        QcaOtr::Persistence::findFingerprint(migrated, peer, account, peerFingerprint, protocol);
    QVERIFY(nativePeer);
    QCOMPARE(nativePeer->trust, trust);
    bool foundTag = false;
    QCOMPARE(QcaOtr::Persistence::findInstanceTag(migrated, account, protocol, &foundTag), instanceTag);
    QVERIFY(foundTag);

    QVERIFY2(QcaOtr::Persistence::saveProfile(nativeDir.path(), migrated, &error), qPrintable(error));

    const QByteArray nativeKeysPath = QFile::encodeName(nativeDir.filePath(QcaOtr::OtrKeysFileName));
    const QByteArray nativeFingerprintsPath = QFile::encodeName(nativeDir.filePath(QcaOtr::OtrFingerprintsFileName));
    const QByteArray nativeInstagsPath = QFile::encodeName(nativeDir.filePath(QcaOtr::OtrInstanceTagsFileName));

    OtrlUserState reread = otrl_userstate_create();
    QVERIFY(reread);
    QCOMPARE(otrl_privkey_read(reread, nativeKeysPath.constData()), gcry_error_t(0));
    QCOMPARE(otrl_privkey_read_fingerprints(reread, nativeFingerprintsPath.constData(), nullptr, nullptr), gcry_error_t(0));
    QCOMPARE(otrl_instag_read(reread, nativeInstagsPath.constData()), gcry_error_t(0));
    QCOMPARE(libotrFingerprint(reread, account, protocol), identityFingerprint);

    context = otrl_context_find(reread,
                                peer.constData(),
                                account.constData(),
                                protocol.constData(),
                                OTRL_INSTAG_MASTER,
                                0,
                                nullptr,
                                nullptr,
                                nullptr);
    QVERIFY(context);
    peerRecord = otrl_context_find_fingerprint(
        context, reinterpret_cast<unsigned char *>(mutablePeerFingerprint.data()), 0, nullptr);
    QVERIFY(peerRecord);
    QCOMPARE(QByteArray(peerRecord->trust), trust);
    OtrlInsTag *rereadTag = otrl_instag_find(reread, account.constData(), protocol.constData());
    QVERIFY(rereadTag);
    QCOMPARE(rereadTag->instag, otrl_instag_t(instanceTag));
    otrl_userstate_free(reread);
}

QTEST_GUILESS_MAIN(PersistenceInteropTest)
#include "persistenceinteroptest.moc"
