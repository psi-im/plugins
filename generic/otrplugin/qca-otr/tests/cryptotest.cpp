#include "qca-otr/crypto.h"

#include <QTest>
#include <QtCrypto>

class CryptoTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void rawDsa();
    void sha256();
    void hmacSha256();
    void aes128Ctr();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void CryptoTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
}

void CryptoTest::cleanupTestCase()
{
    delete initializer_;
}

void CryptoTest::rawDsa()
{
    // Small mathematically valid DSA group. Production OTR keys use the
    // protocol-defined 1024/160-bit DSA parameters; the tiny values here make
    // the unit test easy to audit.
    QcaOtr::DsaPrivateKey privateKey;
    privateKey.domain.p = QCA::BigInteger(23);
    privateKey.domain.q = QCA::BigInteger(11);
    privateKey.domain.g = QCA::BigInteger(2);
    privateKey.x = QCA::BigInteger(3);

    const QcaOtr::DsaPublicKey publicKey = QcaOtr::dsaPublicKey(privateKey);
    QCOMPARE(publicKey.y, QCA::BigInteger(8));

    QcaOtr::DsaSignature signature;
    QVERIFY(QcaOtr::dsaSignDigest(privateKey, QByteArray::fromHex("09"), &signature));
    QVERIFY(QcaOtr::dsaVerifyDigest(publicKey, QByteArray::fromHex("09"), signature));
    QVERIFY(!QcaOtr::dsaVerifyDigest(publicKey, QByteArray::fromHex("0a"), signature));

    // OTR signs the 32-byte M_A/M_B HMAC value directly, without hashing it
    // again. Interpret the complete digest as an unsigned big-endian integer;
    // the DSA arithmetic itself reduces it modulo q. With nonce k=1 this
    // vector has r=2, s=9 for the toy key above.
    QcaOtr::DsaSignature rawDigestSignature;
    rawDigestSignature.r = QCA::BigInteger(2);
    rawDigestSignature.s = QCA::BigInteger(9);
    QVERIFY(QcaOtr::dsaVerifyDigest(publicKey, QByteArray(32, static_cast<char>(0xf0)), rawDigestSignature));

    QcaOtr::DsaSignature damaged = signature;
    damaged.r += QCA::BigInteger(1);
    QVERIFY(!QcaOtr::dsaVerifyDigest(publicKey, QByteArray::fromHex("09"), damaged));
}

void CryptoTest::sha256()
{
    QVERIFY2(QCA::isSupported("sha256"), "QCA provider with SHA-256 support is required");
    QCOMPARE(QcaOtr::sha256(QByteArrayLiteral("abc")).toHex(),
             QByteArrayLiteral("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
}

void CryptoTest::hmacSha256()
{
    QVERIFY2(QCA::isSupported("hmac(sha256)"), "QCA provider with HMAC-SHA256 support is required");

    const QCA::SecureArray key(QByteArrayLiteral("Jefe"));
    const QByteArray data("what do ya want for nothing?");
    QCOMPARE(QcaOtr::hmacSha256(key, data).toHex(),
             QByteArrayLiteral("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"));
}

void CryptoTest::aes128Ctr()
{
    QVERIFY2(QCA::isSupported("aes128-ctr"), "QCA provider with AES-128-CTR support is required");

    const QCA::SecureArray key(QByteArray::fromHex("2b7e151628aed2a6abf7158809cf4f3c"));
    const QByteArray counter = QByteArray::fromHex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    const QByteArray plain = QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a");
    const QByteArray expected = QByteArray::fromHex("874d6191b620e3261bef6864990db6ce");

    QByteArray encrypted;
    QVERIFY(QcaOtr::aes128Ctr(key, counter, plain, &encrypted));
    QCOMPARE(encrypted, expected);

    QByteArray decrypted;
    QVERIFY(QcaOtr::aes128Ctr(key, counter, encrypted, &decrypted));
    QCOMPARE(decrypted, plain);
}

QTEST_GUILESS_MAIN(CryptoTest)
#include "cryptotest.moc"
