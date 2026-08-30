#include "qca-otr/tlv.h"

#include <QTest>

class TlvTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void roundTripsKnownAndUnknownTypes();
    void acceptsZeroLengthAndBinaryValues();
    void rejectsTruncatedHeader();
    void rejectsTruncatedValue();
    void rejectsOversizedValue();
};

void TlvTest::roundTripsKnownAndUnknownTypes()
{
    QVector<QcaOtr::Tlv> tlvs;
    tlvs.append(QcaOtr::Tlv {static_cast<quint16>(QcaOtr::TlvType::Disconnected), QByteArray()});
    tlvs.append(QcaOtr::Tlv {0x1234, QByteArray::fromHex("deadbeef")});

    bool ok = false;
    const QByteArray encoded = QcaOtr::encodeTlvs(tlvs, &ok);
    QVERIFY(ok);
    QCOMPARE(encoded, QByteArray::fromHex("0001000012340004deadbeef"));

    QVector<QcaOtr::Tlv> decoded;
    QVERIFY(QcaOtr::decodeTlvs(encoded, &decoded));
    QCOMPARE(decoded.size(), 2);
    QCOMPARE(decoded.at(0).type, quint16(0x0001));
    QVERIFY(decoded.at(0).value.isEmpty());
    QCOMPARE(decoded.at(1).type, quint16(0x1234));
    QCOMPARE(decoded.at(1).value, QByteArray::fromHex("deadbeef"));
}

void TlvTest::acceptsZeroLengthAndBinaryValues()
{
    QVector<QcaOtr::Tlv> tlvs;
    tlvs.append(QcaOtr::Tlv {static_cast<quint16>(QcaOtr::TlvType::Padding), QByteArray()});
    tlvs.append(QcaOtr::Tlv {static_cast<quint16>(QcaOtr::TlvType::SymmetricKey),
                             QByteArray::fromHex("0000002a61006200ff")});

    bool ok = false;
    const QByteArray encoded = QcaOtr::encodeTlvs(tlvs, &ok);
    QVERIFY(ok);

    QVector<QcaOtr::Tlv> decoded;
    QVERIFY(QcaOtr::decodeTlvs(encoded, &decoded));
    QCOMPARE(decoded.size(), 2);
    QCOMPARE(decoded.at(1).value, QByteArray::fromHex("0000002a61006200ff"));

    QVector<QcaOtr::Tlv> empty;
    QVERIFY(QcaOtr::decodeTlvs({}, &empty));
    QVERIFY(empty.isEmpty());
}

void TlvTest::rejectsTruncatedHeader()
{
    QVector<QcaOtr::Tlv> decoded;
    QVERIFY(!QcaOtr::decodeTlvs(QByteArray::fromHex("0001ff"), &decoded));
}

void TlvTest::rejectsTruncatedValue()
{
    QVector<QcaOtr::Tlv> decoded;
    QVERIFY(!QcaOtr::decodeTlvs(QByteArray::fromHex("00010004aabb"), &decoded));
}

void TlvTest::rejectsOversizedValue()
{
    QVector<QcaOtr::Tlv> tlvs;
    tlvs.append(QcaOtr::Tlv {0x1234, QByteArray(65536, 'x')});
    bool ok = true;
    QVERIFY(QcaOtr::encodeTlvs(tlvs, &ok).isEmpty());
    QVERIFY(!ok);
}

QTEST_MAIN(TlvTest)
#include "tlvtest.moc"
