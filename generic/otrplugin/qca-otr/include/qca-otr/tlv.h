#pragma once

#include <QByteArray>
#include <QVector>
#include <QtGlobal>

namespace QcaOtr {

enum class TlvType : quint16 {
    Padding = 0x0000,
    Disconnected = 0x0001,
    Smp1 = 0x0002,
    Smp2 = 0x0003,
    Smp3 = 0x0004,
    Smp4 = 0x0005,
    SmpAbort = 0x0006,
    Smp1Question = 0x0007,
    SymmetricKey = 0x0008
};

struct Tlv
{
    quint16 type = 0;
    QByteArray value;
};

QByteArray encodeTlvs(const QVector<Tlv> &tlvs, bool *ok = nullptr);
bool decodeTlvs(const QByteArray &encoded, QVector<Tlv> *tlvs);

} // namespace QcaOtr
