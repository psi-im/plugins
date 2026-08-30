#include "libotrtest.h"
#include "qca-otr/session.h"

#include <QTest>
#include <QtCrypto>

#include <cstdio>

namespace {

constexpr quint32 QcaInstance = 0x11111111;
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

struct AppCapture
{
    OtrlPolicy policy = OTRL_POLICY_MANUAL;
    QVector<QByteArray> injected;
    QVector<OtrlSMPEvent> smpEvents;
    unsigned short smpProgress = 0;
    QByteArray smpQuestion;
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

void appHandleSmpEvent(void *data,
                       OtrlSMPEvent event,
                       ConnContext *,
                       unsigned short progress,
                       char *question)
{
    auto *capture = static_cast<AppCapture *>(data);
    capture->smpEvents.append(event);
    capture->smpProgress = progress;
    capture->smpQuestion = question ? QByteArray(question) : QByteArray();
}

OtrlMessageAppOps appOps()
{
    OtrlMessageAppOps ops = {};
    ops.policy = appPolicy;
    ops.is_logged_in = appIsLoggedIn;
    ops.inject_message = appInjectMessage;
    ops.handle_smp_event = appHandleSmpEvent;
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
        valid = true;
    }

    ~LibotrPeer()
    {
        if (userState)
            otrl_userstate_free(userState);
    }

    void clearSmpCapture()
    {
        capture.smpEvents.clear();
        capture.smpProgress = 0;
        capture.smpQuestion.clear();
    }

    bool hasSmpEvent(OtrlSMPEvent event) const
    {
        return capture.smpEvents.contains(event);
    }

    QVector<QByteArray> receive(const QVector<QByteArray> &messages)
    {
        QVector<QByteArray> injected;
        clearSmpCapture();
        for (const QByteArray &message : messages) {
            capture.injected.clear();
            char *newMessage = nullptr;
            OtrlTLV *tlvs = nullptr;
            ConnContext *usedContext = nullptr;
            otrl_message_receiving(userState,
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
            if (newMessage)
                otrl_message_free(newMessage);
            if (tlvs)
                otrl_tlv_free(tlvs);
        }
        return injected;
    }

    bool initiateSmp(const QByteArray &secret, QVector<QByteArray> *messages)
    {
        if (!messages || !secureContext)
            return false;
        messages->clear();
        clearSmpCapture();
        capture.injected.clear();
        otrl_message_initiate_smp(userState,
                                  &ops,
                                  &capture,
                                  secureContext,
                                  reinterpret_cast<const unsigned char *>(secret.constData()),
                                  static_cast<size_t>(secret.size()));
        *messages = capture.injected;
        return !messages->isEmpty();
    }

    bool initiateSmp(const QByteArray &question,
                     const QByteArray &secret,
                     QVector<QByteArray> *messages)
    {
        if (!messages || !secureContext)
            return false;
        messages->clear();
        clearSmpCapture();
        capture.injected.clear();
        otrl_message_initiate_smp_q(userState,
                                    &ops,
                                    &capture,
                                    secureContext,
                                    question.constData(),
                                    reinterpret_cast<const unsigned char *>(secret.constData()),
                                    static_cast<size_t>(secret.size()));
        *messages = capture.injected;
        return !messages->isEmpty();
    }

    bool respondSmp(const QByteArray &secret, QVector<QByteArray> *messages)
    {
        if (!messages || !secureContext)
            return false;
        messages->clear();
        clearSmpCapture();
        capture.injected.clear();
        otrl_message_respond_smp(userState,
                                 &ops,
                                 &capture,
                                 secureContext,
                                 reinterpret_cast<const unsigned char *>(secret.constData()),
                                 static_cast<size_t>(secret.size()));
        *messages = capture.injected;
        return !messages->isEmpty();
    }

    bool abortSmp(QVector<QByteArray> *messages)
    {
        if (!messages || !secureContext)
            return false;
        messages->clear();
        clearSmpCapture();
        capture.injected.clear();
        otrl_message_abort_smp(userState, &ops, &capture, secureContext);
        *messages = capture.injected;
        return !messages->isEmpty();
    }

    bool isEncrypted() const
    {
        return secureContext && secureContext->msgstate == OTRL_MSGSTATE_ENCRYPTED;
    }

    OtrlUserState userState = nullptr;
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
        current = qca->processIncoming(message);
        outgoing += current.outgoingMessages;
    }
    if (last)
        *last = current;
    return outgoing;
}

bool establish(QcaOtr::OtrSession *qca, LibotrPeer *libotr)
{
    bool ok = false;
    QVector<QByteArray> messages = qca->start(libotr->localInstance, 0, &ok);
    if (!ok || messages.isEmpty())
        return false;

    messages = libotr->receive(messages);
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

QCA::SecureArray secure(const QByteArray &value)
{
    return QCA::SecureArray(value);
}

} // namespace

class SmpInteropTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void qcaInitiatedQuestionSucceeds();
    void libotrInitiatedSucceeds();
    void differentSecretsFail();
    void abortBothDirections();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void SmpInteropTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
    OTRL_INIT;
    QVERIFY2(QByteArray(otrl_version()).startsWith("4.1.1"), otrl_version());
}

void SmpInteropTest::cleanupTestCase()
{
    delete initializer_;
}

void SmpInteropTest::qcaInitiatedQuestionSucceeds()
{
    LibotrPeer libotr;
    QVERIFY2(libotr.valid, libotr.setupError.constData());
    QcaOtr::OtrSession qca(qcaPrivateKey(), QcaInstance);
    QVERIFY(establish(&qca, &libotr));

    const QByteArray answer("shared answer");
    const QByteArray question("What is our shared answer?");
    QVector<QByteArray> messages;
    QVERIFY(qca.startSmp(libotr.localInstance, question, secure(answer), &messages));
    QCOMPARE(messages.size(), 1);

    messages = libotr.receive(messages);
    QVERIFY(messages.isEmpty());
    QVERIFY(libotr.hasSmpEvent(OTRL_SMPEVENT_ASK_FOR_ANSWER));
    QCOMPARE(libotr.capture.smpProgress, static_cast<unsigned short>(25));
    QCOMPARE(libotr.capture.smpQuestion, question);

    QVERIFY(libotr.respondSmp(answer, &messages));
    QcaOtr::SessionResult state;
    messages = deliverToQca(&qca, messages, &state);
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::InProgress);
    QCOMPARE(state.smpProgress, quint16(60));
    QCOMPARE(messages.size(), 1);

    messages = libotr.receive(messages);
    QVERIFY(libotr.hasSmpEvent(OTRL_SMPEVENT_SUCCESS));
    QCOMPARE(libotr.capture.smpProgress, static_cast<unsigned short>(100));
    QCOMPARE(messages.size(), 1);

    messages = deliverToQca(&qca, messages, &state);
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::Success);
    QCOMPARE(state.smpProgress, quint16(100));
    QVERIFY(messages.isEmpty());
}

void SmpInteropTest::libotrInitiatedSucceeds()
{
    LibotrPeer libotr;
    QVERIFY2(libotr.valid, libotr.setupError.constData());
    QcaOtr::OtrSession qca(qcaPrivateKey(), QcaInstance);
    QVERIFY(establish(&qca, &libotr));

    const QByteArray answer("same answer");
    QVector<QByteArray> messages;
    QVERIFY(libotr.initiateSmp(answer, &messages));
    QCOMPARE(messages.size(), 1);

    QcaOtr::SessionResult state;
    messages = deliverToQca(&qca, messages, &state);
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::AskForSecret);
    QCOMPARE(state.smpProgress, quint16(25));
    QVERIFY(messages.isEmpty());

    QVERIFY(qca.respondSmp(libotr.localInstance, secure(answer), &messages));
    QCOMPARE(messages.size(), 1);

    messages = libotr.receive(messages);
    QVERIFY(libotr.hasSmpEvent(OTRL_SMPEVENT_IN_PROGRESS));
    QCOMPARE(libotr.capture.smpProgress, static_cast<unsigned short>(60));
    QCOMPARE(messages.size(), 1);

    messages = deliverToQca(&qca, messages, &state);
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::Success);
    QCOMPARE(state.smpProgress, quint16(100));
    QCOMPARE(messages.size(), 1);

    messages = libotr.receive(messages);
    QVERIFY(libotr.hasSmpEvent(OTRL_SMPEVENT_SUCCESS));
    QCOMPARE(libotr.capture.smpProgress, static_cast<unsigned short>(100));
    QVERIFY(messages.isEmpty());
}

void SmpInteropTest::differentSecretsFail()
{
    LibotrPeer libotr;
    QVERIFY2(libotr.valid, libotr.setupError.constData());
    QcaOtr::OtrSession qca(qcaPrivateKey(), QcaInstance);
    QVERIFY(establish(&qca, &libotr));

    QVector<QByteArray> messages;
    QVERIFY(qca.startSmp(libotr.localInstance, secure(QByteArray("qca answer")), &messages));
    messages = libotr.receive(messages);
    QVERIFY(messages.isEmpty());
    QVERIFY(libotr.hasSmpEvent(OTRL_SMPEVENT_ASK_FOR_SECRET));

    QVERIFY(libotr.respondSmp(QByteArray("different answer"), &messages));
    QcaOtr::SessionResult state;
    messages = deliverToQca(&qca, messages, &state);
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::InProgress);
    QCOMPARE(messages.size(), 1);

    messages = libotr.receive(messages);
    QVERIFY(libotr.hasSmpEvent(OTRL_SMPEVENT_FAILURE));
    QCOMPARE(libotr.capture.smpProgress, static_cast<unsigned short>(100));
    QCOMPARE(messages.size(), 1);

    messages = deliverToQca(&qca, messages, &state);
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::Failure);
    QCOMPARE(state.smpProgress, quint16(100));
    QVERIFY(messages.isEmpty());
}

void SmpInteropTest::abortBothDirections()
{
    {
        LibotrPeer libotr;
        QVERIFY2(libotr.valid, libotr.setupError.constData());
        QcaOtr::OtrSession qca(qcaPrivateKey(), QcaInstance);
        QVERIFY(establish(&qca, &libotr));

        QVector<QByteArray> messages;
        QVERIFY(qca.startSmp(libotr.localInstance, secure(QByteArray("answer")), &messages));
        messages = libotr.receive(messages);
        QVERIFY(messages.isEmpty());
        QVERIFY(libotr.hasSmpEvent(OTRL_SMPEVENT_ASK_FOR_SECRET));

        QVERIFY(qca.abortSmp(libotr.localInstance, &messages));
        messages = libotr.receive(messages);
        QVERIFY(libotr.hasSmpEvent(OTRL_SMPEVENT_ABORT));
        QVERIFY(messages.isEmpty());
    }

    {
        LibotrPeer libotr;
        QVERIFY2(libotr.valid, libotr.setupError.constData());
        QcaOtr::OtrSession qca(qcaPrivateKey(), QcaInstance);
        QVERIFY(establish(&qca, &libotr));

        QVector<QByteArray> messages;
        QVERIFY(libotr.initiateSmp(QByteArray("answer"), &messages));
        QcaOtr::SessionResult state;
        messages = deliverToQca(&qca, messages, &state);
        QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::AskForSecret);
        QVERIFY(messages.isEmpty());

        QVERIFY(libotr.abortSmp(&messages));
        messages = deliverToQca(&qca, messages, &state);
        QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::Abort);
        QCOMPARE(state.smpProgress, quint16(0));
        QVERIFY(messages.isEmpty());
    }
}

QTEST_GUILESS_MAIN(SmpInteropTest)
#include "smpinteroptest.moc"
