#include "qca-otr/data.h"
#include "qca-otr/negotiation.h"
#include "qca-otr/session.h"
#include "qca-otr/transport.h"

#include <QTest>
#include <QtCrypto>

namespace {

constexpr quint32 AliceInstance = 0x11111111;
constexpr quint32 BobInstance = 0x22222222;
constexpr quint32 BobSecondInstance = 0x33333333;

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
                            QcaOtr::SessionResult *last = nullptr)
{
    QVector<QByteArray> outgoing;
    QcaOtr::SessionResult current;
    for (const QByteArray &message : messages) {
        current = target->processIncoming(message);
        outgoing += current.outgoingMessages;
    }
    if (last)
        *last = current;
    return outgoing;
}

bool completeHandshake(QcaOtr::OtrSession *initiator,
                       quint32 responderInstance,
                       QcaOtr::OtrSession *responder)
{
    bool ok = false;
    QVector<QByteArray> messages = initiator->start(responderInstance, 0, &ok);
    if (!ok || messages.isEmpty())
        return false;

    QcaOtr::SessionResult state;
    messages = deliver(responder, messages, &state);
    if (state.status != QcaOtr::SessionStatus::Handled || messages.isEmpty())
        return false;
    messages = deliver(initiator, messages, &state);
    if (state.status != QcaOtr::SessionStatus::Handled || messages.isEmpty())
        return false;
    messages = deliver(responder, messages, &state);
    if (state.status != QcaOtr::SessionStatus::Authenticated || messages.isEmpty())
        return false;
    messages = deliver(initiator, messages, &state);
    return state.status == QcaOtr::SessionStatus::Authenticated && messages.isEmpty();
}

bool completeQueryRestart(QcaOtr::OtrSession *querySender,
                          QcaOtr::OtrSession *queryReceiver)
{
    const QcaOtr::OutgoingResult query = querySender->startNegotiation("restart");
    if (query.status != QcaOtr::OutgoingStatus::Negotiation || query.messages.size() != 1)
        return false;

    QcaOtr::SessionResult state = queryReceiver->processIncoming(query.messages.front());
    if (state.status != QcaOtr::SessionStatus::ProtocolMessage || state.outgoingMessages.isEmpty())
        return false;

    QVector<QByteArray> messages = state.outgoingMessages;
    messages = deliver(querySender, messages, &state);
    if (state.status != QcaOtr::SessionStatus::Handled || messages.isEmpty())
        return false;
    messages = deliver(queryReceiver, messages, &state);
    if (state.status != QcaOtr::SessionStatus::Handled || messages.isEmpty())
        return false;
    messages = deliver(querySender, messages, &state);
    if (state.status != QcaOtr::SessionStatus::Authenticated || messages.isEmpty())
        return false;
    messages = deliver(queryReceiver, messages, &state);
    return state.status == QcaOtr::SessionStatus::Authenticated && messages.isEmpty();
}

} // namespace

class ControlTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void errorMessagesUseFirstOtrMarker();
    void remoteErrorRestartsOnlyAutomaticPolicies();
    void disconnectUsesFinishedStateAndCanRestart();
    void disconnectIsPerInstance();
    void unreadableAndIgnoreUnreadableSemantics();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void ControlTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
}

void ControlTest::cleanupTestCase()
{
    delete initializer_;
}

void ControlTest::errorMessagesUseFirstOtrMarker()
{
    QCOMPARE(QcaOtr::Negotiation::errorMessage("boom"), QByteArray("?OTR Error: boom"));

    QByteArray text;
    QVERIFY(QcaOtr::Negotiation::parseErrorMessage("prefix ?OTR Error: boom", &text));
    QCOMPARE(text, QByteArray("boom"));

    // libotr classifies the prefix without requiring the canonical separating
    // space, so keep accepting legacy/no-space peers while generating the
    // canonical libotr form ourselves.
    QVERIFY(QcaOtr::Negotiation::parseErrorMessage("prefix ?OTR Error:boom", &text));
    QCOMPARE(text, QByteArray("boom"));

    // libotr classifies from the first ?OTR occurrence only.
    QVERIFY(!QcaOtr::Negotiation::parseErrorMessage("?OTRbroken ?OTR Error: boom", &text));
}

void ControlTest::remoteErrorRestartsOnlyAutomaticPolicies()
{
    QcaOtr::OtrSession manual(toyKey(3), AliceInstance);
    manual.setPolicy(QcaOtr::SessionPolicy::Manual);
    QcaOtr::SessionResult result = manual.processIncoming("?OTR Error:remote failure");
    QCOMPARE(result.status, QcaOtr::SessionStatus::RemoteError);
    QCOMPARE(result.errorText, QByteArray("remote failure"));
    QVERIFY(result.outgoingMessages.isEmpty());

    QcaOtr::OtrSession automatic(toyKey(6), BobInstance);
    automatic.setPolicy(QcaOtr::SessionPolicy::Opportunistic);
    automatic.startNegotiation("bob@example.test");
    result = automatic.processIncoming("text ?OTR Error: remote failure");
    QCOMPARE(result.status, QcaOtr::SessionStatus::RemoteError);
    QCOMPARE(result.errorText, QByteArray("remote failure"));
    QCOMPARE(result.outgoingMessages.size(), 1);
    QVERIFY(result.outgoingMessages.front().startsWith("?OTRv3?\n<b>bob@example.test</b>"));
}

void ControlTest::disconnectUsesFinishedStateAndCanRestart()
{
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QcaOtr::OtrSession bob(toyKey(6), BobInstance);
    QVERIFY(completeHandshake(&alice, BobInstance, &bob));
    QCOMPARE(alice.peerState(BobInstance), QcaOtr::PeerState::Encrypted);
    QCOMPARE(bob.peerState(AliceInstance), QcaOtr::PeerState::Encrypted);

    QVector<QByteArray> disconnectMessages;
    QVERIFY(alice.disconnect(BobInstance, &disconnectMessages));
    QCOMPARE(disconnectMessages.size(), 1);
    QCOMPARE(alice.peerState(BobInstance), QcaOtr::PeerState::Plaintext);
    QVERIFY(!alice.isEncrypted(BobInstance));

    QByteArray raw;
    QVERIFY(QcaOtr::Transport::dearmor(disconnectMessages.front(), &raw));
    QcaOtr::DataMessage wire;
    QVERIFY(QcaOtr::Wire::decodeDataMessage(raw, &wire));
    QCOMPARE(wire.flags, QcaOtr::DataFlagIgnoreUnreadable);

    const QcaOtr::SessionResult closed = bob.processIncoming(disconnectMessages.front());
    QCOMPARE(closed.status, QcaOtr::SessionStatus::Disconnected);
    QCOMPARE(closed.flags, QcaOtr::DataFlagIgnoreUnreadable);
    QCOMPARE(closed.tlvs.size(), 1);
    QCOMPARE(closed.tlvs.front().type, static_cast<quint16>(QcaOtr::TlvType::Disconnected));
    QVERIFY(closed.tlvs.front().value.isEmpty());
    QCOMPARE(bob.peerState(AliceInstance), QcaOtr::PeerState::Finished);
    QVERIFY(!bob.isEncrypted(AliceInstance));

    const QcaOtr::OutgoingResult blocked = bob.prepareOutgoing("must not leak", AliceInstance);
    QCOMPARE(blocked.status, QcaOtr::OutgoingStatus::Finished);
    QVERIFY(blocked.messages.isEmpty());

    QVERIFY(completeQueryRestart(&alice, &bob));
    QCOMPARE(alice.peerState(BobInstance), QcaOtr::PeerState::Encrypted);
    QCOMPARE(bob.peerState(AliceInstance), QcaOtr::PeerState::Encrypted);
}

void ControlTest::disconnectIsPerInstance()
{
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QcaOtr::OtrSession bobOne(toyKey(6), BobInstance);
    QcaOtr::OtrSession bobTwo(toyKey(7), BobSecondInstance);

    QVERIFY(completeHandshake(&alice, BobInstance, &bobOne));
    QVERIFY(completeHandshake(&alice, BobSecondInstance, &bobTwo));
    QVERIFY(alice.isEncrypted(BobInstance));
    QVERIFY(alice.isEncrypted(BobSecondInstance));

    QVector<QByteArray> messages;
    QVERIFY(alice.disconnect(BobInstance, &messages));
    QCOMPARE(alice.peerState(BobInstance), QcaOtr::PeerState::Plaintext);
    QVERIFY(alice.isEncrypted(BobSecondInstance));

    const QcaOtr::SessionResult closed = bobOne.processIncoming(messages.front());
    QCOMPARE(closed.status, QcaOtr::SessionStatus::Disconnected);
    QCOMPARE(bobOne.peerState(AliceInstance), QcaOtr::PeerState::Finished);
    QVERIFY(bobTwo.isEncrypted(AliceInstance));

    const QcaOtr::OutgoingResult stillSecure = alice.prepareOutgoing("second stays secure", BobSecondInstance);
    QCOMPARE(stillSecure.status, QcaOtr::OutgoingStatus::Encrypted);
    QVERIFY(!stillSecure.messages.isEmpty());
    const QcaOtr::SessionResult received = bobTwo.processIncoming(stillSecure.messages.front());
    QCOMPARE(received.status, QcaOtr::SessionStatus::Message);
    QCOMPARE(received.plaintext, QByteArray("second stays secure"));
}

void ControlTest::unreadableAndIgnoreUnreadableSemantics()
{
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QcaOtr::OtrSession bob(toyKey(6), BobInstance);
    QVERIFY(completeHandshake(&alice, BobInstance, &bob));

    QVector<QByteArray> encrypted;
    QVERIFY(alice.sendMessage(BobInstance, "outside session", &encrypted));
    bob.resetPeer(AliceInstance);
    QcaOtr::SessionResult received = bob.processIncoming(encrypted.front());
    QCOMPARE(received.status, QcaOtr::SessionStatus::Unreadable);
    QVERIFY(received.outgoingMessages.isEmpty());

    QVERIFY(alice.sendMessage(BobInstance,
                              "ignore outside session",
                              &encrypted,
                              0,
                              QcaOtr::DataFlagIgnoreUnreadable));
    received = bob.processIncoming(encrypted.front());
    QCOMPARE(received.status, QcaOtr::SessionStatus::Ignored);

    QcaOtr::OtrSession aliceTwo(toyKey(7), AliceInstance);
    QcaOtr::OtrSession bobTwo(toyKey(8), BobInstance);
    QVERIFY(completeHandshake(&aliceTwo, BobInstance, &bobTwo));
    QVERIFY(aliceTwo.sendMessage(BobInstance, "tamper me", &encrypted));

    QByteArray tamperedRaw;
    QVERIFY(QcaOtr::Transport::dearmor(encrypted.front(), &tamperedRaw));
    QcaOtr::DataMessage message;
    QVERIFY(QcaOtr::Wire::decodeDataMessage(tamperedRaw, &message));
    QVERIFY(!message.mac.isEmpty());
    message.mac[0] = static_cast<char>(message.mac.at(0) ^ 0x01);
    bool ok = false;
    tamperedRaw = QcaOtr::Wire::encodeDataMessage(message, &ok);
    QVERIFY(ok);

    received = bobTwo.processIncoming(QcaOtr::Transport::armor(tamperedRaw));
    QCOMPARE(received.status, QcaOtr::SessionStatus::Unreadable);
    QCOMPARE(received.errorText, QByteArray("Unreadable encrypted message."));
    QCOMPARE(received.outgoingMessages.size(), 1);
    QVERIFY(received.outgoingMessages.front().startsWith("?OTR Error: "));
}

QTEST_GUILESS_MAIN(ControlTest)
#include "controltest.moc"
