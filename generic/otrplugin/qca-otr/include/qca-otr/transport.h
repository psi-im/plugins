#pragma once

#include <QByteArray>
#include <QVector>
#include <QtGlobal>

namespace QcaOtr::Transport {

constexpr quint32 MinimumInstanceTag = 0x00000100;

struct Route
{
    quint32 senderInstance = 0;
    quint32 receiverInstance = 0;
};

struct Fragment
{
    Route route;
    quint16 index = 0;
    quint16 count = 0;
    QByteArray payload;
};

enum class FragmentParseStatus {
    NotFragment,
    Fragment,
    Malformed
};

enum class FragmentResult {
    Unfragmented,
    Incomplete,
    Complete,
    Malformed
};

QByteArray armor(const QByteArray &raw);
bool dearmor(const QByteArray &message, QByteArray *raw);

// Extract OTRv3 instance tags from a raw binary protocol message or its
// ?OTR:base64. transport representation. The receiver may be zero for an
// initial D-H Commit addressed to an as-yet unknown peer instance.
bool routeFromRaw(const QByteArray &raw, Route *route);
bool routeFromArmored(const QByteArray &message, Route *route);

FragmentParseStatus parseFragment(const QByteArray &message, Fragment *fragment);

// Split an already-armored OTR message using the exact libotr 4.1.1 v3
// fragment format. If the message fits, the returned vector contains the
// original message as its only element.
bool fragmentMessage(const QByteArray &message,
                     int maxMessageSize,
                     quint32 senderInstance,
                     quint32 receiverInstance,
                     QVector<QByteArray> *fragments);

class FragmentAccumulator
{
public:
    explicit FragmentAccumulator(qsizetype maxBufferedBytes = 16 * 1024 * 1024);

    FragmentResult accumulate(const QByteArray &message, QByteArray *completeMessage = nullptr);
    void reset();

private:
    qsizetype maxBufferedBytes_ = 0;
    Route route_;
    quint16 count_ = 0;
    quint16 index_ = 0;
    QByteArray data_;
};

} // namespace QcaOtr::Transport
