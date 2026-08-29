#pragma once

#include "qca-otr/crypto.h"

#include <QByteArray>
#include <QtGlobal>

#include <utility>

namespace QcaOtr::Wire {

class Writer
{
public:
    void writeByte(quint8 value);
    void writeShort(quint16 value);
    void writeInt(quint32 value);
    void writeBytes(const QByteArray &value);
    bool writeMpi(const QCA::BigInteger &value);
    void writeData(const QByteArray &value);
    bool writeDsaPublicKey(const DsaPublicKey &key);

    const QByteArray &data() const { return data_; }
    QByteArray take() { return std::move(data_); }

private:
    QByteArray data_;
};

class Reader
{
public:
    explicit Reader(const QByteArray &data) : data_(data) {}

    bool readByte(quint8 *value);
    bool readShort(quint16 *value);
    bool readInt(quint32 *value);
    bool readBytes(quint32 length, QByteArray *value);
    bool readMpi(QCA::BigInteger *value);
    bool readData(QByteArray *value);
    bool readDsaPublicKey(DsaPublicKey *key);

    bool atEnd() const { return offset_ == static_cast<quint64>(data_.size()); }
    quint64 remaining() const { return static_cast<quint64>(data_.size()) - offset_; }

private:
    QByteArray data_;
    quint64 offset_ = 0;
};

QByteArray encodeMpi(const QCA::BigInteger &value, bool *ok = nullptr);
QCA::SecureArray encodeMpiSecure(const QCA::BigInteger &value, bool *ok = nullptr);
bool decodeMpi(const QByteArray &encoded, QCA::BigInteger *value);

QByteArray encodeDsaPublicKey(const DsaPublicKey &key, bool *ok = nullptr);
bool decodeDsaPublicKey(const QByteArray &encoded, DsaPublicKey *key);

} // namespace QcaOtr::Wire
