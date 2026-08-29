#include "qca-otr/akesession.h"

#include <QTest>
#include <QtCrypto>

namespace {

QcaOtr::DsaPrivateKey toyKey(int x)
{
    QcaOtr::DsaPrivateKey key;
    key.domain.p = QCA::BigInteger(23);
    key.domain.q = QCA::BigInteger(11);
    key.domain.g = QCA::BigInteger(2);
    key.x = QCA::BigInteger(x);
    return key;
}

quint8 messageType(const QByteArray &message)
{
    return message.size() >= 3 ? static_cast<quint8>(message.at(2)) : 0;
}

} // namespace

class AkeSessionTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void completeHandshake();
    void simultaneousInitiation();
    void rejectsTamperingWithoutLosingState();
    void ignoresOtherInstances();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void AkeSessionTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
}

void AkeSessionTest::cleanupTestCase()
{
    delete initializer_;
}

void AkeSessionTest::completeHandshake()
{
    constexpr quint32 AliceInstance = 0x11111111;
    constexpr quint32 BobInstance = 0x22222222;

    const QcaOtr::DsaPrivateKey aliceKey = toyKey(3);
    const QcaOtr::DsaPrivateKey bobKey = toyKey(6);
    QcaOtr::AkeSession alice(aliceKey, AliceInstance, BobInstance);
    QcaOtr::AkeSession bob(bobKey, BobInstance, AliceInstance);

    bool ok = false;
    const QByteArray commit = alice.start(&ok);
    QVERIFY(ok);
    QCOMPARE(messageType(commit), quint8(0x02));
    QCOMPARE(alice.state(), QcaOtr::AkeState::AwaitingDhKey);

    const QcaOtr::AkeHandleResult dhKey = bob.processIncoming(commit);
    QCOMPARE(dhKey.status, QcaOtr::AkeHandleStatus::Handled);
    QCOMPARE(messageType(dhKey.outgoingMessage), quint8(0x0a));
    QCOMPARE(bob.state(), QcaOtr::AkeState::AwaitingRevealSignature);

    const QcaOtr::AkeHandleResult reveal = alice.processIncoming(dhKey.outgoingMessage);
    QCOMPARE(reveal.status, QcaOtr::AkeHandleStatus::Handled);
    QCOMPARE(messageType(reveal.outgoingMessage), quint8(0x11));
    QCOMPARE(alice.state(), QcaOtr::AkeState::AwaitingSignature);

    // libotr retransmits the exact previous Reveal Signature when the same
    // D-H Key arrives again.
    const QcaOtr::AkeHandleResult retransmitReveal = alice.processIncoming(dhKey.outgoingMessage);
    QCOMPARE(retransmitReveal.status, QcaOtr::AkeHandleStatus::Handled);
    QCOMPARE(retransmitReveal.outgoingMessage, reveal.outgoingMessage);

    const QcaOtr::AkeHandleResult signature = bob.processIncoming(reveal.outgoingMessage);
    QCOMPARE(signature.status, QcaOtr::AkeHandleStatus::Authenticated);
    QCOMPARE(messageType(signature.outgoingMessage), quint8(0x12));
    QVERIFY(bob.isAuthenticated());

    const QcaOtr::AkeHandleResult done = alice.processIncoming(signature.outgoingMessage);
    QCOMPARE(done.status, QcaOtr::AkeHandleStatus::Authenticated);
    QVERIFY(done.outgoingMessage.isEmpty());
    QVERIFY(alice.isAuthenticated());

    QCOMPARE(alice.established().sessionId, bob.established().sessionId);
    QCOMPARE(alice.established().peerFingerprint,
             QcaOtr::dsaPublicKeyFingerprint(QcaOtr::dsaPublicKey(bobKey)));
    QCOMPARE(bob.established().peerFingerprint,
             QcaOtr::dsaPublicKeyFingerprint(QcaOtr::dsaPublicKey(aliceKey)));
    QCOMPARE(alice.established().peerDhPublic, bob.established().localDh.publicValue);
    QCOMPARE(bob.established().peerDhPublic, alice.established().localDh.publicValue);
    QCOMPARE(alice.established().localKeyId, quint32(1));
    QCOMPARE(alice.established().peerKeyId, quint32(1));
    QCOMPARE(bob.established().localKeyId, quint32(1));
    QCOMPARE(bob.established().peerKeyId, quint32(1));
    QVERIFY(alice.established().initiated);
    QVERIFY(!bob.established().initiated);
}

void AkeSessionTest::simultaneousInitiation()
{
    constexpr quint32 AliceInstance = 0x11111111;
    constexpr quint32 BobInstance = 0x22222222;

    QcaOtr::AkeSession alice(toyKey(3), AliceInstance, BobInstance);
    QcaOtr::AkeSession bob(toyKey(6), BobInstance, AliceInstance);

    bool aliceOk = false;
    bool bobOk = false;
    const QByteArray aliceCommit = alice.start(&aliceOk);
    const QByteArray bobCommit = bob.start(&bobOk);
    QVERIFY(aliceOk);
    QVERIFY(bobOk);

    const QcaOtr::AkeHandleResult aliceCollision = alice.processIncoming(bobCommit);
    const QcaOtr::AkeHandleResult bobCollision = bob.processIncoming(aliceCommit);
    QCOMPARE(aliceCollision.status, QcaOtr::AkeHandleStatus::Handled);
    QCOMPARE(bobCollision.status, QcaOtr::AkeHandleStatus::Handled);

    const bool aliceWon = messageType(aliceCollision.outgoingMessage) == 0x02;
    const bool bobWon = messageType(bobCollision.outgoingMessage) == 0x02;
    QVERIFY(aliceWon != bobWon);

    QcaOtr::AkeSession *winner = aliceWon ? &alice : &bob;
    QcaOtr::AkeSession *loser = aliceWon ? &bob : &alice;
    const QByteArray winnerCommit = aliceWon ? aliceCommit : bobCommit;
    const QByteArray loserDhKey = aliceWon ? bobCollision.outgoingMessage : aliceCollision.outgoingMessage;
    const QByteArray winnerRetransmit = aliceWon ? aliceCollision.outgoingMessage : bobCollision.outgoingMessage;

    QCOMPARE(winnerRetransmit, winnerCommit);
    QCOMPARE(winner->state(), QcaOtr::AkeState::AwaitingDhKey);
    QCOMPARE(loser->state(), QcaOtr::AkeState::AwaitingRevealSignature);

    // A repeated winning commit replaces the stored responder commitment but
    // must return the exact same D-H Key.
    const QcaOtr::AkeHandleResult repeatedCommit = loser->processIncoming(winnerCommit);
    QCOMPARE(repeatedCommit.status, QcaOtr::AkeHandleStatus::Handled);
    QCOMPARE(repeatedCommit.outgoingMessage, loserDhKey);

    const QcaOtr::AkeHandleResult reveal = winner->processIncoming(loserDhKey);
    QCOMPARE(reveal.status, QcaOtr::AkeHandleStatus::Handled);
    QCOMPARE(messageType(reveal.outgoingMessage), quint8(0x11));

    const QcaOtr::AkeHandleResult signature = loser->processIncoming(reveal.outgoingMessage);
    QCOMPARE(signature.status, QcaOtr::AkeHandleStatus::Authenticated);
    QCOMPARE(messageType(signature.outgoingMessage), quint8(0x12));

    const QcaOtr::AkeHandleResult done = winner->processIncoming(signature.outgoingMessage);
    QCOMPARE(done.status, QcaOtr::AkeHandleStatus::Authenticated);
    QVERIFY(winner->isAuthenticated());
    QVERIFY(loser->isAuthenticated());
    QCOMPARE(winner->established().sessionId, loser->established().sessionId);
}

void AkeSessionTest::rejectsTamperingWithoutLosingState()
{
    constexpr quint32 AliceInstance = 0x11111111;
    constexpr quint32 BobInstance = 0x22222222;

    QcaOtr::AkeSession alice(toyKey(3), AliceInstance, BobInstance);
    QcaOtr::AkeSession bob(toyKey(6), BobInstance, AliceInstance);

    bool ok = false;
    const QByteArray commit = alice.start(&ok);
    QVERIFY(ok);
    const QcaOtr::AkeHandleResult dhKey = bob.processIncoming(commit);
    const QcaOtr::AkeHandleResult reveal = alice.processIncoming(dhKey.outgoingMessage);
    QCOMPARE(alice.state(), QcaOtr::AkeState::AwaitingSignature);
    QCOMPARE(bob.state(), QcaOtr::AkeState::AwaitingRevealSignature);

    QByteArray damagedReveal = reveal.outgoingMessage;
    damagedReveal[damagedReveal.size() - 1] = static_cast<char>(damagedReveal.back() ^ 0x01);
    const QcaOtr::AkeHandleResult rejectedReveal = bob.processIncoming(damagedReveal);
    QCOMPARE(rejectedReveal.status, QcaOtr::AkeHandleStatus::Error);
    QCOMPARE(bob.state(), QcaOtr::AkeState::AwaitingRevealSignature);

    const QcaOtr::AkeHandleResult signature = bob.processIncoming(reveal.outgoingMessage);
    QCOMPARE(signature.status, QcaOtr::AkeHandleStatus::Authenticated);

    QByteArray damagedSignature = signature.outgoingMessage;
    damagedSignature[damagedSignature.size() - 1] = static_cast<char>(damagedSignature.back() ^ 0x01);
    const QcaOtr::AkeHandleResult rejectedSignature = alice.processIncoming(damagedSignature);
    QCOMPARE(rejectedSignature.status, QcaOtr::AkeHandleStatus::Error);
    QCOMPARE(alice.state(), QcaOtr::AkeState::AwaitingSignature);

    const QcaOtr::AkeHandleResult done = alice.processIncoming(signature.outgoingMessage);
    QCOMPARE(done.status, QcaOtr::AkeHandleStatus::Authenticated);
    QVERIFY(alice.isAuthenticated());
}

void AkeSessionTest::ignoresOtherInstances()
{
    constexpr quint32 AliceInstance = 0x11111111;
    constexpr quint32 BobInstance = 0x22222222;
    constexpr quint32 CharlieInstance = 0x33333333;

    QcaOtr::AkeSession alice(toyKey(3), AliceInstance, BobInstance);
    QcaOtr::AkeSession charlie(toyKey(6), CharlieInstance, AliceInstance);

    bool ok = false;
    const QByteArray foreignCommit = charlie.start(&ok);
    QVERIFY(ok);

    const QcaOtr::AkeHandleResult result = alice.processIncoming(foreignCommit);
    QCOMPARE(result.status, QcaOtr::AkeHandleStatus::Ignored);
    QCOMPARE(alice.state(), QcaOtr::AkeState::None);
    QCOMPARE(alice.peerInstance(), BobInstance);
}

QTEST_GUILESS_MAIN(AkeSessionTest)
#include "akesessiontest.moc"
