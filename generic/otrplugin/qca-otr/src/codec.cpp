#include "qca-otr/codec.h"

#include <limits>

namespace QcaOtr::Wire {
namespace {

QByteArray unsignedMinimalBytes(const QCA::BigInteger &value, bool *ok)
{
    if (ok)
        *ok = false;
    if (value < QCA::BigInteger(0))
        return {};

    QByteArray bytes = value.toArray().toByteArray();
    while (!bytes.isEmpty() && bytes.front() == '\0')
        bytes.remove(0, 1);

    if (ok)
        *ok = true;
    return bytes;
}

QCA::BigInteger unsignedInteger(const QByteArray &bytes)
{
    if (bytes.isEmpty())
        return QCA::BigInteger(0);

    QByteArray positive;
    positive.reserve(bytes.size() + 1);
    positive.append('\0');
    positive.append(bytes);
    return QCA::BigInteger(QCA::SecureArray(positive));
}

} // namespace

void Writer::writeByte(quint8 value)
{
    data_.append(static_cast<char>(value));
}

void Writer::writeShort(quint16 value)
{
    data_.append(static_cast<char>((value >> 8) & 0xff));
    data_.append(static_cast<char>(value & 0xff));
}

void Writer::writeInt(quint32 value)
{
    data_.append(static_cast<char>((value >> 24) & 0xff));
    data_.append(static_cast<char>((value >> 16) & 0xff));
    data_.append(static_cast<char>((value >> 8) & 0xff));
    data_.append(static_cast<char>(value & 0xff));
}

void Writer::writeBytes(const QByteArray &value)
{
    data_.append(value);
}

bool Writer::writeMpi(const QCA::BigInteger &value)
{
    bool ok = false;
    const QByteArray bytes = unsignedMinimalBytes(value, &ok);
    if (!ok || static_cast<quint64>(bytes.size()) > std::numeric_limits<quint32>::max())
        return false;

    writeInt(static_cast<quint32>(bytes.size()));
    data_.append(bytes);
    return true;
}

void Writer::writeData(const QByteArray &value)
{
    // QByteArray is int-sized on Qt 5/6, therefore its length always fits in
    // the 32-bit OTR DATA length field on the supported platforms.
    writeInt(static_cast<quint32>(value.size()));
    data_.append(value);
}

bool Writer::writeDsaPublicKey(const DsaPublicKey &key)
{
    // OTRv3 defines public-key type 0 as DSA.
    writeShort(0);
    return writeMpi(key.domain.p) && writeMpi(key.domain.q) && writeMpi(key.domain.g) && writeMpi(key.y);
}

bool Reader::readBytes(quint32 length, QByteArray *value)
{
    if (!value || static_cast<quint64>(length) > remaining())
        return false;

    *value = data_.mid(static_cast<int>(offset_), static_cast<int>(length));
    offset_ += length;
    return true;
}

bool Reader::readByte(quint8 *value)
{
    if (!value || remaining() < 1)
        return false;

    *value = static_cast<quint8>(data_.at(static_cast<int>(offset_)));
    ++offset_;
    return true;
}

bool Reader::readShort(quint16 *value)
{
    if (!value || remaining() < 2)
        return false;

    const auto *p = reinterpret_cast<const unsigned char *>(data_.constData() + offset_);
    *value = (static_cast<quint16>(p[0]) << 8) | static_cast<quint16>(p[1]);
    offset_ += 2;
    return true;
}

bool Reader::readInt(quint32 *value)
{
    if (!value || remaining() < 4)
        return false;

    const auto *p = reinterpret_cast<const unsigned char *>(data_.constData() + offset_);
    *value = (static_cast<quint32>(p[0]) << 24) | (static_cast<quint32>(p[1]) << 16) |
        (static_cast<quint32>(p[2]) << 8) | static_cast<quint32>(p[3]);
    offset_ += 4;
    return true;
}

bool Reader::readMpi(QCA::BigInteger *value)
{
    if (!value)
        return false;

    const quint64 savedOffset = offset_;
    quint32 length = 0;
    if (!readInt(&length) || static_cast<quint64>(length) > remaining()) {
        offset_ = savedOffset;
        return false;
    }

    QByteArray bytes;
    if (!readBytes(length, &bytes)) {
        offset_ = savedOffset;
        return false;
    }

    // OTR MPIs are unsigned and MUST use minimal encoding. Zero is encoded as
    // a zero-length MPI; any non-empty MPI beginning with 0x00 is invalid.
    if (!bytes.isEmpty() && bytes.front() == '\0') {
        offset_ = savedOffset;
        return false;
    }

    *value = unsignedInteger(bytes);
    return true;
}

bool Reader::readData(QByteArray *value)
{
    if (!value)
        return false;

    const quint64 savedOffset = offset_;
    quint32 length = 0;
    if (!readInt(&length) || !readBytes(length, value)) {
        offset_ = savedOffset;
        return false;
    }
    return true;
}

bool Reader::readDsaPublicKey(DsaPublicKey *key)
{
    if (!key)
        return false;

    const quint64 savedOffset = offset_;
    quint16 type = 0;
    DsaPublicKey decoded;
    if (!readShort(&type) || type != 0 || !readMpi(&decoded.domain.p) || !readMpi(&decoded.domain.q) ||
        !readMpi(&decoded.domain.g) || !readMpi(&decoded.y)) {
        offset_ = savedOffset;
        return false;
    }

    *key = decoded;
    return true;
}

QByteArray encodeMpi(const QCA::BigInteger &value, bool *ok)
{
    Writer writer;
    const bool success = writer.writeMpi(value);
    if (ok)
        *ok = success;
    return success ? writer.take() : QByteArray();
}

bool decodeMpi(const QByteArray &encoded, QCA::BigInteger *value)
{
    Reader reader(encoded);
    return reader.readMpi(value) && reader.atEnd();
}

QByteArray encodeDsaPublicKey(const DsaPublicKey &key, bool *ok)
{
    Writer writer;
    const bool success = writer.writeDsaPublicKey(key);
    if (ok)
        *ok = success;
    return success ? writer.take() : QByteArray();
}

bool decodeDsaPublicKey(const QByteArray &encoded, DsaPublicKey *key)
{
    Reader reader(encoded);
    return reader.readDsaPublicKey(key) && reader.atEnd();
}

} // namespace QcaOtr::Wire
