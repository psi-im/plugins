#include "qca-otr/data.h"
#include "qca-otr/session.h"
#include "qca-otr/transport.h"

#include <QTest>
#include <QtCrypto>

namespace {

constexpr quint32 AliceInstance = 0x11111111;
constexpr quint32 BobInstance = 0x22222222;

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

} // namespace

class SymkeyTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void exchangesSymmetricKeyBothDirections();
    void rejectsInvalidStateAndOversizedUseData();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void SymkeyTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
}

void SymkeyTest::cleanupTestCase()
{
    delete initializer_;
}

void SymkeyTest::exchangesSymmetricKeyBothDirections()
{
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QcaOtr::OtrSession bob(toyKey(6), BobInstance);
    QVERIFY(completeHandshake(&alice, BobInstance, &bob));

    const QByteArray aliceUseData = QByteArray::fromHex("61006200ff");
    QCA::SecureArray aliceKey;
    QVector<QByteArray> messages;
    QVERIFY(alice.sendSymmetricKey(BobInstance,
                                   0x01020304,
                                   aliceUseData,
                                   &aliceKey,
                                   &messages));
    QCOMPARE(aliceKey.size(), 32);
    QCOMPARE(messages.size(), 1);

    QByteArray raw;
    QVERIFY(QcaOtr::Transport::dearmor(messages.front(), &raw));
    QcaOtr::DataMessage wire;
    QVERIFY(QcaOtr::Wire::decodeDataMessage(raw, &wire));
    QCOMPARE(wire.flags, QcaOtr::DataFlagIgnoreUnreadable);

    QcaOtr::SessionResult received = bob.processIncoming(messages.front());
    QCOMPARE(received.status, QcaOtr::SessionStatus::Message);
    QVERIFY(received.plaintext.isEmpty());
    QVERIFY(received.hasSymmetricKey);
    QCOMPARE(received.symmetricKeyUse, quint32(0x01020304));
    QCOMPARE(received.symmetricKeyData, aliceUseData);
    QCOMPARE(received.symmetricKey.size(), 32);
    QCOMPARE(received.symmetricKey.toByteArray(), aliceKey.toByteArray());
    QCOMPARE(received.extraKey.toByteArray(), aliceKey.toByteArray());
    QCOMPARE(received.tlvs.size(), 1);
    QCOMPARE(received.tlvs.front().type, static_cast<quint16>(QcaOtr::TlvType::SymmetricKey));
    QCOMPARE(received.tlvs.front().value.left(4), QByteArray::fromHex("01020304"));
    QCOMPARE(received.tlvs.front().value.mid(4), aliceUseData);

    const QByteArray bobUseData = QByteArray::fromHex("0001020300");
    QCA::SecureArray bobKey;
    QVERIFY(bob.sendSymmetricKey(AliceInstance,
                                 0xa1b2c3d4,
                                 bobUseData,
                                 &bobKey,
                                 &messages));
    QCOMPARE(bobKey.size(), 32);

    received = alice.processIncoming(messages.front());
    QCOMPARE(received.status, QcaOtr::SessionStatus::Message);
    QVERIFY(received.hasSymmetricKey);
    QCOMPARE(received.symmetricKeyUse, quint32(0xa1b2c3d4));
    QCOMPARE(received.symmetricKeyData, bobUseData);
    QCOMPARE(received.symmetricKey.toByteArray(), bobKey.toByteArray());
}

void SymkeyTest::rejectsInvalidStateAndOversizedUseData()
{
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QCA::SecureArray key;
    QVector<QByteArray> messages;

    QVERIFY(!alice.sendSymmetricKey(BobInstance, 1, {}, &key, &messages));
    QVERIFY(key.isEmpty());
    QVERIFY(messages.isEmpty());

    QcaOtr::OtrSession bob(toyKey(6), BobInstance);
    QVERIFY(completeHandshake(&alice, BobInstance, &bob));
    QVERIFY(!alice.sendSymmetricKey(BobInstance,
                                    1,
                                    QByteArray(65532, 'x'),
                                    &key,
                                    &messages));
    QVERIFY(key.isEmpty());
    QVERIFY(messages.isEmpty());
}

QTEST_GUILESS_MAIN(SymkeyTest)
#include "symkeytest.moc"
