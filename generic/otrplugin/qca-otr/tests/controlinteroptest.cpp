#include "libotrtest.h"
#include "qca-otr/data.h"
#include "qca-otr/negotiation.h"
#include "qca-otr/session.h"
#include "qca-otr/transport.h"

#include <QTest>
#include <QtCrypto>

#include <cstdio>

namespace {

constexpr quint32 QcaInstance = 0x11111111;
constexpr const char *LibotrAccount = "otrtest3";
constexpr const char *Protocol = "prpl-aim";
constexpr const char *RemoteUser = "qca-peer";
constexpr int ExtraKeySize = 32;

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
    const QByteArray bytes = QByteArray::fromHex(hex);
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

struct CapturedTlv
{
    quint16 type = 0;
    QByteArray value;
};

struct AppCapture
{
    OtrlPolicy policy = OTRL_POLICY_MANUAL;
    QVector<QByteArray> injected;
    QVector<OtrlMessageEvent> events;
    QByteArray eventMessage;
    bool receivedSymkey = false;
    quint32 symkeyUse = 0;
    QByteArray symkeyData;
    QByteArray symkey;
};

OtrlPolicy appPolicy(void *data, ConnContext *)
{
    return static_cast<AppCapture *>(data)->policy;
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

void appReceivedSymkey(void *data,
                       ConnContext *,
                       unsigned int use,
                       const unsigned char *useData,
                       size_t useDataLength,
                       const unsigned char *symkey)
{
    auto *capture = static_cast<AppCapture *>(data);
    capture->receivedSymkey = true;
    capture->symkeyUse = use;
    capture->symkeyData = QByteArray(reinterpret_cast<const char *>(useData),
                                     static_cast<int>(useDataLength));
    capture->symkey = QByteArray(reinterpret_cast<const char *>(symkey), ExtraKeySize);
}

void appHandleMessageEvent(void *data,
                           OtrlMessageEvent event,
                           ConnContext *,
                           const char *message,
                           gcry_error_t)
{
    auto *capture = static_cast<AppCapture *>(data);
    capture->events.append(event);
    if (message)
        capture->eventMessage = QByteArray(message);
}

const char *appErrorMessage(void *, ConnContext *, OtrlErrorCode)
{
    return "oracle error";
}

OtrlMessageAppOps appOps()
{
    OtrlMessageAppOps ops = {};
    ops.policy = appPolicy;
    ops.is_logged_in = appIsLoggedIn;
    ops.inject_message = appInjectMessage;
    ops.received_symkey = appReceivedSymkey;
    ops.handle_msg_event = appHandleMessageEvent;
    ops.otr_error_message = appErrorMessage;
    return ops;
}

class LibotrPeer
{
public:
    explicit LibotrPeer(OtrlPolicy policy = OTRL_POLICY_MANUAL) : ops(appOps())
    {
        capture.policy = policy;
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

    void setPolicy(OtrlPolicy policy)
    {
        capture.policy = policy;
    }

    QVector<QByteArray> receive(const QVector<QByteArray> &messages, QByteArray *plaintext = nullptr)
    {
        QVector<QByteArray> injected;
        receivedTlvs.clear();
        capture.events.clear();
        capture.eventMessage.clear();
        capture.receivedSymkey = false;
        capture.symkeyUse = 0;
        capture.symkeyData.clear();
        capture.symkey.clear();
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
            injected += capture.injected;

            if (!internal && newMessage && plaintext)
                *plaintext = QByteArray(newMessage);
            for (OtrlTLV *tlv = tlvs; tlv; tlv = tlv->next) {
                CapturedTlv captured;
                captured.type = tlv->type;
                captured.value = QByteArray(reinterpret_cast<const char *>(tlv->data), tlv->len);
                receivedTlvs.append(captured);
            }
            if (newMessage)
                otrl_message_free(newMessage);
            if (tlvs)
                otrl_tlv_free(tlvs);
        }
        return injected;
    }

    bool send(const QByteArray &plaintext, OtrlTLV *tlvs, QVector<QByteArray> *messages)
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
                                                        tlvs,
                                                        &returnedMessage,
                                                        OTRL_FRAGMENT_SEND_SKIP,
                                                        &usedContext,
                                                        nullptr,
                                                        nullptr);
        if (usedContext && usedContext->their_instance == QcaInstance)
            secureContext = usedContext;
        if (returnedMessage) {
            messages->append(QByteArray(returnedMessage));
            otrl_message_free(returnedMessage);
        }
        *messages += capture.injected;
        return !error && !messages->isEmpty();
    }

    bool sendPlaintext(const QByteArray &plaintext, QVector<QByteArray> *messages)
    {
        return send(plaintext, nullptr, messages);
    }

    bool disconnect(QVector<QByteArray> *messages)
    {
        if (!messages)
            return false;
        messages->clear();
        capture.injected.clear();
        otrl_message_disconnect(userState,
                                &ops,
                                &capture,
                                LibotrAccount,
                                Protocol,
                                RemoteUser,
                                QcaInstance);
        *messages = capture.injected;
        return !messages->isEmpty();
    }

    bool sendSymmetricKey(quint32 use,
                          const QByteArray &useData,
                          QByteArray *key,
                          QVector<QByteArray> *messages)
    {
        if (!key || !messages || !secureContext)
            return false;
        key->fill('\0', ExtraKeySize);
        messages->clear();
        capture.injected.clear();
        const gcry_error_t error = otrl_message_symkey(
            userState,
            &ops,
            &capture,
            secureContext,
            use,
            reinterpret_cast<const unsigned char *>(useData.constData()),
            static_cast<size_t>(useData.size()),
            reinterpret_cast<unsigned char *>(key->data()));
        *messages = capture.injected;
        return !error && !messages->isEmpty();
    }

    bool isEncrypted() const
    {
        return secureContext && secureContext->msgstate == OTRL_MSGSTATE_ENCRYPTED;
    }

    OtrlMessageState messageState() const
    {
        return secureContext ? secureContext->msgstate : OTRL_MSGSTATE_PLAINTEXT;
    }

    OtrlUserState userState = nullptr;
    ConnContext *masterContext = nullptr;
    ConnContext *secureContext = nullptr;
    OtrlMessageAppOps ops = {};
    AppCapture capture;
    QVector<CapturedTlv> receivedTlvs;
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
        current = qca->processIncoming(message);
        outgoing += current.outgoingMessages;
    }
    if (last)
        *last = current;
    return outgoing;
}

bool finishQcaInitiatedHandshake(QcaOtr::OtrSession *qca,
                                 LibotrPeer *libotr,
                                 const QVector<QByteArray> &commit)
{
    QVector<QByteArray> messages = libotr->receive(commit);
    if (messages.isEmpty())
        return false;

    QcaOtr::SessionResult state;
    messages = deliverToQca(qca, messages, &state);
    if (state.status != QcaOtr::SessionStatus::Handled || messages.isEmpty())
        return false;

    messages = libotr->receive(messages);
    if (messages.isEmpty() || !libotr->isEncrypted())
        return false;

    messages = deliverToQca(qca, messages, &state);
    return state.status == QcaOtr::SessionStatus::Authenticated && messages.isEmpty() &&
        qca->isEncrypted(libotr->localInstance);
}

bool finishLibotrInitiatedHandshake(QcaOtr::OtrSession *qca,
                                    LibotrPeer *libotr,
                                    const QVector<QByteArray> &commit)
{
    QcaOtr::SessionResult state;
    QVector<QByteArray> messages = deliverToQca(qca, commit, &state);
    if (state.status != QcaOtr::SessionStatus::Handled || messages.isEmpty())
        return false;

    messages = libotr->receive(messages);
    if (messages.isEmpty())
        return false;

    messages = deliverToQca(qca, messages, &state);
    if (state.status != QcaOtr::SessionStatus::Authenticated || messages.isEmpty() ||
        !qca->isEncrypted(libotr->localInstance)) {
        return false;
    }

    messages = libotr->receive(messages);
    return messages.isEmpty() && libotr->isEncrypted();
}

bool establish(QcaOtr::OtrSession *qca, LibotrPeer *libotr)
{
    bool ok = false;
    const QVector<QByteArray> commit = qca->start(libotr->localInstance, 0, &ok);
    return ok && !commit.isEmpty() && finishQcaInitiatedHandshake(qca, libotr, commit);
}

} // namespace

class ControlInteropTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void queryNegotiationBothDirections();
    void whitespaceNegotiationBothDirections();
    void tlvSymkeyAndDisconnectBothDirections();
    void protocolErrorsBothDirections();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void ControlInteropTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
    OTRL_INIT;
    QVERIFY2(QByteArray(otrl_version()).startsWith("4.1.1"), otrl_version());
}

void ControlInteropTest::cleanupTestCase()
{
    delete initializer_;
}

void ControlInteropTest::queryNegotiationBothDirections()
{
    {
        LibotrPeer libotr;
        QVERIFY2(libotr.valid, libotr.setupError.constData());
        QcaOtr::OtrSession qca(qcaPrivateKey(), QcaInstance);

        const QcaOtr::OutgoingResult query = qca.startNegotiation("qca-peer");
        QCOMPARE(query.status, QcaOtr::OutgoingStatus::Negotiation);
        QCOMPARE(query.messages.size(), 1);
        QVERIFY(query.messages.front().startsWith("?OTRv3?"));

        const QVector<QByteArray> commit = libotr.receive(query.messages);
        QVERIFY(!commit.isEmpty());
        QVERIFY(finishLibotrInitiatedHandshake(&qca, &libotr, commit));
    }

    {
        LibotrPeer libotr;
        QVERIFY2(libotr.valid, libotr.setupError.constData());
        QcaOtr::OtrSession qca(qcaPrivateKey(), QcaInstance);

        QVector<QByteArray> query;
        QVERIFY(libotr.sendPlaintext("?OTR?", &query));
        QCOMPARE(query.size(), 1);
        QVERIFY(query.front().startsWith("?OTRv23?"));

        const QcaOtr::SessionResult received = qca.processIncoming(query.front());
        QCOMPARE(received.status, QcaOtr::SessionStatus::ProtocolMessage);
        QVERIFY(!received.outgoingMessages.isEmpty());
        QVERIFY(finishQcaInitiatedHandshake(&qca, &libotr, received.outgoingMessages));
    }
}

void ControlInteropTest::whitespaceNegotiationBothDirections()
{
    {
        LibotrPeer libotr(OTRL_POLICY_OPPORTUNISTIC);
        QVERIFY2(libotr.valid, libotr.setupError.constData());
        QcaOtr::OtrSession qca(qcaPrivateKey(), QcaInstance);
        qca.setPolicy(QcaOtr::SessionPolicy::Opportunistic);

        const QcaOtr::OutgoingResult tagged = qca.prepareOutgoing("qca whitespace");
        QCOMPARE(tagged.status, QcaOtr::OutgoingStatus::Plaintext);
        QCOMPARE(tagged.messages.size(), 1);

        QByteArray plaintext;
        const QVector<QByteArray> commit = libotr.receive(tagged.messages, &plaintext);
        QCOMPARE(plaintext, QByteArray("qca whitespace"));
        QVERIFY(!commit.isEmpty());
        QVERIFY(finishLibotrInitiatedHandshake(&qca, &libotr, commit));
    }

    {
        LibotrPeer libotr(OTRL_POLICY_OPPORTUNISTIC);
        QVERIFY2(libotr.valid, libotr.setupError.constData());
        QcaOtr::OtrSession qca(qcaPrivateKey(), QcaInstance);
        qca.setPolicy(QcaOtr::SessionPolicy::Opportunistic);

        QVector<QByteArray> tagged;
        QVERIFY(libotr.sendPlaintext("libotr whitespace", &tagged));
        QCOMPARE(tagged.size(), 1);
        QVERIFY(tagged.front().contains(OTRL_MESSAGE_TAG_BASE));

        const QcaOtr::SessionResult received = qca.processIncoming(tagged.front());
        QCOMPARE(received.status, QcaOtr::SessionStatus::Plaintext);
        QCOMPARE(received.plaintext, QByteArray("libotr whitespace"));
        QVERIFY(!received.outgoingMessages.isEmpty());
        QVERIFY(finishQcaInitiatedHandshake(&qca, &libotr, received.outgoingMessages));
    }
}

void ControlInteropTest::tlvSymkeyAndDisconnectBothDirections()
{
    LibotrPeer libotr;
    QVERIFY2(libotr.valid, libotr.setupError.constData());
    QcaOtr::OtrSession qca(qcaPrivateKey(), QcaInstance);
    QVERIFY(establish(&qca, &libotr));

    QVector<QByteArray> messages;
    QVector<QcaOtr::Tlv> qcaTlvs;
    QcaOtr::Tlv qcaTlv;
    qcaTlv.type = 0x1234;
    qcaTlv.value = QByteArray::fromHex("0001ff00");
    qcaTlvs.append(qcaTlv);
    QVERIFY(qca.sendMessage(libotr.localInstance, "qca tlv", qcaTlvs, &messages));
    QByteArray plaintext;
    libotr.receive(messages, &plaintext);
    QCOMPARE(plaintext, QByteArray("qca tlv"));
    QCOMPARE(libotr.receivedTlvs.size(), 1);
    QCOMPARE(libotr.receivedTlvs.front().type, quint16(0x1234));
    QCOMPARE(libotr.receivedTlvs.front().value, qcaTlv.value);

    const QByteArray libotrTlvValue = QByteArray::fromHex("00fe0200");
    OtrlTLV *libotrTlv = otrl_tlv_new(0x4321,
                                     static_cast<unsigned short>(libotrTlvValue.size()),
                                     reinterpret_cast<const unsigned char *>(libotrTlvValue.constData()));
    QVERIFY(libotrTlv != nullptr);
    QVERIFY(libotr.send("libotr tlv", libotrTlv, &messages));
    otrl_tlv_free(libotrTlv);
    QcaOtr::SessionResult received = qca.processIncoming(messages.front());
    QCOMPARE(received.status, QcaOtr::SessionStatus::Message);
    QCOMPARE(received.plaintext, QByteArray("libotr tlv"));
    QCOMPARE(received.tlvs.size(), 1);
    QCOMPARE(received.tlvs.front().type, quint16(0x4321));
    QCOMPARE(received.tlvs.front().value, libotrTlvValue);

    const QByteArray qcaUseData = QByteArray::fromHex("61006200ff");
    QCA::SecureArray qcaKey;
    QVERIFY(qca.sendSymmetricKey(libotr.localInstance,
                                 0x01020304,
                                 qcaUseData,
                                 &qcaKey,
                                 &messages));
    libotr.receive(messages);
    QVERIFY(libotr.capture.receivedSymkey);
    QCOMPARE(libotr.capture.symkeyUse, quint32(0x01020304));
    QCOMPARE(libotr.capture.symkeyData, qcaUseData);
    QCOMPARE(libotr.capture.symkey, qcaKey.toByteArray());

    const QByteArray libotrUseData = QByteArray::fromHex("0001020300");
    QByteArray libotrKey;
    QVERIFY(libotr.sendSymmetricKey(0xa1b2c3d4, libotrUseData, &libotrKey, &messages));
    received = qca.processIncoming(messages.front());
    QCOMPARE(received.status, QcaOtr::SessionStatus::Message);
    QVERIFY(received.hasSymmetricKey);
    QCOMPARE(received.symmetricKeyUse, quint32(0xa1b2c3d4));
    QCOMPARE(received.symmetricKeyData, libotrUseData);
    QCOMPARE(received.symmetricKey.toByteArray(), libotrKey);

    QVERIFY(qca.disconnect(libotr.localInstance, &messages));
    libotr.receive(messages);
    QCOMPARE(libotr.messageState(), OTRL_MSGSTATE_FINISHED);
    QCOMPARE(qca.peerState(libotr.localInstance), QcaOtr::PeerState::Plaintext);

    const QcaOtr::OutgoingResult restart = qca.startNegotiation("qca-peer");
    QCOMPARE(restart.status, QcaOtr::OutgoingStatus::Negotiation);
    const QVector<QByteArray> restartCommit = libotr.receive(restart.messages);
    QVERIFY(!restartCommit.isEmpty());
    QVERIFY(finishLibotrInitiatedHandshake(&qca, &libotr, restartCommit));

    QVERIFY(libotr.disconnect(&messages));
    QCOMPARE(libotr.messageState(), OTRL_MSGSTATE_PLAINTEXT);
    received = qca.processIncoming(messages.front());
    QCOMPARE(received.status, QcaOtr::SessionStatus::Disconnected);
    QCOMPARE(qca.peerState(libotr.localInstance), QcaOtr::PeerState::Finished);
}

void ControlInteropTest::protocolErrorsBothDirections()
{
    LibotrPeer libotr;
    QVERIFY2(libotr.valid, libotr.setupError.constData());
    QcaOtr::OtrSession qca(qcaPrivateKey(), QcaInstance);

    libotr.receive(QVector<QByteArray>{QcaOtr::Negotiation::errorMessage("from qca")});
    QVERIFY(libotr.capture.events.contains(OTRL_MSGEVENT_RCVDMSG_GENERAL_ERR));
    QVERIFY(libotr.capture.eventMessage.contains("from qca"));

    QVERIFY(establish(&qca, &libotr));
    QVector<QByteArray> messages;
    QVERIFY(qca.sendMessage(libotr.localInstance, "tamper for libotr", &messages));
    QCOMPARE(messages.size(), 1);

    QByteArray raw;
    QVERIFY(QcaOtr::Transport::dearmor(messages.front(), &raw));
    QcaOtr::DataMessage data;
    QVERIFY(QcaOtr::Wire::decodeDataMessage(raw, &data));
    QVERIFY(!data.mac.isEmpty());
    data.mac[0] = static_cast<char>(data.mac.at(0) ^ 0x01);
    bool ok = false;
    raw = QcaOtr::Wire::encodeDataMessage(data, &ok);
    QVERIFY(ok);

    const QVector<QByteArray> errors =
        libotr.receive(QVector<QByteArray>{QcaOtr::Transport::armor(raw)});
    QVERIFY(libotr.capture.events.contains(OTRL_MSGEVENT_RCVDMSG_UNREADABLE) ||
            libotr.capture.events.contains(OTRL_MSGEVENT_RCVDMSG_MALFORMED));
    QCOMPARE(errors.size(), 1);
    QVERIFY(errors.front().startsWith("?OTR Error: oracle error"));

    const QcaOtr::SessionResult received = qca.processIncoming(errors.front());
    QCOMPARE(received.status, QcaOtr::SessionStatus::RemoteError);
    QCOMPARE(received.errorText, QByteArray("oracle error"));
}

QTEST_GUILESS_MAIN(ControlInteropTest)
#include "controlinteroptest.moc"
