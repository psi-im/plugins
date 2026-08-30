#include "qca-otr/session.h"
#include "qca-otr/transport.h"

#include <QTest>
#include <QtCrypto>

namespace {

constexpr quint32 AliceInstance = 0x11111111;
constexpr quint32 BobInstance = 0x22222222;
constexpr quint32 BobSecondInstance = 0x33333333;
constexpr quint32 OtherLocalInstance = 0x44444444;

QcaOtr::DsaPrivateKey toyKey(int x)
{
    QcaOtr::DsaPrivateKey key;
    key.domain.p = QCA::BigInteger(23);
    key.domain.q = QCA::BigInteger(11);
    key.domain.g = QCA::BigInteger(2);
    key.x = QCA::BigInteger(x);
    return key;
}

QVector<QByteArray> deliver(QcaOtr::OtrSession *target,
                            const QVector<QByteArray> &messages,
                            int maxMessageSize,
                            QcaOtr::SessionResult *lastResult = nullptr)
{
    QVector<QByteArray> outgoing;
    QcaOtr::SessionResult last;
    for (const QByteArray &message : messages) {
        last = target->processIncoming(message, maxMessageSize);
        outgoing += last.outgoingMessages;
    }
    if (lastResult)
        *lastResult = last;
    return outgoing;
}

bool completeHandshake(QcaOtr::OtrSession *initiator,
                       quint32 responderInstance,
                       QcaOtr::OtrSession *responder,
                       int maxMessageSize)
{
    bool ok = false;
    QVector<QByteArray> messages = initiator->start(responderInstance, maxMessageSize, &ok);
    if (!ok || messages.isEmpty())
        return false;

    QcaOtr::SessionResult state;
    messages = deliver(responder, messages, maxMessageSize, &state);
    if (state.status != QcaOtr::SessionStatus::Handled || messages.isEmpty())
        return false;

    messages = deliver(initiator, messages, maxMessageSize, &state);
    if (state.status != QcaOtr::SessionStatus::Handled || messages.isEmpty())
        return false;

    messages = deliver(responder, messages, maxMessageSize, &state);
    if (state.status != QcaOtr::SessionStatus::Authenticated || messages.isEmpty())
        return false;

    messages = deliver(initiator, messages, maxMessageSize, &state);
    return state.status == QcaOtr::SessionStatus::Authenticated && messages.isEmpty();
}

} // namespace

class SessionTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void fragmentedHandshakeAndData();
    void broadcastCommitAuthenticatesMultipleInstances();
    void queryNegotiationStartsAke();
    void whitespacePolicyNegotiation();
    void alwaysPolicyQueuesUntilEncrypted();
    void routesTlvsThroughChildSession();
    void rejectsOtherLocalInstance();
    void rejectsFragmentRouteMismatch();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void SessionTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
}

void SessionTest::cleanupTestCase()
{
    delete initializer_;
}

void SessionTest::fragmentedHandshakeAndData()
{
    constexpr int Mms = 80;
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QcaOtr::OtrSession bob(toyKey(6), BobInstance);

    QVERIFY(completeHandshake(&alice, BobInstance, &bob, Mms));
    QVERIFY(alice.isEncrypted(BobInstance));
    QVERIFY(bob.isEncrypted(AliceInstance));

    QcaOtr::AkeEstablishedSession aliceEstablished;
    QcaOtr::AkeEstablishedSession bobEstablished;
    QVERIFY(alice.establishedSession(BobInstance, &aliceEstablished));
    QVERIFY(bob.establishedSession(AliceInstance, &bobEstablished));
    QCOMPARE(aliceEstablished.sessionId, bobEstablished.sessionId);

    const QByteArray plaintext = QByteArray("fragmented-data:") + QByteArray(512, 'x');
    QVector<QByteArray> encrypted;
    QVERIFY(alice.sendMessage(BobInstance, plaintext, &encrypted, Mms));
    QVERIFY(encrypted.size() > 1);

    QcaOtr::SessionResult received;
    const QVector<QByteArray> response = deliver(&bob, encrypted, Mms, &received);
    QCOMPARE(received.status, QcaOtr::SessionStatus::Message);
    QCOMPARE(received.peerInstance, AliceInstance);
    QCOMPARE(received.plaintext, plaintext);
    QVERIFY(response.isEmpty());

    const QByteArray reply = QByteArray("reply:") + QByteArray(256, 'y');
    encrypted.clear();
    QVERIFY(bob.sendMessage(AliceInstance, reply, &encrypted, Mms));
    QVERIFY(encrypted.size() > 1);
    deliver(&alice, encrypted, Mms, &received);
    QCOMPARE(received.status, QcaOtr::SessionStatus::Message);
    QCOMPARE(received.plaintext, reply);
}

void SessionTest::broadcastCommitAuthenticatesMultipleInstances()
{
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QcaOtr::OtrSession bobOne(toyKey(6), BobInstance);
    QcaOtr::OtrSession bobTwo(toyKey(7), BobSecondInstance);

    bool ok = false;
    const QVector<QByteArray> commit = alice.start(0, 0, &ok);
    QVERIFY(ok);
    QCOMPARE(commit.size(), 1);

    QcaOtr::SessionResult state;
    QVector<QByteArray> bobOneDh = deliver(&bobOne, commit, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Handled);
    QVERIFY(!bobOneDh.isEmpty());

    QVector<QByteArray> bobTwoDh = deliver(&bobTwo, commit, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Handled);
    QVERIFY(!bobTwoDh.isEmpty());

    // Complete Bob #1 first. The master/broadcast AKE must remain available
    // afterwards so Bob #2 can still answer the exact same original Commit.
    QVector<QByteArray> reveal = deliver(&alice, bobOneDh, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Handled);
    QVector<QByteArray> signature = deliver(&bobOne, reveal, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Authenticated);
    deliver(&alice, signature, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Authenticated);
    QVERIFY(alice.isEncrypted(BobInstance));

    reveal = deliver(&alice, bobTwoDh, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Handled);
    signature = deliver(&bobTwo, reveal, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Authenticated);
    deliver(&alice, signature, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Authenticated);
    QVERIFY(alice.isEncrypted(BobSecondInstance));

    QCOMPARE(alice.peerInstances(), QVector<quint32>({BobInstance, BobSecondInstance}));
    QVERIFY(bobOne.isEncrypted(AliceInstance));
    QVERIFY(bobTwo.isEncrypted(AliceInstance));

    QVector<QByteArray> encrypted;
    QVERIFY(alice.sendMessage(BobInstance, "for first instance", &encrypted));
    deliver(&bobOne, encrypted, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Message);
    QCOMPARE(state.plaintext, QByteArray("for first instance"));

    QVERIFY(alice.sendMessage(BobSecondInstance, "for second instance", &encrypted));
    deliver(&bobTwo, encrypted, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Message);
    QCOMPARE(state.plaintext, QByteArray("for second instance"));
}

void SessionTest::queryNegotiationStartsAke()
{
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QcaOtr::OtrSession bob(toyKey(6), BobInstance);
    alice.setPolicy(QcaOtr::SessionPolicy::Manual);
    bob.setPolicy(QcaOtr::SessionPolicy::Manual);

    const QcaOtr::OutgoingResult query = alice.startNegotiation("alice@example.test");
    QCOMPARE(query.status, QcaOtr::OutgoingStatus::Negotiation);
    QCOMPARE(query.messages.size(), 1);
    QVERIFY(query.messages.front().startsWith("?OTRv3?"));

    QcaOtr::SessionResult state = bob.processIncoming(query.messages.front());
    QCOMPARE(state.status, QcaOtr::SessionStatus::ProtocolMessage);
    QCOMPARE(state.outgoingMessages.size(), 1);

    QVector<QByteArray> messages = state.outgoingMessages;
    messages = deliver(&alice, messages, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Handled);
    messages = deliver(&bob, messages, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Handled);
    messages = deliver(&alice, messages, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Authenticated);
    messages = deliver(&bob, messages, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Authenticated);
    QVERIFY(messages.isEmpty());
    QVERIFY(alice.isEncrypted(BobInstance));
    QVERIFY(bob.isEncrypted(AliceInstance));
}

void SessionTest::whitespacePolicyNegotiation()
{
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QcaOtr::OtrSession manualBob(toyKey(6), BobInstance);
    alice.setPolicy(QcaOtr::SessionPolicy::Opportunistic);
    manualBob.setPolicy(QcaOtr::SessionPolicy::Manual);

    const QcaOtr::OutgoingResult tagged = alice.prepareOutgoing("hello");
    QCOMPARE(tagged.status, QcaOtr::OutgoingStatus::Plaintext);
    QCOMPARE(tagged.messages.size(), 1);
    QVERIFY(tagged.messages.front().size() > 5);

    QcaOtr::SessionResult received = manualBob.processIncoming(tagged.messages.front());
    QCOMPARE(received.status, QcaOtr::SessionStatus::Plaintext);
    QCOMPARE(received.plaintext, QByteArray("hello"));
    QVERIFY(received.outgoingMessages.isEmpty());

    // A normal plaintext reply after our offer is libotr's signal that the
    // peer rejected whitespace discovery, so subsequent messages stay clean.
    received = alice.processIncoming("plain reply");
    QCOMPARE(received.status, QcaOtr::SessionStatus::Plaintext);
    const QcaOtr::OutgoingResult afterRejection = alice.prepareOutgoing("again");
    QCOMPARE(afterRejection.messages, QVector<QByteArray>({QByteArray("again")}));

    QcaOtr::OtrSession opportunisticBob(toyKey(7), BobSecondInstance);
    opportunisticBob.setPolicy(QcaOtr::SessionPolicy::Opportunistic);
    received = opportunisticBob.processIncoming(tagged.messages.front());
    QCOMPARE(received.status, QcaOtr::SessionStatus::Plaintext);
    QCOMPARE(received.plaintext, QByteArray("hello"));
    QCOMPARE(received.outgoingMessages.size(), 1);
}

void SessionTest::alwaysPolicyQueuesUntilEncrypted()
{
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QcaOtr::OtrSession bob(toyKey(6), BobInstance);
    alice.setPolicy(QcaOtr::SessionPolicy::Always);
    bob.setPolicy(QcaOtr::SessionPolicy::Manual);

    const QcaOtr::OutgoingResult initial = alice.prepareOutgoing("held secret", 0, "alice@example.test");
    QCOMPARE(initial.status, QcaOtr::OutgoingStatus::Negotiation);
    QCOMPARE(initial.messages.size(), 1);
    QVERIFY(!initial.messages.front().contains("held secret"));

    QcaOtr::SessionResult state = bob.processIncoming(initial.messages.front());
    QCOMPARE(state.status, QcaOtr::SessionStatus::ProtocolMessage);
    QCOMPARE(state.outgoingMessages.size(), 1);

    QVector<QByteArray> messages = state.outgoingMessages;
    messages = deliver(&alice, messages, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Handled);
    messages = deliver(&bob, messages, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Handled);
    messages = deliver(&alice, messages, 0, &state);
    QCOMPARE(state.status, QcaOtr::SessionStatus::Authenticated);
    QCOMPARE(messages.size(), 2); // Signature first, then the retained Data Message.

    const QcaOtr::SessionResult authenticated = bob.processIncoming(messages.at(0));
    QCOMPARE(authenticated.status, QcaOtr::SessionStatus::Authenticated);
    const QcaOtr::SessionResult secret = bob.processIncoming(messages.at(1));
    QCOMPARE(secret.status, QcaOtr::SessionStatus::Message);
    QCOMPARE(secret.plaintext, QByteArray("held secret"));
}

void SessionTest::routesTlvsThroughChildSession()
{
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QcaOtr::OtrSession bob(toyKey(6), BobInstance);
    QVERIFY(completeHandshake(&alice, BobInstance, &bob, 0));

    QVector<QcaOtr::Tlv> tlvs;
    tlvs.append(QcaOtr::Tlv {0x1234, QByteArray::fromHex("00010200")});

    QVector<QByteArray> encrypted;
    QVERIFY(alice.sendMessage(BobInstance, "payload", tlvs, &encrypted));
    QCOMPARE(encrypted.size(), 1);

    const QcaOtr::SessionResult received = bob.processIncoming(encrypted.front());
    QCOMPARE(received.status, QcaOtr::SessionStatus::Message);
    QCOMPARE(received.peerInstance, AliceInstance);
    QCOMPARE(received.plaintext, QByteArray("payload"));
    QCOMPARE(received.tlvs.size(), 1);
    QCOMPARE(received.tlvs.front().type, quint16(0x1234));
    QCOMPARE(received.tlvs.front().value, QByteArray::fromHex("00010200"));
}

void SessionTest::rejectsOtherLocalInstance()
{
    QcaOtr::OtrSession bob(toyKey(6), BobInstance);
    QcaOtr::OtrSession wrongRecipient(toyKey(7), OtherLocalInstance);

    bool ok = false;
    const QVector<QByteArray> commit = bob.start(AliceInstance, 0, &ok);
    QVERIFY(ok);
    QCOMPARE(commit.size(), 1);

    const QcaOtr::SessionResult result = wrongRecipient.processIncoming(commit.front());
    QCOMPARE(result.status, QcaOtr::SessionStatus::ForOtherInstance);
    QCOMPARE(result.peerInstance, BobInstance);
}

void SessionTest::rejectsFragmentRouteMismatch()
{
    QcaOtr::OtrSession bob(toyKey(6), BobInstance);
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);

    bool ok = false;
    const QVector<QByteArray> commit = bob.start(AliceInstance, 0, &ok);
    QVERIFY(ok);
    QCOMPARE(commit.size(), 1);

    QVector<QByteArray> forgedFragments;
    QVERIFY(QcaOtr::Transport::fragmentMessage(commit.front(),
                                               60,
                                               BobSecondInstance,
                                               AliceInstance,
                                               &forgedFragments));
    QVERIFY(forgedFragments.size() > 1);

    QcaOtr::SessionResult result;
    for (const QByteArray &fragment : forgedFragments)
        result = alice.processIncoming(fragment, 60);
    QCOMPARE(result.status, QcaOtr::SessionStatus::Error);
}

QTEST_GUILESS_MAIN(SessionTest)
#include "sessiontest.moc"
