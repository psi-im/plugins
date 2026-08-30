/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#include "qca-otr/session.h"

#include <QTest>
#include <QtCrypto>

#include <cstdio>
#include <cstdlib>

extern "C" {
#include <libotr/auth.h>
#include <libotr/context.h>
#include <libotr/instag.h>
#include <libotr/privkey.h>
#include <libotr/proto.h>
#include <libotr/tlv.h>
#include <libotr/message.h>
#include <libotr/userstate.h>
}

namespace {

constexpr quint32 QcaInstance = 0x11111111;
constexpr const char *LibotrAccount = "otrtest3";
constexpr const char *Protocol = "prpl-aim";
constexpr const char *RemoteUser = "qca-peer";
constexpr int FragmentMms = 80;

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

struct AppCapture
{
    QVector<QByteArray> injected;
    bool goneSecure = false;
    int maxMessageSize = FragmentMms;
};

OtrlPolicy appPolicy(void *, ConnContext *)
{
    return OTRL_POLICY_MANUAL;
}

int appIsLoggedIn(void *, const char *, const char *, const char *)
{
    return 1;
}

void appInjectMessage(void *data, const char *, const char *, const char *, const char *message)
{
    if (message)
        static_cast<AppCapture *>(data)->injected.append(QByteArray(message));
}

void appGoneSecure(void *data, ConnContext *)
{
    static_cast<AppCapture *>(data)->goneSecure = true;
}

int appMaxMessageSize(void *data, ConnContext *)
{
    return static_cast<AppCapture *>(data)->maxMessageSize;
}

OtrlMessageAppOps appOps()
{
    OtrlMessageAppOps ops = {};
    ops.policy = appPolicy;
    ops.is_logged_in = appIsLoggedIn;
    ops.inject_message = appInjectMessage;
    ops.gone_secure = appGoneSecure;
    ops.max_message_size = appMaxMessageSize;
    return ops;
}

class LibotrPeer
{
public:
    LibotrPeer() : ops(appOps())
    {
        userState = otrl_userstate_create();
        if (!userState) {
            setupError = "otrl_userstate_create failed";
            return;
        }

        FILE *keyFile = std::tmpfile();
        if (!keyFile) {
            setupError = "private-key tmpfile failed";
            return;
        }
        const size_t fixtureSize = sizeof(LibotrPrivateKey) - 1;
        const bool written = std::fwrite(LibotrPrivateKey, 1, fixtureSize, keyFile) == fixtureSize;
        if (written)
            std::rewind(keyFile);
        const gcry_error_t keyError = written ? otrl_privkey_read_FILEp(userState, keyFile)
                                              : gcry_error(GPG_ERR_EIO);
        std::fclose(keyFile);
        if (keyError) {
            setupError = QByteArray("otrl_privkey_read_FILEp: ") + gcry_strerror(keyError);
            return;
        }

        FILE *instagFile = std::tmpfile();
        if (!instagFile) {
            setupError = "instag tmpfile failed";
            return;
        }
        const gcry_error_t instagError =
            otrl_instag_generate_FILEp(userState, instagFile, LibotrAccount, Protocol);
        std::fclose(instagFile);
        if (instagError) {
            setupError = QByteArray("otrl_instag_generate_FILEp: ") + gcry_strerror(instagError);
            return;
        }

        const OtrlInsTag *instag = otrl_instag_find(userState, LibotrAccount, Protocol);
        if (!instag || instag->instag < OTRL_MIN_VALID_INSTAG) {
            setupError = "otrl_instag_find failed";
            return;
        }
        localInstance = instag->instag;

        int added = 0;
        masterContext = otrl_context_find(userState,
                                          RemoteUser,
                                          LibotrAccount,
                                          Protocol,
                                          OTRL_INSTAG_MASTER,
                                          1,
                                          &added,
                                          nullptr,
                                          nullptr);
        if (!masterContext) {
            setupError = "master otrl_context_find failed";
            return;
        }
        masterContext->our_instance = localInstance;
        valid = true;
    }

    ~LibotrPeer()
    {
        if (userState)
            otrl_userstate_free(userState);
    }

    QVector<QByteArray> receive(const QVector<QByteArray> &messages, QByteArray *plaintext = nullptr)
    {
        QVector<QByteArray> allInjected;
        if (plaintext)
            plaintext->clear();

        for (const QByteArray &message : messages) {
            capture.injected.clear();
            char *newMessage = nullptr;
            OtrlTLV *tlvs = nullptr;
            ConnContext *usedContext = nullptr;
            const int internal = otrl_message_receiving(userState,
                                                        &ops,
                                                        &capture,
                                                        LibotrAccount,
                                                        Protocol,
                                                        RemoteUser,
                                                        message.constData(),
                                                        &newMessage,
                                                        &tlvs,
                                                        &usedContext,
                                                        nullptr,
                                                        nullptr);
            if (usedContext && usedContext->their_instance == QcaInstance)
                secureContext = usedContext;
            allInjected += capture.injected;

            if (!internal && newMessage && plaintext)
                *plaintext = QByteArray(newMessage);
            if (newMessage)
                otrl_message_free(newMessage);
            if (tlvs)
                otrl_tlv_free(tlvs);
        }
        return allInjected;
    }

    bool sendPlaintext(const QByteArray &plaintext, QVector<QByteArray> *messages)
    {
        if (!messages)
            return false;
        messages->clear();
        capture.injected.clear();

        char *returnedMessage = nullptr;
        ConnContext *usedContext = nullptr;
        const gcry_error_t error = otrl_message_sending(userState,
                                                        &ops,
                                                        &capture,
                                                        LibotrAccount,
                                                        Protocol,
                                                        RemoteUser,
                                                        QcaInstance,
                                                        plaintext.constData(),
                                                        nullptr,
                                                        &returnedMessage,
                                                        OTRL_FRAGMENT_SEND_ALL,
                                                        &usedContext,
                                                        nullptr,
                                                        nullptr);
        if (usedContext)
            secureContext = usedContext;
        if (returnedMessage) {
            // SEND_ALL normally injects every fragment, but preserve a returned
            // message as well in case libotr decides no fragmentation is needed.
            if (capture.injected.isEmpty())
                capture.injected.append(QByteArray(returnedMessage));
            otrl_message_free(returnedMessage);
        }
        if (error)
            return false;

        *messages = capture.injected;
        return !messages->isEmpty();
    }

    bool isEncrypted() const
    {
        return secureContext && secureContext->msgstate == OTRL_MSGSTATE_ENCRYPTED;
    }

    OtrlUserState userState = nullptr;
    ConnContext *masterContext = nullptr;
    ConnContext *secureContext = nullptr;
    OtrlMessageAppOps ops = {};
    AppCapture capture;
    QByteArray setupError;
    quint32 localInstance = 0;
    bool valid = false;
};

QVector<QByteArray> deliverToQca(QcaOtr::OtrSession *qca,
                                 const QVector<QByteArray> &messages,
                                 QcaOtr::SessionResult *last = nullptr)
{
    QVector<QByteArray> outgoing;
    QcaOtr::SessionResult current;
    for (const QByteArray &message : messages) {
        current = qca->processIncoming(message, FragmentMms);
        outgoing += current.outgoingMessages;
    }
    if (last)
        *last = current;
    return outgoing;
}

bool finishLibotrResponderHandshake(QcaOtr::OtrSession *qca,
                                    LibotrPeer *libotr,
                                    const QVector<QByteArray> &dhKey)
{
    QcaOtr::SessionResult state;
    QVector<QByteArray> messages = deliverToQca(qca, dhKey, &state);
    if (state.status != QcaOtr::SessionStatus::Handled || messages.isEmpty())
        return false;

    messages = libotr->receive(messages);
    if (messages.isEmpty() || !libotr->isEncrypted())
        return false;

    messages = deliverToQca(qca, messages, &state);
    return state.status == QcaOtr::SessionStatus::Authenticated && messages.isEmpty() &&
        qca->isEncrypted(libotr->localInstance);
}

} // namespace

class SessionInteropTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void fragmentedPublicApiExchange();
    void broadcastToMultipleLibotrInstances();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void SessionInteropTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
    OTRL_INIT;
    QVERIFY2(QByteArray(otrl_version()).startsWith("4.1.1"), otrl_version());
}

void SessionInteropTest::cleanupTestCase()
{
    delete initializer_;
}

void SessionInteropTest::fragmentedPublicApiExchange()
{
    LibotrPeer libotr;
    QVERIFY2(libotr.valid, libotr.setupError.constData());

    QcaOtr::OtrSession qca(qcaPrivateKey(), QcaInstance);
    bool ok = false;
    QVector<QByteArray> messages = qca.start(libotr.localInstance, FragmentMms, &ok);
    QVERIFY(ok);
    QVERIFY(messages.size() > 1);

    // QCA Commit -> libotr D-H Key.
    messages = libotr.receive(messages);
    QVERIFY(messages.size() > 1);

    QVERIFY(finishLibotrResponderHandshake(&qca, &libotr, messages));

    // QCA -> libotr: force a large encrypted Data Message through fragments.
    const QByteArray fromQca = QByteArray("qca-fragmented:") + QByteArray(500, 'q');
    QVERIFY(qca.sendMessage(libotr.localInstance, fromQca, &messages, FragmentMms));
    QVERIFY(messages.size() > 1);
    QByteArray plaintext;
    const QVector<QByteArray> protocolReply = libotr.receive(messages, &plaintext);
    QVERIFY(protocolReply.isEmpty());
    QCOMPARE(plaintext, fromQca);

    // libotr -> QCA: public message_sending uses max_message_size and injects
    // the exact libotr fragment stream into our transport-facing session.
    const QByteArray fromLibotr = QByteArray("libotr-fragmented:") + QByteArray(500, 'l');
    QVERIFY(libotr.sendPlaintext(fromLibotr, &messages));
    QVERIFY(messages.size() > 1);
    QcaOtr::SessionResult state;
    deliverToQca(&qca, messages, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Message);
    QCOMPARE(state.plaintext, fromLibotr);
}

void SessionInteropTest::broadcastToMultipleLibotrInstances()
{
    LibotrPeer first;
    LibotrPeer second;
    QVERIFY2(first.valid, first.setupError.constData());
    QVERIFY2(second.valid, second.setupError.constData());
    QVERIFY(first.localInstance != second.localInstance);

    QcaOtr::OtrSession qca(qcaPrivateKey(), QcaInstance);
    bool ok = false;
    const QVector<QByteArray> broadcastCommit = qca.start(0, FragmentMms, &ok);
    QVERIFY(ok);
    QVERIFY(broadcastCommit.size() > 1);

    // Both independent libotr user states answer the exact same receiver=0
    // Commit. QCA must retain the master AKE long enough to clone it into two
    // independent child contexts rather than binding the first response globally.
    const QVector<QByteArray> firstDh = first.receive(broadcastCommit);
    const QVector<QByteArray> secondDh = second.receive(broadcastCommit);
    QVERIFY(firstDh.size() > 1);
    QVERIFY(secondDh.size() > 1);

    QVERIFY(finishLibotrResponderHandshake(&qca, &first, firstDh));
    QVERIFY(finishLibotrResponderHandshake(&qca, &second, secondDh));
    QVERIFY(qca.isEncrypted(first.localInstance));
    QVERIFY(qca.isEncrypted(second.localInstance));

    QVector<QByteArray> messages;
    QByteArray plaintext;
    QVERIFY(qca.sendMessage(first.localInstance, "first libotr instance", &messages, FragmentMms));
    first.receive(messages, &plaintext);
    QCOMPARE(plaintext, QByteArray("first libotr instance"));

    QVERIFY(qca.sendMessage(second.localInstance, "second libotr instance", &messages, FragmentMms));
    second.receive(messages, &plaintext);
    QCOMPARE(plaintext, QByteArray("second libotr instance"));

    QVERIFY(first.sendPlaintext("reply from first", &messages));
    QcaOtr::SessionResult state;
    deliverToQca(&qca, messages, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Message);
    QCOMPARE(state.peerInstance, first.localInstance);
    QCOMPARE(state.plaintext, QByteArray("reply from first"));

    QVERIFY(second.sendPlaintext("reply from second", &messages));
    deliverToQca(&qca, messages, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Message);
    QCOMPARE(state.peerInstance, second.localInstance);
    QCOMPARE(state.plaintext, QByteArray("reply from second"));
}

QTEST_GUILESS_MAIN(SessionInteropTest)
#include "sessioninteroptest.moc"
