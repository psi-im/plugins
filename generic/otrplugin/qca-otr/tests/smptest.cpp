#include "qca-otr/smp.h"

#include <QTest>
#include <QtCrypto>

namespace {

QCA::SecureArray secret(const char *text)
{
    return QCA::SecureArray(QByteArray(text));
}

} // namespace

class SmpTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void succeedsWithMatchingSecrets();
    void failsWithDifferentSecrets();
    void tracksQuestionAndExpectedMessages();
    void rejectsMalformedAndUnexpectedMessages();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void SmpTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
}

void SmpTest::cleanupTestCase()
{
    delete initializer_;
}

void SmpTest::succeedsWithMatchingSecrets()
{
    QcaOtr::SmpSession alice;
    QcaOtr::SmpSession bob;
    const QCA::SecureArray shared = secret("0123456789abcdef0123456789abcdef");

    const QcaOtr::SmpStepResult message1 = alice.initiate(shared);
    QCOMPARE(message1.status, QcaOtr::SmpStepStatus::Ok);
    QCOMPARE(alice.expected(), QcaOtr::SmpExpected::Message2);
    QVERIFY(!message1.outgoing.isEmpty());

    const QcaOtr::SmpStepResult received1 = bob.receiveMessage1(message1.outgoing, false);
    QCOMPARE(received1.status, QcaOtr::SmpStepStatus::Ok);
    QVERIFY(bob.awaitingSecret());
    QVERIFY(!bob.receivedQuestion());

    const QcaOtr::SmpStepResult message2 = bob.respond(shared);
    QCOMPARE(message2.status, QcaOtr::SmpStepStatus::Ok);
    QCOMPARE(bob.expected(), QcaOtr::SmpExpected::Message3);

    const QcaOtr::SmpStepResult message3 = alice.receiveMessage2(message2.outgoing);
    QCOMPARE(message3.status, QcaOtr::SmpStepStatus::Ok);
    QCOMPARE(alice.expected(), QcaOtr::SmpExpected::Message4);

    const QcaOtr::SmpStepResult message4 = bob.receiveMessage3(message3.outgoing);
    QCOMPARE(message4.status, QcaOtr::SmpStepStatus::Ok);
    QCOMPARE(message4.progress, QcaOtr::SmpProgress::Succeeded);
    QCOMPARE(bob.progress(), QcaOtr::SmpProgress::Succeeded);
    QCOMPARE(bob.expected(), QcaOtr::SmpExpected::Message1);

    const QcaOtr::SmpStepResult final = alice.receiveMessage4(message4.outgoing);
    QCOMPARE(final.status, QcaOtr::SmpStepStatus::Ok);
    QCOMPARE(final.progress, QcaOtr::SmpProgress::Succeeded);
    QCOMPARE(alice.progress(), QcaOtr::SmpProgress::Succeeded);
    QCOMPARE(alice.expected(), QcaOtr::SmpExpected::Message1);
}

void SmpTest::failsWithDifferentSecrets()
{
    QcaOtr::SmpSession alice;
    QcaOtr::SmpSession bob;

    const QcaOtr::SmpStepResult message1 = alice.initiate(secret("alice secret"));
    QCOMPARE(message1.status, QcaOtr::SmpStepStatus::Ok);
    QCOMPARE(bob.receiveMessage1(message1.outgoing, false).status, QcaOtr::SmpStepStatus::Ok);

    const QcaOtr::SmpStepResult message2 = bob.respond(secret("bob secret"));
    QCOMPARE(message2.status, QcaOtr::SmpStepStatus::Ok);
    const QcaOtr::SmpStepResult message3 = alice.receiveMessage2(message2.outgoing);
    QCOMPARE(message3.status, QcaOtr::SmpStepStatus::Ok);

    const QcaOtr::SmpStepResult message4 = bob.receiveMessage3(message3.outgoing);
    QCOMPARE(message4.status, QcaOtr::SmpStepStatus::Ok);
    QCOMPARE(message4.progress, QcaOtr::SmpProgress::Failed);

    const QcaOtr::SmpStepResult final = alice.receiveMessage4(message4.outgoing);
    QCOMPARE(final.status, QcaOtr::SmpStepStatus::Ok);
    QCOMPARE(final.progress, QcaOtr::SmpProgress::Failed);
}

void SmpTest::tracksQuestionAndExpectedMessages()
{
    QcaOtr::SmpSession alice;
    QcaOtr::SmpSession bob;
    const QCA::SecureArray shared = secret("shared");

    const QcaOtr::SmpStepResult message1 = alice.initiate(shared);
    QCOMPARE(message1.status, QcaOtr::SmpStepStatus::Ok);
    QCOMPARE(bob.receiveMessage1(message1.outgoing, true).status, QcaOtr::SmpStepStatus::Ok);
    QVERIFY(bob.awaitingSecret());
    QVERIFY(bob.receivedQuestion());

    QCOMPARE(bob.receiveMessage2(QByteArray()).status, QcaOtr::SmpStepStatus::Unexpected);
    QCOMPARE(bob.expected(), QcaOtr::SmpExpected::Message1);

    const QcaOtr::SmpStepResult message2 = bob.respond(shared);
    QCOMPARE(message2.status, QcaOtr::SmpStepStatus::Ok);
    QCOMPARE(bob.expected(), QcaOtr::SmpExpected::Message3);
}

void SmpTest::rejectsMalformedAndUnexpectedMessages()
{
    QcaOtr::SmpSession alice;
    QcaOtr::SmpSession bob;
    const QCA::SecureArray shared = secret("shared");

    QCOMPARE(alice.receiveMessage4(QByteArray("not a message")).status,
             QcaOtr::SmpStepStatus::Unexpected);

    const QcaOtr::SmpStepResult message1 = alice.initiate(shared);
    QCOMPARE(message1.status, QcaOtr::SmpStepStatus::Ok);
    QByteArray malformed = message1.outgoing;
    malformed.chop(1);

    const QcaOtr::SmpStepResult rejected = bob.receiveMessage1(malformed, false);
    QCOMPARE(rejected.status, QcaOtr::SmpStepStatus::Invalid);
    QCOMPARE(rejected.progress, QcaOtr::SmpProgress::Cheated);
    QCOMPARE(bob.progress(), QcaOtr::SmpProgress::Cheated);
    QCOMPARE(bob.expected(), QcaOtr::SmpExpected::Message1);
    QVERIFY(!bob.awaitingSecret());

    bob.abort();
    QCOMPARE(bob.progress(), QcaOtr::SmpProgress::Ok);
}

QTEST_GUILESS_MAIN(SmpTest)
#include "smptest.moc"
