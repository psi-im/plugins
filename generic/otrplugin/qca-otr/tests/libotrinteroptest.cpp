#include "qca-otr/akesession.h"

#include <QTest>
#include <QtCrypto>

#include <cstdio>
#include <cstdlib>

extern "C" {
#include <libotr/auth.h>
#include <libotr/b64.h>
#include <libotr/context.h>
#include <libotr/privkey.h>
#include <libotr/proto.h>
#include <libotr/userstate.h>
}

namespace {

constexpr quint32 QcaInstance = 0x11111111;
constexpr quint32 LibotrInstance = 0x22222222;
constexpr const char *LibotrAccount = "otrtest3";
constexpr const char *Protocol = "prpl-aim";
constexpr const char *RemoteUser = "qca-peer";

// Exact otrtest3 entry from libotr 4.1.1's test_suite/otr.private_key.
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

QCA::BigInteger unsignedHex(const char *hex)
{
    QByteArray bytes = QByteArray::fromHex(hex);
    QByteArray positive;
    positive.reserve(bytes.size() + 1);
    positive.append('\0');
    positive.append(bytes);
    return QCA::BigInteger(QCA::SecureArray(positive));
}

QcaOtr::DsaPrivateKey qcaPrivateKey()
{
    QcaOtr::DsaPrivateKey key;
    key.domain.p = unsignedHex(
        "00BD114F05B275A8A94954047983C5CD96ED95C782D2ED65A18E78C98E8EAFBAF58BBD046BE9895AD55FD0FF95907E7EBD6ACA2688D24779BDE9F0AAB13924CE65F597F9C9B9953DDBACF51DA7113FBAB9BE1DF6C6EA836DEB48983CCDCFC4125B5013D0CE52F890D0C391A035D30BCD5169A3451FD7023685274576DCB5F8FA47");
    key.domain.q = unsignedHex("00D1DA3915346A704EB2D2F2A48CD48F3DCC4CF25D");
    key.domain.g = unsignedHex(
        "501BCFB989AD2C346BBD7782CA0230551F976B1A07EE3AEE27E4B63B7B00B1ACA712AD85784986411278163156D4DBA9DF75C8560F9C2E02C02AEC830EC403A56B6F64432869D6CA9314A648076511343507629BF4FC96F8FDBB9797258DDF11F437B1450BA23F1AA7E885EC6A33D37B7D7EC384A004420DB238E140B94AAAFE");
    key.x = unsignedHex("00AB1E941176D94505911118AC799A504ADCCE88F8");
    return key;
}

QCA::BigInteger expectedQcaPublicY()
{
    return unsignedHex(
        "7C9CB7732164787DD1931BB58257665EB60D6AA72B8D64D634530A61BE93D5AF01427962646542F18401B73032B12B9CBCAE8E3CF080DAD55C6612A97D6D8776CF2CBDD3AAC75D302B60E6956E5B3C60B39E171A2D5F150A924C6E22981EFDF052D5C6507B2DEC15E96CB6CAF7B260D5386BBDD7D7F69B4BF14451D64D847AEB");
}

QByteArray toLibotrMessage(const QByteArray &raw)
{
    char *encoded = otrl_base64_otr_encode(
        reinterpret_cast<const unsigned char *>(raw.constData()), static_cast<size_t>(raw.size()));
    if (!encoded)
        return {};
    const QByteArray result(encoded);
    std::free(encoded);
    return result;
}

QByteArray fromLibotrMessage(const char *encoded)
{
    if (!encoded)
        return {};

    unsigned char *raw = nullptr;
    size_t length = 0;
    if (otrl_base64_otr_decode(encoded, &raw, &length) != 0)
        return {};
    const QByteArray result(reinterpret_cast<const char *>(raw), static_cast<int>(length));
    std::free(raw);
    return result;
}

struct AuthCapture
{
    bool succeeded = false;
    QByteArray peerFingerprint;
    QByteArray sessionId;
    quint32 peerKeyId = 0;
    bool initiated = false;
};

gcry_error_t authSucceeded(const OtrlAuthInfo *auth, void *data)
{
    auto *capture = static_cast<AuthCapture *>(data);
    capture->succeeded = true;
    capture->peerFingerprint = QByteArray(reinterpret_cast<const char *>(auth->their_fingerprint), 20);
    capture->sessionId = QByteArray(reinterpret_cast<const char *>(auth->secure_session_id),
                                    static_cast<int>(auth->secure_session_id_len));
    capture->peerKeyId = auth->their_keyid;
    capture->initiated = auth->initiated != 0;
    return 0;
}

class LibotrPeer
{
public:
    LibotrPeer(quint32 localInstance, quint32 peerInstance)
    {
        userState = otrl_userstate_create();
        if (!userState) {
            setupError = "otrl_userstate_create failed";
            return;
        }

        FILE *file = std::tmpfile();
        if (!file) {
            setupError = "tmpfile failed";
            return;
        }
        const size_t fixtureSize = sizeof(LibotrPrivateKey) - 1;
        const bool written = std::fwrite(LibotrPrivateKey, 1, fixtureSize, file) == fixtureSize;
        if (written)
            std::rewind(file);
        const gcry_error_t readError = written ? otrl_privkey_read_FILEp(userState, file) : gcry_error(GPG_ERR_EIO);
        std::fclose(file);
        if (readError) {
            setupError = QByteArray("otrl_privkey_read_FILEp: ") + gcry_strerror(readError);
            return;
        }

        privateKey = otrl_privkey_find(userState, LibotrAccount, Protocol);
        if (!privateKey) {
            setupError = "otrl_privkey_find failed";
            return;
        }

        int added = 0;
        context = otrl_context_find(userState,
                                    RemoteUser,
                                    LibotrAccount,
                                    Protocol,
                                    peerInstance,
                                    1,
                                    &added,
                                    nullptr,
                                    nullptr);
        if (!context) {
            setupError = "otrl_context_find failed";
            return;
        }
        context->our_instance = localInstance;
        context->their_instance = peerInstance;
        valid = true;
    }

    ~LibotrPeer()
    {
        if (userState)
            otrl_userstate_free(userState);
    }

    QByteArray ownFingerprint() const
    {
        unsigned char fingerprint[20];
        if (!otrl_privkey_fingerprint_raw(userState, fingerprint, LibotrAccount, Protocol))
            return {};
        return QByteArray(reinterpret_cast<const char *>(fingerprint), 20);
    }

    OtrlUserState userState = nullptr;
    OtrlPrivKey *privateKey = nullptr;
    ConnContext *context = nullptr;
    QByteArray setupError;
    bool valid = false;
};

} // namespace

class LibotrInteropTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void qcaInitiates();
    void libotrInitiates();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void LibotrInteropTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
    OTRL_INIT;
    QVERIFY2(QByteArray(otrl_version()).startsWith("4.1.1"), otrl_version());

    const QcaOtr::DsaPrivateKey key = qcaPrivateKey();
    QCOMPARE(QcaOtr::dsaPublicKey(key).y, expectedQcaPublicY());
}

void LibotrInteropTest::cleanupTestCase()
{
    delete initializer_;
}

void LibotrInteropTest::qcaInitiates()
{
    LibotrPeer libotr(LibotrInstance, QcaInstance);
    QVERIFY2(libotr.valid, libotr.setupError.constData());

    const QcaOtr::DsaPrivateKey qcaKey = qcaPrivateKey();
    QcaOtr::AkeSession qca(qcaKey, QcaInstance, LibotrInstance);

    bool ok = false;
    const QByteArray commit = qca.start(&ok);
    QVERIFY(ok);
    const QByteArray armoredCommit = toLibotrMessage(commit);
    QVERIFY(!armoredCommit.isEmpty());

    QCOMPARE(otrl_auth_handle_commit(&libotr.context->auth, armoredCommit.constData(), 3), gcry_error_t(0));
    const QByteArray dhKey = fromLibotrMessage(libotr.context->auth.lastauthmsg);
    QVERIFY(!dhKey.isEmpty());

    const QcaOtr::AkeHandleResult reveal = qca.processIncoming(dhKey);
    QCOMPARE(reveal.status, QcaOtr::AkeHandleStatus::Handled);
    QVERIFY(!reveal.outgoingMessage.isEmpty());

    const QByteArray armoredReveal = toLibotrMessage(reveal.outgoingMessage);
    QVERIFY(!armoredReveal.isEmpty());
    int haveMessage = 0;
    AuthCapture capture;
    QCOMPARE(otrl_auth_handle_revealsig(&libotr.context->auth,
                                        armoredReveal.constData(),
                                        &haveMessage,
                                        libotr.privateKey,
                                        authSucceeded,
                                        &capture),
             gcry_error_t(0));
    QCOMPARE(haveMessage, 1);
    QVERIFY(capture.succeeded);

    const QByteArray signature = fromLibotrMessage(libotr.context->auth.lastauthmsg);
    QVERIFY(!signature.isEmpty());
    const QcaOtr::AkeHandleResult done = qca.processIncoming(signature);
    QCOMPARE(done.status, QcaOtr::AkeHandleStatus::Authenticated);
    QVERIFY(qca.isAuthenticated());

    QCOMPARE(qca.established().sessionId, capture.sessionId);
    QCOMPARE(qca.established().peerFingerprint, libotr.ownFingerprint());
    QCOMPARE(capture.peerFingerprint,
             QcaOtr::dsaPublicKeyFingerprint(QcaOtr::dsaPublicKey(qcaKey)));
    QCOMPARE(qca.established().peerKeyId, quint32(1));
    QCOMPARE(capture.peerKeyId, quint32(1));
    QVERIFY(qca.established().initiated);
    QVERIFY(!capture.initiated);
}

void LibotrInteropTest::libotrInitiates()
{
    LibotrPeer libotr(LibotrInstance, QcaInstance);
    QVERIFY2(libotr.valid, libotr.setupError.constData());

    const QcaOtr::DsaPrivateKey qcaKey = qcaPrivateKey();
    QcaOtr::AkeSession qca(qcaKey, QcaInstance, LibotrInstance);

    QCOMPARE(otrl_auth_start_v23(&libotr.context->auth, 3), gcry_error_t(0));
    const QByteArray commit = fromLibotrMessage(libotr.context->auth.lastauthmsg);
    QVERIFY(!commit.isEmpty());

    const QcaOtr::AkeHandleResult dhKey = qca.processIncoming(commit);
    QCOMPARE(dhKey.status, QcaOtr::AkeHandleStatus::Handled);
    QVERIFY(!dhKey.outgoingMessage.isEmpty());

    const QByteArray armoredDhKey = toLibotrMessage(dhKey.outgoingMessage);
    QVERIFY(!armoredDhKey.isEmpty());
    int haveMessage = 0;
    QCOMPARE(otrl_auth_handle_key(&libotr.context->auth,
                                  armoredDhKey.constData(),
                                  &haveMessage,
                                  libotr.privateKey),
             gcry_error_t(0));
    QCOMPARE(haveMessage, 1);

    const QByteArray reveal = fromLibotrMessage(libotr.context->auth.lastauthmsg);
    QVERIFY(!reveal.isEmpty());
    const QcaOtr::AkeHandleResult signature = qca.processIncoming(reveal);
    QCOMPARE(signature.status, QcaOtr::AkeHandleStatus::Authenticated);
    QVERIFY(qca.isAuthenticated());
    QVERIFY(!signature.outgoingMessage.isEmpty());

    const QByteArray armoredSignature = toLibotrMessage(signature.outgoingMessage);
    QVERIFY(!armoredSignature.isEmpty());
    AuthCapture capture;
    haveMessage = 0;
    QCOMPARE(otrl_auth_handle_signature(&libotr.context->auth,
                                        armoredSignature.constData(),
                                        &haveMessage,
                                        authSucceeded,
                                        &capture),
             gcry_error_t(0));
    QCOMPARE(haveMessage, 0);
    QVERIFY(capture.succeeded);

    QCOMPARE(qca.established().sessionId, capture.sessionId);
    QCOMPARE(qca.established().peerFingerprint, libotr.ownFingerprint());
    QCOMPARE(capture.peerFingerprint,
             QcaOtr::dsaPublicKeyFingerprint(QcaOtr::dsaPublicKey(qcaKey)));
    QCOMPARE(qca.established().peerKeyId, quint32(1));
    QCOMPARE(capture.peerKeyId, quint32(1));
    QVERIFY(!qca.established().initiated);
    QVERIFY(capture.initiated);
}

QTEST_GUILESS_MAIN(LibotrInteropTest)
#include "libotrinteroptest.moc"
