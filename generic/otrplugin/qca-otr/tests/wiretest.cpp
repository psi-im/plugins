#include "qca-otr/codec.h"

#include <QTest>

class WireTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void integers();
    void mpiEncoding();
    void mpiRejectsInvalidEncoding();
    void opaqueData();
    void dsaPublicKey();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void WireTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
}

void WireTest::cleanupTestCase()
{
    delete initializer_;
}

void WireTest::integers()
{
    QcaOtr::Wire::Writer writer;
    writer.writeByte(0xab);
    writer.writeShort(0xcdef);
    writer.writeInt(0x12345678);
    QCOMPARE(writer.data().toHex(), QByteArrayLiteral("abcdef12345678"));

    QcaOtr::Wire::Reader reader(writer.data());
    quint8 byte = 0;
    quint16 shortValue = 0;
    quint32 intValue = 0;
    QVERIFY(reader.readByte(&byte));
    QVERIFY(reader.readShort(&shortValue));
    QVERIFY(reader.readInt(&intValue));
    QCOMPARE(byte, quint8(0xab));
    QCOMPARE(shortValue, quint16(0xcdef));
    QCOMPARE(intValue, quint32(0x12345678));
    QVERIFY(reader.atEnd());
}

void WireTest::mpiEncoding()
{
    bool ok = false;
    QCOMPARE(QcaOtr::Wire::encodeMpi(QCA::BigInteger(0), &ok).toHex(), QByteArrayLiteral("00000000"));
    QVERIFY(ok);

    QCOMPARE(QcaOtr::Wire::encodeMpi(QCA::BigInteger(127), &ok).toHex(), QByteArrayLiteral("000000017f"));
    QVERIFY(ok);

    // QCA::BigInteger::toArray() needs a sign byte for 0x80. OTR MPI is
    // unsigned, so that byte must not appear on the wire.
    QCOMPARE(QcaOtr::Wire::encodeMpi(QCA::BigInteger(128), &ok).toHex(), QByteArrayLiteral("0000000180"));
    QVERIFY(ok);

    QCOMPARE(QcaOtr::Wire::encodeMpi(QCA::BigInteger(256), &ok).toHex(), QByteArrayLiteral("000000020100"));
    QVERIFY(ok);

    QCA::BigInteger decoded;
    QVERIFY(QcaOtr::Wire::decodeMpi(QByteArray::fromHex("0000000180"), &decoded));
    QCOMPARE(decoded, QCA::BigInteger(128));

    const QByteArray negative = QcaOtr::Wire::encodeMpi(QCA::BigInteger(-1), &ok);
    QVERIFY(!ok);
    QVERIFY(negative.isEmpty());
}

void WireTest::mpiRejectsInvalidEncoding()
{
    QCA::BigInteger value;

    // Leading zeroes violate OTR's minimum-length MPI encoding.
    QVERIFY(!QcaOtr::Wire::decodeMpi(QByteArray::fromHex("0000000100"), &value));
    QVERIFY(!QcaOtr::Wire::decodeMpi(QByteArray::fromHex("000000020001"), &value));

    // Length exceeds the remaining input.
    QVERIFY(!QcaOtr::Wire::decodeMpi(QByteArray::fromHex("0000000212"), &value));

    // A valid value followed by unconsumed bytes is not a complete encoded MPI.
    QVERIFY(!QcaOtr::Wire::decodeMpi(QByteArray::fromHex("000000011234"), &value));
}

void WireTest::opaqueData()
{
    QcaOtr::Wire::Writer writer;
    writer.writeData(QByteArray::fromHex("001122ff"));
    QCOMPARE(writer.data().toHex(), QByteArrayLiteral("00000004001122ff"));

    QByteArray decoded;
    QcaOtr::Wire::Reader reader(writer.data());
    QVERIFY(reader.readData(&decoded));
    QCOMPARE(decoded, QByteArray::fromHex("001122ff"));
    QVERIFY(reader.atEnd());

    QcaOtr::Wire::Reader truncated(QByteArray::fromHex("000000041122"));
    QVERIFY(!truncated.readData(&decoded));
    QCOMPARE(truncated.remaining(), quint64(6));
}

void WireTest::dsaPublicKey()
{
    QcaOtr::DsaPublicKey key;
    key.domain.p = QCA::BigInteger(23);
    key.domain.q = QCA::BigInteger(11);
    key.domain.g = QCA::BigInteger(2);
    key.y = QCA::BigInteger(8);

    bool ok = false;
    const QByteArray encoded = QcaOtr::Wire::encodeDsaPublicKey(key, &ok);
    QVERIFY(ok);
    QCOMPARE(encoded.toHex(),
             QByteArrayLiteral("0000"
                               "0000000117"
                               "000000010b"
                               "0000000102"
                               "0000000108"));

    QcaOtr::DsaPublicKey decoded;
    QVERIFY(QcaOtr::Wire::decodeDsaPublicKey(encoded, &decoded));
    QCOMPARE(decoded.domain.p, key.domain.p);
    QCOMPARE(decoded.domain.q, key.domain.q);
    QCOMPARE(decoded.domain.g, key.domain.g);
    QCOMPARE(decoded.y, key.y);

    QByteArray wrongType = encoded;
    wrongType[1] = '\x01';
    QVERIFY(!QcaOtr::Wire::decodeDsaPublicKey(wrongType, &decoded));
}

QTEST_GUILESS_MAIN(WireTest)
#include "wiretest.moc"
