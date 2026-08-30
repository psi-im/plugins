/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#include "qca-otr/transport.h"

#include <QTest>

#include <cstdlib>

extern "C" {
#include <libotr/context.h>
#include <libotr/proto.h>
#include <libotr/userstate.h>
}

namespace {

constexpr quint32 LocalInstance = 0x11111111;
constexpr quint32 PeerInstance = 0x22222222;

class LibotrContext
{
public:
    LibotrContext()
    {
        userState = otrl_userstate_create();
        if (!userState)
            return;

        int added = 0;
        context = otrl_context_find(userState,
                                    "peer",
                                    "account",
                                    "protocol",
                                    PeerInstance,
                                    1,
                                    &added,
                                    nullptr,
                                    nullptr);
        if (!context)
            return;

        context->protocol_version = 3;
        context->our_instance = LocalInstance;
        context->their_instance = PeerInstance;
        context->auth.protocol_version = 3;
    }

    ~LibotrContext()
    {
        if (userState)
            otrl_userstate_free(userState);
    }

    explicit operator bool() const { return context != nullptr; }

    OtrlUserState userState = nullptr;
    ConnContext *context = nullptr;
};

} // namespace

class FragmentInteropTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void qcaFragmentsMatchLibotr();
    void qcaReassemblesLibotr();
    void libotrReassemblesQca();
};

void FragmentInteropTest::initTestCase()
{
    OTRL_INIT;
}

void FragmentInteropTest::qcaFragmentsMatchLibotr()
{
    LibotrContext libotr;
    QVERIFY(libotr);

    const QByteArray message = "?OTR:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/ABCDEFGHIJKLMNOPQRSTUVWXYZ.";
    constexpr int MaxMessageSize = 60;
    constexpr int PayloadCapacity = MaxMessageSize - 37;
    const int fragmentCount = ((message.size() - 1) / PayloadCapacity) + 1;

    char **libotrFragments = nullptr;
    QCOMPARE(otrl_proto_fragment_create(MaxMessageSize,
                                        fragmentCount,
                                        &libotrFragments,
                                        libotr.context,
                                        message.constData()),
             gcry_error_t(0));
    QVERIFY(libotrFragments != nullptr);

    QVector<QByteArray> qcaFragments;
    QVERIFY(QcaOtr::Transport::fragmentMessage(message,
                                               MaxMessageSize,
                                               LocalInstance,
                                               PeerInstance,
                                               &qcaFragments));
    QCOMPARE(qcaFragments.size(), fragmentCount);
    for (int i = 0; i < fragmentCount; ++i)
        QCOMPARE(qcaFragments.at(i), QByteArray(libotrFragments[i]));

    otrl_proto_fragment_free(&libotrFragments, static_cast<unsigned short>(fragmentCount));
}

void FragmentInteropTest::qcaReassemblesLibotr()
{
    LibotrContext libotr;
    QVERIFY(libotr);

    const QByteArray message = "?OTR:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/ABCDEFGHIJKLMNOPQRSTUVWXYZ.";
    constexpr int MaxMessageSize = 60;
    constexpr int PayloadCapacity = MaxMessageSize - 37;
    const int fragmentCount = ((message.size() - 1) / PayloadCapacity) + 1;

    char **fragments = nullptr;
    QCOMPARE(otrl_proto_fragment_create(MaxMessageSize,
                                        fragmentCount,
                                        &fragments,
                                        libotr.context,
                                        message.constData()),
             gcry_error_t(0));

    QcaOtr::Transport::FragmentAccumulator accumulator;
    QByteArray complete;
    for (int i = 0; i < fragmentCount; ++i) {
        const auto expected = i + 1 == fragmentCount ? QcaOtr::Transport::FragmentResult::Complete
                                                     : QcaOtr::Transport::FragmentResult::Incomplete;
        QCOMPARE(accumulator.accumulate(QByteArray(fragments[i]), &complete), expected);
    }
    QCOMPARE(complete, message);
    otrl_proto_fragment_free(&fragments, static_cast<unsigned short>(fragmentCount));
}

void FragmentInteropTest::libotrReassemblesQca()
{
    LibotrContext libotr;
    QVERIFY(libotr);

    const QByteArray message = "?OTR:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/ABCDEFGHIJKLMNOPQRSTUVWXYZ.";
    QVector<QByteArray> fragments;
    QVERIFY(QcaOtr::Transport::fragmentMessage(message,
                                               60,
                                               LocalInstance,
                                               PeerInstance,
                                               &fragments));
    QVERIFY(fragments.size() > 1);

    char *complete = nullptr;
    for (int i = 0; i < fragments.size(); ++i) {
        const OtrlFragmentResult result =
            otrl_proto_fragment_accumulate(&complete, libotr.context, fragments.at(i).constData());
        QCOMPARE(result,
                 i + 1 == fragments.size() ? OTRL_FRAGMENT_COMPLETE : OTRL_FRAGMENT_INCOMPLETE);
    }
    QVERIFY(complete != nullptr);
    QCOMPARE(QByteArray(complete), message);
    std::free(complete);
}

QTEST_GUILESS_MAIN(FragmentInteropTest)
#include "fragmentinteroptest.moc"
