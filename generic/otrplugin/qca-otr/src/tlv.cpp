#include "qca-otr/tlv.h"

#include <limits>

namespace QcaOtr {

QByteArray encodeTlvs(const QVector<Tlv> &tlvs, bool *ok)
{
    if (ok)
        *ok = false;

    QByteArray encoded;
    for (const Tlv &tlv : tlvs) {
        if (tlv.value.size() > std::numeric_limits<quint16>::max())
            return {};

        const quint16 length = static_cast<quint16>(tlv.value.size());
        encoded.append(static_cast<char>((tlv.type >> 8) & 0xff));
        encoded.append(static_cast<char>(tlv.type & 0xff));
        encoded.append(static_cast<char>((length >> 8) & 0xff));
        encoded.append(static_cast<char>(length & 0xff));
        encoded.append(tlv.value);
    }

    if (ok)
        *ok = true;
    return encoded;
}

bool decodeTlvs(const QByteArray &encoded, QVector<Tlv> *tlvs)
{
    if (!tlvs)
        return false;

    QVector<Tlv> decoded;
    int offset = 0;
    while (offset < encoded.size()) {
        if (encoded.size() - offset < 4)
            return false;

        const auto *header = reinterpret_cast<const unsigned char *>(encoded.constData() + offset);
        const quint16 type = (static_cast<quint16>(header[0]) << 8) | header[1];
        const quint16 length = (static_cast<quint16>(header[2]) << 8) | header[3];
        offset += 4;

        if (encoded.size() - offset < length)
            return false;

        Tlv tlv;
        tlv.type = type;
        tlv.value = encoded.mid(offset, length);
        decoded.append(tlv);
        offset += length;
    }

    *tlvs = decoded;
    return true;
}

} // namespace QcaOtr
