/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#include "qca-otr/transport.h"

#include <QTest>

class TransportTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void armorRoundTrip();
    void extractsV3Route();
    void fragmentsAndReassembles();
    void acceptsZeroCommitReceiver();
    void rejectsOutOfOrderAndMixedRoutes();
    void enforcesBufferLimit();
};

void TransportTest::armorRoundTrip()
{
    const QByteArray raw = QByteArray::fromHex("0003021111111122222222deadbeef");
    const QByteArray encoded = QcaOtr::Transport::armor(raw);
    QCOMPARE(encoded, QByteArray("?OTR:AAMCERERESIiIiLerb7v."));

    QByteArray decoded;
    QVERIFY(QcaOtr::Transport::dearmor(encoded, &decoded));
    QCOMPARE(decoded, raw);
    QVERIFY(!QcaOtr::Transport::dearmor("?OTR:not-base64!.", &decoded));
    QVERIFY(!QcaOtr::Transport::dearmor("?OTR:AAAA", &decoded));
}

void TransportTest::extractsV3Route()
{
    const QByteArray raw = QByteArray::fromHex("000303111111112222222200000000");
    QcaOtr::Transport::Route route;
    QVERIFY(QcaOtr::Transport::routeFromRaw(raw, &route));
    QCOMPARE(route.senderInstance, quint32(0x11111111));
    QCOMPARE(route.receiverInstance, quint32(0x22222222));

    QVERIFY(QcaOtr::Transport::routeFromArmored(QcaOtr::Transport::armor(raw), &route));
    QCOMPARE(route.senderInstance, quint32(0x11111111));
    QCOMPARE(route.receiverInstance, quint32(0x22222222));

    QByteArray v2(raw);
    v2[1] = '\x02';
    QVERIFY(!QcaOtr::Transport::routeFromRaw(v2, &route));
}

void TransportTest::fragmentsAndReassembles()
{
    constexpr quint32 Sender = 0x11111111;
    constexpr quint32 Receiver = 0x22222222;
    const QByteArray message = "?OTR:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/==.";

    QVector<QByteArray> fragments;
    QVERIFY(QcaOtr::Transport::fragmentMessage(message, 60, Sender, Receiver, &fragments));
    QCOMPARE(fragments.size(), 4);
    QCOMPARE(fragments.at(0),
             QByteArray("?OTR|11111111|22222222,00001,00004,?OTR:ABCDEFGHIJKLMNOPQR,"));
    for (const QByteArray &fragment : fragments)
        QVERIFY(fragment.size() <= 60);

    QcaOtr::Transport::Fragment parsed;
    QCOMPARE(QcaOtr::Transport::parseFragment(fragments.at(0), &parsed),
             QcaOtr::Transport::FragmentParseStatus::Fragment);
    QCOMPARE(parsed.route.senderInstance, Sender);
    QCOMPARE(parsed.route.receiverInstance, Receiver);
    QCOMPARE(parsed.index, quint16(1));
    QCOMPARE(parsed.count, quint16(4));
    QCOMPARE(parsed.payload, QByteArray("?OTR:ABCDEFGHIJKLMNOPQR"));

    QcaOtr::Transport::FragmentAccumulator accumulator;
    QByteArray complete;
    for (int i = 0; i < fragments.size(); ++i) {
        const auto expected = i + 1 == fragments.size() ? QcaOtr::Transport::FragmentResult::Complete
                                                       : QcaOtr::Transport::FragmentResult::Incomplete;
        QCOMPARE(accumulator.accumulate(fragments.at(i), &complete), expected);
    }
    QCOMPARE(complete, message);

    QVector<QByteArray> unfragmented;
    QVERIFY(QcaOtr::Transport::fragmentMessage(message, 4096, Sender, Receiver, &unfragmented));
    QCOMPARE(unfragmented, QVector<QByteArray> { message });
}

void TransportTest::acceptsZeroCommitReceiver()
{
    QcaOtr::Transport::Fragment fragment;
    QCOMPARE(QcaOtr::Transport::parseFragment(
                 "?OTR|11111111|00000000,00001,00001,?OTR:AAAA.,", &fragment),
             QcaOtr::Transport::FragmentParseStatus::Fragment);
    QCOMPARE(fragment.route.senderInstance, quint32(0x11111111));
    QCOMPARE(fragment.route.receiverInstance, quint32(0));
}

void TransportTest::rejectsOutOfOrderAndMixedRoutes()
{
    constexpr quint32 Sender = 0x11111111;
    constexpr quint32 Receiver = 0x22222222;
    const QByteArray message(80, 'x');

    QVector<QByteArray> fragments;
    QVERIFY(QcaOtr::Transport::fragmentMessage(message, 60, Sender, Receiver, &fragments));
    QVERIFY(fragments.size() > 2);

    QcaOtr::Transport::FragmentAccumulator accumulator;
    QCOMPARE(accumulator.accumulate(fragments.at(0)), QcaOtr::Transport::FragmentResult::Incomplete);
    QCOMPARE(accumulator.accumulate(fragments.at(2)), QcaOtr::Transport::FragmentResult::Malformed);

    QCOMPARE(accumulator.accumulate(fragments.at(0)), QcaOtr::Transport::FragmentResult::Incomplete);
    QByteArray wrongRoute = fragments.at(1);
    wrongRoute.replace("|22222222,", "|33333333,");
    QCOMPARE(accumulator.accumulate(wrongRoute), QcaOtr::Transport::FragmentResult::Malformed);

    QCOMPARE(accumulator.accumulate(fragments.at(0)), QcaOtr::Transport::FragmentResult::Incomplete);
    QCOMPARE(accumulator.accumulate("ordinary message"), QcaOtr::Transport::FragmentResult::Unfragmented);
    QCOMPARE(accumulator.accumulate(fragments.at(1)), QcaOtr::Transport::FragmentResult::Malformed);
}

void TransportTest::enforcesBufferLimit()
{
    constexpr quint32 Sender = 0x11111111;
    constexpr quint32 Receiver = 0x22222222;
    QVector<QByteArray> fragments;
    QVERIFY(QcaOtr::Transport::fragmentMessage(QByteArray(100, 'x'), 60, Sender, Receiver, &fragments));

    QcaOtr::Transport::FragmentAccumulator accumulator(30);
    QCOMPARE(accumulator.accumulate(fragments.at(0)), QcaOtr::Transport::FragmentResult::Incomplete);
    QCOMPARE(accumulator.accumulate(fragments.at(1)), QcaOtr::Transport::FragmentResult::Malformed);
}

QTEST_GUILESS_MAIN(TransportTest)
#include "transporttest.moc"
