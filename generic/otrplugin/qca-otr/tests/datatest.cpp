#include "qca-otr/data.h"

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

bool completeAke(QcaOtr::AkeSession *alice, QcaOtr::AkeSession *bob)
{
    if (!alice || !bob)
        return false;

    bool ok = false;
    const QByteArray commit = alice->start(&ok);
    if (!ok)
        return false;
    const QcaOtr::AkeHandleResult dhKey = bob->processIncoming(commit);
    if (dhKey.status != QcaOtr::AkeHandleStatus::Handled)
        return false;
    const QcaOtr::AkeHandleResult reveal = alice->processIncoming(dhKey.outgoingMessage);
    if (reveal.status != QcaOtr::AkeHandleStatus::Handled)
        return false;
    const QcaOtr::AkeHandleResult signature = bob->processIncoming(reveal.outgoingMessage);
    if (signature.status != QcaOtr::AkeHandleStatus::Authenticated)
        return false;
    const QcaOtr::AkeHandleResult done = alice->processIncoming(signature.outgoingMessage);
    return done.status == QcaOtr::AkeHandleStatus::Authenticated && alice->isAuthenticated() &&
        bob->isAuthenticated();
}

} // namespace

class DataTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void sessionKeysAreDirectional();
    void wireRoundTrip();
    void exchangesMessagesAndRotatesKeys();
    void exchangesTlvs();
    void rejectsReplayAndTampering();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void DataTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
}

void DataTest::cleanupTestCase()
{
    delete initializer_;
}

void DataTest::sessionKeysAreDirectional()
{
    QcaOtr::DhKeyPair alice;
    QcaOtr::DhKeyPair bob;
    QVERIFY(QcaOtr::generateDhKeyPair(&alice));
    QVERIFY(QcaOtr::generateDhKeyPair(&bob));

    QcaOtr::DataSessionKeys aliceKeys;
    QcaOtr::DataSessionKeys bobKeys;
    QVERIFY(QcaOtr::deriveDataSessionKeys(alice, bob.publicValue, &aliceKeys));
    QVERIFY(QcaOtr::deriveDataSessionKeys(bob, alice.publicValue, &bobKeys));

    QCOMPARE(aliceKeys.sendEncryptionKey.toByteArray(), bobKeys.receiveEncryptionKey.toByteArray());
    QCOMPARE(aliceKeys.receiveEncryptionKey.toByteArray(), bobKeys.sendEncryptionKey.toByteArray());
    QCOMPARE(aliceKeys.sendMacKey.toByteArray(), bobKeys.receiveMacKey.toByteArray());
    QCOMPARE(aliceKeys.receiveMacKey.toByteArray(), bobKeys.sendMacKey.toByteArray());
    QCOMPARE(aliceKeys.extraKey.toByteArray(), bobKeys.extraKey.toByteArray());
}

void DataTest::wireRoundTrip()
{
    QcaOtr::DhKeyPair next;
    QVERIFY(QcaOtr::generateDhKeyPair(&next));

    QcaOtr::DataMessage message;
    message.senderInstance = AliceInstance;
    message.receiverInstance = BobInstance;
    message.flags = 0x01;
    message.senderKeyId = 7;
    message.recipientKeyId = 9;
    message.nextDhPublic = next.publicValue;
    message.counter = QByteArray::fromHex("0102030405060708");
    message.encryptedData = QByteArray::fromHex("deadbeef00");
    message.mac = QByteArray(20, '\x11');
    message.revealedMacKeys = QByteArray(40, '\x22');

    bool ok = false;
    const QByteArray encoded = QcaOtr::Wire::encodeDataMessage(message, &ok);
    QVERIFY(ok);
    QVERIFY(!encoded.isEmpty());
    QCOMPARE(static_cast<quint8>(encoded.at(2)), quint8(0x03));

    QcaOtr::DataMessage decoded;
    QVERIFY(QcaOtr::Wire::decodeDataMessage(encoded, &decoded));
    QCOMPARE(decoded.senderInstance, message.senderInstance);
    QCOMPARE(decoded.receiverInstance, message.receiverInstance);
    QCOMPARE(decoded.flags, message.flags);
    QCOMPARE(decoded.senderKeyId, message.senderKeyId);
    QCOMPARE(decoded.recipientKeyId, message.recipientKeyId);
    QCOMPARE(decoded.nextDhPublic, message.nextDhPublic);
    QCOMPARE(decoded.counter, message.counter);
    QCOMPARE(decoded.encryptedData, message.encryptedData);
    QCOMPARE(decoded.mac, message.mac);
    QCOMPARE(decoded.revealedMacKeys, message.revealedMacKeys);

    QByteArray trailing = encoded;
    trailing.append('\0');
    QVERIFY(!QcaOtr::Wire::decodeDataMessage(trailing, &decoded));
}

void DataTest::exchangesMessagesAndRotatesKeys()
{
    QcaOtr::AkeSession aliceAke(toyKey(3), AliceInstance, BobInstance);
    QcaOtr::AkeSession bobAke(toyKey(6), BobInstance, AliceInstance);
    QVERIFY(completeAke(&aliceAke, &bobAke));

    QcaOtr::DataSession alice(aliceAke.established(), AliceInstance, BobInstance);
    QcaOtr::DataSession bob(bobAke.established(), BobInstance, AliceInstance);
    QVERIFY(alice.isReady());
    QVERIFY(bob.isReady());
    QCOMPARE(alice.localKeyId(), quint32(2));
    QCOMPARE(alice.peerKeyId(), quint32(1));
    QCOMPARE(bob.localKeyId(), quint32(2));
    QCOMPARE(bob.peerKeyId(), quint32(1));

    QByteArray aliceFirst;
    QVERIFY(alice.sendMessage("hello bob", &aliceFirst));
    QcaOtr::DataMessage firstWire;
    QVERIFY(QcaOtr::Wire::decodeDataMessage(aliceFirst, &firstWire));
    QCOMPARE(firstWire.senderKeyId, quint32(1));
    QCOMPARE(firstWire.recipientKeyId, quint32(1));
    QCOMPARE(firstWire.revealedMacKeys.size(), 0);

    const QcaOtr::DataReceiveResult bobFirst = bob.processIncoming(aliceFirst);
    QCOMPARE(bobFirst.status, QcaOtr::DataReceiveStatus::Message);
    QCOMPARE(bobFirst.plaintext, QByteArray("hello bob"));
    QVERIFY(bobFirst.tlvs.isEmpty());
    QCOMPARE(bob.peerKeyId(), quint32(2));
    QCOMPARE(bob.localKeyId(), quint32(2));

    QByteArray bobFirstReply;
    QVERIFY(bob.sendMessage("hello alice", &bobFirstReply));
    QcaOtr::DataMessage replyWire;
    QVERIFY(QcaOtr::Wire::decodeDataMessage(bobFirstReply, &replyWire));
    QCOMPARE(replyWire.senderKeyId, quint32(1));
    QCOMPARE(replyWire.recipientKeyId, quint32(2));

    const QcaOtr::DataReceiveResult aliceReply = alice.processIncoming(bobFirstReply);
    QCOMPARE(aliceReply.status, QcaOtr::DataReceiveStatus::Message);
    QCOMPARE(aliceReply.plaintext, QByteArray("hello alice"));
    QVERIFY(aliceReply.tlvs.isEmpty());
    QCOMPARE(alice.localKeyId(), quint32(3));
    QCOMPARE(alice.peerKeyId(), quint32(2));

    QByteArray aliceSecond;
    QVERIFY(alice.sendMessage("ratcheted", &aliceSecond));
    QcaOtr::DataMessage secondWire;
    QVERIFY(QcaOtr::Wire::decodeDataMessage(aliceSecond, &secondWire));
    QCOMPARE(secondWire.senderKeyId, quint32(2));
    QCOMPARE(secondWire.recipientKeyId, quint32(2));
    QVERIFY(secondWire.revealedMacKeys.size() >= 20);
    QCOMPARE(secondWire.revealedMacKeys.size() % 20, 0);

    const QcaOtr::DataReceiveResult bobSecond = bob.processIncoming(aliceSecond);
    QCOMPARE(bobSecond.status, QcaOtr::DataReceiveStatus::Message);
    QCOMPARE(bobSecond.plaintext, QByteArray("ratcheted"));
    QVERIFY(bobSecond.tlvs.isEmpty());
    QCOMPARE(bob.localKeyId(), quint32(3));
    QCOMPARE(bob.peerKeyId(), quint32(3));
}

void DataTest::exchangesTlvs()
{
    QcaOtr::AkeSession aliceAke(toyKey(3), AliceInstance, BobInstance);
    QcaOtr::AkeSession bobAke(toyKey(6), BobInstance, AliceInstance);
    QVERIFY(completeAke(&aliceAke, &bobAke));

    QcaOtr::DataSession alice(aliceAke.established(), AliceInstance, BobInstance);
    QcaOtr::DataSession bob(bobAke.established(), BobInstance, AliceInstance);
    QVERIFY(alice.isReady());
    QVERIFY(bob.isReady());

    QVector<QcaOtr::Tlv> tlvs;
    tlvs.append({static_cast<quint16>(QcaOtr::TlvType::Padding), QByteArray::fromHex("00010200")});
    tlvs.append({0x1234, QByteArray::fromHex("61006200ff")});

    QByteArray encrypted;
    QVERIFY(alice.sendMessage("control payload", tlvs, &encrypted));

    const QcaOtr::DataReceiveResult received = bob.processIncoming(encrypted);
    QCOMPARE(received.status, QcaOtr::DataReceiveStatus::Message);
    QCOMPARE(received.plaintext, QByteArray("control payload"));
    QCOMPARE(received.tlvs.size(), 2);
    QCOMPARE(received.tlvs.at(0).type, quint16(0));
    QCOMPARE(received.tlvs.at(0).value, QByteArray::fromHex("00010200"));
    QCOMPARE(received.tlvs.at(1).type, quint16(0x1234));
    QCOMPARE(received.tlvs.at(1).value, QByteArray::fromHex("61006200ff"));
    QCOMPARE(received.extraKey.size(), 32);
}

void DataTest::rejectsReplayAndTampering()
{
    QcaOtr::AkeSession aliceAke(toyKey(3), AliceInstance, BobInstance);
    QcaOtr::AkeSession bobAke(toyKey(6), BobInstance, AliceInstance);
    QVERIFY(completeAke(&aliceAke, &bobAke));

    QcaOtr::DataSession alice(aliceAke.established(), AliceInstance, BobInstance);
    QcaOtr::DataSession bob(bobAke.established(), BobInstance, AliceInstance);
    QVERIFY(alice.isReady());
    QVERIFY(bob.isReady());

    QByteArray message;
    QVERIFY(alice.sendMessage("once", &message));

    QByteArray damaged = message;
    damaged[damaged.size() - 5] = static_cast<char>(damaged.at(damaged.size() - 5) ^ 0x01);
    const QcaOtr::DataReceiveResult tampered = bob.processIncoming(damaged);
    QCOMPARE(tampered.status, QcaOtr::DataReceiveStatus::Error);
    QCOMPARE(bob.peerKeyId(), quint32(1));

    const QcaOtr::DataReceiveResult first = bob.processIncoming(message);
    QCOMPARE(first.status, QcaOtr::DataReceiveStatus::Message);
    QCOMPARE(first.plaintext, QByteArray("once"));
    QCOMPARE(bob.peerKeyId(), quint32(2));

    const QcaOtr::DataReceiveResult replay = bob.processIncoming(message);
    QCOMPARE(replay.status, QcaOtr::DataReceiveStatus::Error);
}

QTEST_GUILESS_MAIN(DataTest)
#include "datatest.moc"
