#include "qca-otr/transport.h"

#include <QTest>

#include <cstdlib>

extern "C" {
#include <libotr/context.h>
#include <libotr/proto.h>
}

namespace {

constexpr quint32 LocalInstance = 0x11111111;
constexpr quint32 PeerInstance = 0x22222222;

void initializeContext(ConnContext *context)
{
    std::memset(context, 0, sizeof(*context));
    context->protocol_version = 3;
    context->our_instance = LocalInstance;
    context->their_instance = PeerInstance;
    context->auth.protocol_version = 3;
}

} // namespace

class FragmentInteropTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void qcaFragmentsMatchLibotr();
    void qcaReassemblesLibotr();
    void libotrReassemblesQca();
};

void FragmentInteropTest::qcaFragmentsMatchLibotr()
{
    ConnContext context;
    initializeContext(&context);

    const QByteArray message = "?OTR:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/ABCDEFGHIJKLMNOPQRSTUVWXYZ.";
    constexpr int MaxMessageSize = 60;
    constexpr int PayloadCapacity = MaxMessageSize - 37;
    const int fragmentCount = ((message.size() - 1) / PayloadCapacity) + 1;

    char **libotrFragments = nullptr;
    QCOMPARE(otrl_proto_fragment_create(MaxMessageSize,
                                        fragmentCount,
                                        &libotrFragments,
                                        &context,
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
    ConnContext context;
    initializeContext(&context);

    const QByteArray message = "?OTR:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/ABCDEFGHIJKLMNOPQRSTUVWXYZ.";
    constexpr int MaxMessageSize = 60;
    constexpr int PayloadCapacity = MaxMessageSize - 37;
    const int fragmentCount = ((message.size() - 1) / PayloadCapacity) + 1;

    char **fragments = nullptr;
    QCOMPARE(otrl_proto_fragment_create(MaxMessageSize,
                                        fragmentCount,
                                        &fragments,
                                        &context,
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
    ConnContext context;
    initializeContext(&context);

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
            otrl_proto_fragment_accumulate(&complete, &context, fragments.at(i).constData());
        QCOMPARE(result,
                 i + 1 == fragments.size() ? OTRL_FRAGMENT_COMPLETE : OTRL_FRAGMENT_INCOMPLETE);
    }
    QVERIFY(complete != nullptr);
    QCOMPARE(QByteArray(complete), message);
    std::free(complete);
}

QTEST_GUILESS_MAIN(FragmentInteropTest)
#include "fragmentinteroptest.moc"
