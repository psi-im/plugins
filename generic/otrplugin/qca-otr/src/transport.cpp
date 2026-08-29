#include "qca-otr/transport.h"

#include <limits>

namespace QcaOtr::Transport {
namespace {

constexpr int RawV3HeaderSize = 11;
constexpr int FragmentOverheadWithTerminator = 37;
constexpr int FragmentPrefixSize = 35;

quint32 readBigEndian32(const char *data)
{
    const auto *p = reinterpret_cast<const unsigned char *>(data);
    return (static_cast<quint32>(p[0]) << 24) | (static_cast<quint32>(p[1]) << 16) |
        (static_cast<quint32>(p[2]) << 8) | static_cast<quint32>(p[3]);
}

bool validRoute(quint32 sender, quint32 receiver)
{
    return sender >= MinimumInstanceTag && (receiver == 0 || receiver >= MinimumInstanceTag);
}

bool parseFixedUnsigned(const QByteArray &message,
                        int offset,
                        int length,
                        int base,
                        quint32 *value)
{
    if (!value || offset < 0 || length <= 0 || offset + length > message.size())
        return false;

    bool ok = false;
    const qulonglong parsed = message.mid(offset, length).toULongLong(&ok, base);
    if (!ok || parsed > std::numeric_limits<quint32>::max())
        return false;

    *value = static_cast<quint32>(parsed);
    return true;
}

QByteArray fixedHex(quint32 value)
{
    return QByteArray::number(value, 16).rightJustified(8, '0');
}

QByteArray fixedDecimal(quint16 value)
{
    return QByteArray::number(value).rightJustified(5, '0');
}

bool sameRoute(const Route &a, const Route &b)
{
    return a.senderInstance == b.senderInstance && a.receiverInstance == b.receiverInstance;
}

} // namespace

QByteArray armor(const QByteArray &raw)
{
    return QByteArrayLiteral("?OTR:") + raw.toBase64() + QByteArrayLiteral(".");
}

bool dearmor(const QByteArray &message, QByteArray *raw)
{
    if (!raw)
        return false;

    const int start = message.indexOf("?OTR:");
    if (start < 0)
        return false;

    const int payloadStart = start + 5;
    const int end = message.indexOf('.', payloadStart);
    if (end < 0 || end == payloadStart)
        return false;

    const QByteArray base64 = message.mid(payloadStart, end - payloadStart);
    const QByteArray decoded = QByteArray::fromBase64(base64, QByteArray::AbortOnBase64DecodingErrors);
    if (decoded.isEmpty())
        return false;

    *raw = decoded;
    return true;
}

bool routeFromRaw(const QByteArray &raw, Route *route)
{
    if (!route || raw.size() < RawV3HeaderSize)
        return false;

    const auto *p = reinterpret_cast<const unsigned char *>(raw.constData());
    const quint16 version = (static_cast<quint16>(p[0]) << 8) | static_cast<quint16>(p[1]);
    if (version != 3)
        return false;

    Route decoded;
    decoded.senderInstance = readBigEndian32(raw.constData() + 3);
    decoded.receiverInstance = readBigEndian32(raw.constData() + 7);
    if (!validRoute(decoded.senderInstance, decoded.receiverInstance))
        return false;

    *route = decoded;
    return true;
}

bool routeFromArmored(const QByteArray &message, Route *route)
{
    QByteArray raw;
    return dearmor(message, &raw) && routeFromRaw(raw, route);
}

FragmentParseStatus parseFragment(const QByteArray &message, Fragment *fragment)
{
    const int tag = message.indexOf("?OTR|");
    if (tag < 0)
        return FragmentParseStatus::NotFragment;
    if (!fragment)
        return FragmentParseStatus::Malformed;

    // libotr 4.1.1 emits:
    // ?OTR|ssssssss|rrrrrrrr,kkkkk,nnnnn,payload,
    // where the instance tags are eight hex digits and fragment numbers are
    // five decimal digits. Keep the parser strict so malformed transport input
    // cannot silently mutate an in-progress reassembly.
    const int base = tag;
    if (message.size() < base + FragmentPrefixSize + 2 || message.mid(base, 5) != "?OTR|" ||
        message.at(base + 13) != '|' || message.at(base + 22) != ',' ||
        message.at(base + 28) != ',' || message.at(base + 34) != ',') {
        return FragmentParseStatus::Malformed;
    }

    quint32 sender = 0;
    quint32 receiver = 0;
    quint32 index = 0;
    quint32 count = 0;
    if (!parseFixedUnsigned(message, base + 5, 8, 16, &sender) ||
        !parseFixedUnsigned(message, base + 14, 8, 16, &receiver) ||
        !parseFixedUnsigned(message, base + 23, 5, 10, &index) ||
        !parseFixedUnsigned(message, base + 29, 5, 10, &count) ||
        !validRoute(sender, receiver) || index == 0 || count == 0 || index > count ||
        index > std::numeric_limits<quint16>::max() || count > std::numeric_limits<quint16>::max()) {
        return FragmentParseStatus::Malformed;
    }

    const int payloadStart = base + FragmentPrefixSize;
    const int payloadEnd = message.indexOf(',', payloadStart);
    if (payloadEnd <= payloadStart)
        return FragmentParseStatus::Malformed;

    Fragment decoded;
    decoded.route.senderInstance = sender;
    decoded.route.receiverInstance = receiver;
    decoded.index = static_cast<quint16>(index);
    decoded.count = static_cast<quint16>(count);
    decoded.payload = message.mid(payloadStart, payloadEnd - payloadStart);
    *fragment = decoded;
    return FragmentParseStatus::Fragment;
}

bool fragmentMessage(const QByteArray &message,
                     int maxMessageSize,
                     quint32 senderInstance,
                     quint32 receiverInstance,
                     QVector<QByteArray> *fragments)
{
    if (!fragments || message.isEmpty() || !validRoute(senderInstance, receiverInstance))
        return false;

    fragments->clear();
    if (maxMessageSize <= 0 || message.size() <= maxMessageSize) {
        fragments->append(message);
        return true;
    }

    // libotr uses 37 here: 36 visible framing bytes plus the terminating NUL
    // in its C buffer. Mirroring that calculation gives byte-for-byte matching
    // fragment counts and leaves one byte of conservative transport slack.
    if (maxMessageSize <= FragmentOverheadWithTerminator)
        return false;
    const int payloadCapacity = maxMessageSize - FragmentOverheadWithTerminator;
    const qint64 count64 = (static_cast<qint64>(message.size()) + payloadCapacity - 1) / payloadCapacity;
    if (count64 <= 0 || count64 > std::numeric_limits<quint16>::max())
        return false;

    const quint16 count = static_cast<quint16>(count64);
    fragments->reserve(count);
    int offset = 0;
    for (quint32 i = 1; i <= count; ++i) {
        const QByteArray payload = message.mid(offset, payloadCapacity);
        if (payload.isEmpty()) {
            fragments->clear();
            return false;
        }

        QByteArray encoded;
        encoded.reserve(FragmentOverheadWithTerminator - 1 + payload.size());
        encoded += QByteArrayLiteral("?OTR|");
        encoded += fixedHex(senderInstance);
        encoded += '|';
        encoded += fixedHex(receiverInstance);
        encoded += ',';
        encoded += fixedDecimal(static_cast<quint16>(i));
        encoded += ',';
        encoded += fixedDecimal(count);
        encoded += ',';
        encoded += payload;
        encoded += ',';
        fragments->append(encoded);
        offset += payload.size();
    }

    return offset == message.size();
}

FragmentAccumulator::FragmentAccumulator(qsizetype maxBufferedBytes) : maxBufferedBytes_(maxBufferedBytes)
{
}

void FragmentAccumulator::reset()
{
    route_ = {};
    count_ = 0;
    index_ = 0;
    data_.clear();
}

FragmentResult FragmentAccumulator::accumulate(const QByteArray &message, QByteArray *completeMessage)
{
    if (completeMessage)
        completeMessage->clear();

    Fragment fragment;
    const FragmentParseStatus parsed = parseFragment(message, &fragment);
    if (parsed == FragmentParseStatus::NotFragment) {
        reset();
        return FragmentResult::Unfragmented;
    }
    if (parsed == FragmentParseStatus::Malformed) {
        reset();
        return FragmentResult::Malformed;
    }

    if (fragment.index == 1) {
        reset();
        route_ = fragment.route;
        count_ = fragment.count;
        index_ = 1;
        data_ = fragment.payload;
    } else {
        if (count_ == 0 || fragment.count != count_ || fragment.index != static_cast<quint32>(index_) + 1 ||
            !sameRoute(fragment.route, route_)) {
            reset();
            return FragmentResult::Malformed;
        }
        data_ += fragment.payload;
        index_ = fragment.index;
    }

    if (maxBufferedBytes_ <= 0 || data_.size() > maxBufferedBytes_) {
        reset();
        return FragmentResult::Malformed;
    }

    if (index_ != count_)
        return FragmentResult::Incomplete;

    if (completeMessage)
        *completeMessage = data_;
    reset();
    return FragmentResult::Complete;
}

} // namespace QcaOtr::Transport
