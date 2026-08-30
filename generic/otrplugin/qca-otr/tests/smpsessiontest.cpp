/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#include "qca-otr/session.h"

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

QCA::SecureArray secret(const char *text)
{
    return QCA::SecureArray(QByteArray(text));
}

QVector<QByteArray> deliver(QcaOtr::OtrSession *target,
                            const QVector<QByteArray> &messages,
                            QcaOtr::SessionResult *lastResult = nullptr)
{
    QVector<QByteArray> outgoing;
    QcaOtr::SessionResult last;
    for (const QByteArray &message : messages) {
        last = target->processIncoming(message);
        outgoing += last.outgoingMessages;
    }
    if (lastResult)
        *lastResult = last;
    return outgoing;
}

bool completeHandshake(QcaOtr::OtrSession *alice, QcaOtr::OtrSession *bob)
{
    bool ok = false;
    QVector<QByteArray> messages = alice->start(BobInstance, 0, &ok);
    if (!ok || messages.isEmpty())
        return false;

    QcaOtr::SessionResult state;
    messages = deliver(bob, messages, &state);
    if (state.status != QcaOtr::SessionStatus::Handled || messages.isEmpty())
        return false;
    messages = deliver(alice, messages, &state);
    if (state.status != QcaOtr::SessionStatus::Handled || messages.isEmpty())
        return false;
    messages = deliver(bob, messages, &state);
    if (state.status != QcaOtr::SessionStatus::Authenticated || messages.isEmpty())
        return false;
    messages = deliver(alice, messages, &state);
    return state.status == QcaOtr::SessionStatus::Authenticated && messages.isEmpty();
}

} // namespace

class SmpSessionTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void matchingSecretsSucceed();
    void differentSecretsFail();
    void questionAndAbort();
    void malformedAndUnexpectedTlvs();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void SmpSessionTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
}

void SmpSessionTest::cleanupTestCase()
{
    delete initializer_;
}

void SmpSessionTest::matchingSecretsSucceed()
{
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QcaOtr::OtrSession bob(toyKey(6), BobInstance);
    QVERIFY(completeHandshake(&alice, &bob));

    QVector<QByteArray> messages;
    QVERIFY(alice.startSmp(BobInstance, secret("shared answer"), &messages));
    QCOMPARE(messages.size(), 1);

    QcaOtr::SessionResult state = bob.processIncoming(messages.front());
    QCOMPARE(state.status, QcaOtr::SessionStatus::Message);
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::AskForSecret);
    QCOMPARE(state.smpProgress, quint16(25));
    QVERIFY(state.flags & QcaOtr::DataFlagIgnoreUnreadable);
    QCOMPARE(state.tlvs.size(), 1);
    QCOMPARE(state.tlvs.front().type, static_cast<quint16>(QcaOtr::TlvType::Smp1));

    QVERIFY(bob.respondSmp(AliceInstance, secret("shared answer"), &messages));
    QCOMPARE(messages.size(), 1);
    state = alice.processIncoming(messages.front());
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::InProgress);
    QCOMPARE(state.smpProgress, quint16(60));
    QCOMPARE(state.outgoingMessages.size(), 1);

    state = bob.processIncoming(state.outgoingMessages.front());
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::Success);
    QCOMPARE(state.smpProgress, quint16(100));
    QCOMPARE(state.outgoingMessages.size(), 1);

    state = alice.processIncoming(state.outgoingMessages.front());
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::Success);
    QCOMPARE(state.smpProgress, quint16(100));
    QVERIFY(state.outgoingMessages.isEmpty());
}

void SmpSessionTest::differentSecretsFail()
{
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QcaOtr::OtrSession bob(toyKey(6), BobInstance);
    QVERIFY(completeHandshake(&alice, &bob));

    QVector<QByteArray> messages;
    QVERIFY(alice.startSmp(BobInstance, secret("alice answer"), &messages));
    QcaOtr::SessionResult state = bob.processIncoming(messages.front());
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::AskForSecret);

    QVERIFY(bob.respondSmp(AliceInstance, secret("bob answer"), &messages));
    state = alice.processIncoming(messages.front());
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::InProgress);
    QCOMPARE(state.outgoingMessages.size(), 1);

    state = bob.processIncoming(state.outgoingMessages.front());
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::Failure);
    QCOMPARE(state.outgoingMessages.size(), 1);

    state = alice.processIncoming(state.outgoingMessages.front());
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::Failure);
}

void SmpSessionTest::questionAndAbort()
{
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QcaOtr::OtrSession bob(toyKey(6), BobInstance);
    QVERIFY(completeHandshake(&alice, &bob));

    QVector<QByteArray> messages;
    const QByteArray question("What is our shared answer?");
    QVERIFY(alice.startSmp(BobInstance, question, secret("answer"), &messages));

    QcaOtr::SessionResult state = bob.processIncoming(messages.front());
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::AskForAnswer);
    QCOMPARE(state.smpProgress, quint16(25));
    QCOMPARE(state.smpQuestion, question);

    QVERIFY(bob.abortSmp(AliceInstance, &messages));
    QCOMPARE(messages.size(), 1);
    state = alice.processIncoming(messages.front());
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::Abort);
    QCOMPARE(state.smpProgress, quint16(0));
}

void SmpSessionTest::malformedAndUnexpectedTlvs()
{
    QcaOtr::OtrSession alice(toyKey(3), AliceInstance);
    QcaOtr::OtrSession bob(toyKey(6), BobInstance);
    QVERIFY(completeHandshake(&alice, &bob));

    QVector<QcaOtr::Tlv> tlvs;
    tlvs.append(QcaOtr::Tlv {static_cast<quint16>(QcaOtr::TlvType::Smp2), QByteArray()});
    QVector<QByteArray> messages;
    QVERIFY(alice.sendMessage(BobInstance,
                              QByteArray(),
                              tlvs,
                              &messages,
                              0,
                              QcaOtr::DataFlagIgnoreUnreadable));
    QcaOtr::SessionResult state = bob.processIncoming(messages.front());
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::Error);

    tlvs.clear();
    tlvs.append(QcaOtr::Tlv {static_cast<quint16>(QcaOtr::TlvType::Smp1), QByteArray("truncated")});
    QVERIFY(alice.sendMessage(BobInstance,
                              QByteArray(),
                              tlvs,
                              &messages,
                              0,
                              QcaOtr::DataFlagIgnoreUnreadable));
    state = bob.processIncoming(messages.front());
    QCOMPARE(state.smpEvent, QcaOtr::SmpEvent::Cheated);
    QCOMPARE(state.smpProgress, quint16(0));
}

QTEST_GUILESS_MAIN(SmpSessionTest)
#include "smpsessiontest.moc"
