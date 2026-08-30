/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "qca-otr/crypto.h"

#include <QByteArray>
#include <QtGlobal>

#include <utility>

namespace QcaOtr::Wire {

/** Encoder for OTR's network-byte-order primitive wire types. */
class Writer
{
public:
    void writeByte(quint8 value);
    void writeShort(quint16 value);
    void writeInt(quint32 value);
    void writeBytes(const QByteArray &value);

    /** Writes an OTR MPI; returns false when @p value cannot be encoded. */
    bool writeMpi(const QCA::BigInteger &value);

    /** Writes an OTR DATA field: a 32-bit length followed by the bytes. */
    void writeData(const QByteArray &value);

    /** Writes the protocol DSA public-key representation. */
    bool writeDsaPublicKey(const DsaPublicKey &key);

    /** Returns the currently encoded bytes without transferring ownership. */
    const QByteArray &data() const { return data_; }

    /** Transfers the encoded bytes out of the writer. */
    QByteArray take() { return std::move(data_); }

private:
    QByteArray data_;
};

/** Bounds-checking decoder for OTR primitive wire types. */
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

    /** Returns true only when the complete input has been consumed. */
    bool atEnd() const { return offset_ == static_cast<quint64>(data_.size()); }

    /** Returns the number of unread input bytes. */
    quint64 remaining() const { return static_cast<quint64>(data_.size()) - offset_; }

private:
    QByteArray data_;
    quint64 offset_ = 0;
};

/** Encodes one OTR MPI into ordinary memory. */
QByteArray encodeMpi(const QCA::BigInteger &value, bool *ok = nullptr);

/** Encodes one OTR MPI while retaining the serialized value in secure memory. */
QCA::SecureArray encodeMpiSecure(const QCA::BigInteger &value, bool *ok = nullptr);

/** Decodes exactly one OTR MPI. */
bool decodeMpi(const QByteArray &encoded, QCA::BigInteger *value);

/** Encodes one OTR DSA public key. */
QByteArray encodeDsaPublicKey(const DsaPublicKey &key, bool *ok = nullptr);

/** Decodes exactly one OTR DSA public key. */
bool decodeDsaPublicKey(const QByteArray &encoded, DsaPublicKey *key);

} // namespace QcaOtr::Wire
