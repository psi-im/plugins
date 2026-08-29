#include "qca-otr/ake.h"
#include "qca-otr/codec.h"

#include <QTest>
#include <QtCrypto>

class AkeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void dhParameters();
    void dhKeyAgreement();
    void keyDerivation();
    void signatureInputs();
    void innerAuthenticator();
    void messageEncoding();
    void rejectsMalformedMessages();

private:
    QCA::Initializer *initializer_ = nullptr;
};

void AkeTest::initTestCase()
{
    initializer_ = new QCA::Initializer;
}

void AkeTest::cleanupTestCase()
{
    delete initializer_;
}

void AkeTest::dhParameters()
{
    QCOMPARE(QcaOtr::dhGenerator(), QCA::BigInteger(2));

    bool ok = false;
    const QByteArray encoded = QcaOtr::Wire::encodeMpi(QcaOtr::dhModulus(), &ok);
    QVERIFY(ok);
    QCOMPARE(encoded.left(4).toHex(), QByteArrayLiteral("000000c0"));
    QCOMPARE(encoded.mid(4).toHex(),
             QByteArrayLiteral(
                 "ffffffffffffffffc90fdaa22168c234c4c6628b80dc1cd1"
                 "29024e088a67cc74020bbea63b139b22514a08798e3404dd"
                 "ef9519b3cd3a431b302b0a6df25f14374fe1356d6d51c245"
                 "e485b576625e7ec6f44c42e9a637ed6b0bff5cb6f406b7ed"
                 "ee386bfb5a899fa5ae9f24117c4b1fe649286651ece45b3d"
                 "c2007cb8a163bf0598da48361c55d39a69163fa8fd24cf5f"
                 "83655d23dca3ad961c62f356208552bb9ed529077096966d"
                 "670c354e4abc9804f1746c08ca237327ffffffffffffffff"));

    QVERIFY(!QcaOtr::isValidDhPublicValue(QCA::BigInteger(1)));
    QVERIFY(QcaOtr::isValidDhPublicValue(QCA::BigInteger(2)));

    QCA::BigInteger maximum = QcaOtr::dhModulus();
    maximum -= QCA::BigInteger(2);
    QVERIFY(QcaOtr::isValidDhPublicValue(maximum));
    maximum += QCA::BigInteger(1);
    QVERIFY(!QcaOtr::isValidDhPublicValue(maximum));

    QcaOtr::DhKeyPair generated;
    QVERIFY(QcaOtr::generateDhKeyPair(&generated));
    QVERIFY(generated.privateExponent > QCA::BigInteger(0));
    QVERIFY(QcaOtr::isValidDhPublicValue(generated.publicValue));
}

void AkeTest::dhKeyAgreement()
{
    const QCA::BigInteger x(0x12345);
    const QCA::BigInteger y(0x23456);
    const QCA::BigInteger gx = QCA::BigIntegerMath::modPow(QcaOtr::dhGenerator(), x, QcaOtr::dhModulus());
    const QCA::BigInteger gy = QCA::BigIntegerMath::modPow(QcaOtr::dhGenerator(), y, QcaOtr::dhModulus());

    QCA::BigInteger fromX;
    QCA::BigInteger fromY;
    QVERIFY(QcaOtr::computeDhSharedSecret(x, gy, &fromX));
    QVERIFY(QcaOtr::computeDhSharedSecret(y, gx, &fromY));
    QCOMPARE(fromX, fromY);

    bool ok = false;
    const QByteArray encoded = QcaOtr::Wire::encodeMpi(fromX, &ok);
    QVERIFY(ok);
    QCOMPARE(encoded.mid(4).toHex(),
             QByteArrayLiteral(
                 "80fa4e533780b22ad49a998be8fcb7ab1745cd63b37dbe89b8c3ebedc5c733bb"
                 "8416d054c32ff37e231b04f3ec94b7af3b474fd7352958c1702f32ec971b943b"
                 "3567b7bd5e13ba9766ece4d6193f3f12f206c98f8a280c2de4594ed569ac622d"
                 "74784cc70a8b2ba6b702169ea4e10626aee2d771befbc45a15dbee8da7d98c70"
                 "d62f245739790463a11348e5f46bd8496077675b357536fd2c51bd47b4aead39e"
                 "889f1a73031ed06623cc398bbfffab76612e2baf7b2bf14c3a882dca1facec4"));

    QVERIFY(!QcaOtr::computeDhSharedSecret(x, QCA::BigInteger(1), &fromX));
}

void AkeTest::keyDerivation()
{
    QcaOtr::AkeKeys keys;
    QVERIFY(QcaOtr::deriveAkeKeys(QCA::BigInteger("81985529216486895"), &keys));

    QCOMPARE(keys.sessionId.toHex(), QByteArrayLiteral("534adebb077e7804"));
    QCOMPARE(keys.c.toByteArray().toHex(), QByteArrayLiteral("fea6bc3378decf96c4bfb74f2d9fced0"));
    QCOMPARE(keys.cPrime.toByteArray().toHex(), QByteArrayLiteral("b4c2c04fd93df0bd62dc3a1fc81de2cd"));
    QCOMPARE(keys.m1.toByteArray().toHex(),
             QByteArrayLiteral("fabfea340eddca81e5cef27fa7b5335313498b1ee3a1b9a1d49291e684835c2b"));
    QCOMPARE(keys.m2.toByteArray().toHex(),
             QByteArrayLiteral("617f4920841e671df8c90ebca16c370302460a8e13ba4a32adaad460ac9400d0"));
    QCOMPARE(keys.m1Prime.toByteArray().toHex(),
             QByteArrayLiteral("37b1a76453a18fc942d3f967c46b4d173f1859e2d895bfb62bc03d70d5f3d09f"));
    QCOMPARE(keys.m2Prime.toByteArray().toHex(),
             QByteArrayLiteral("5e7d3c09827774054ca4bece51ad48969c922f0bd387a92151cf13811af624e5"));
}

void AkeTest::signatureInputs()
{
    QcaOtr::DsaPublicKey publicKey;
    publicKey.domain.p = QCA::BigInteger(23);
    publicKey.domain.q = QCA::BigInteger(11);
    publicKey.domain.g = QCA::BigInteger(2);
    publicKey.y = QCA::BigInteger(8);

    const QCA::SecureArray m1(
        QByteArray::fromHex("fabfea340eddca81e5cef27fa7b5335313498b1ee3a1b9a1d49291e684835c2b"));
    QCOMPARE(QcaOtr::akeSignatureDigest(QCA::BigInteger(8), QCA::BigInteger(16), publicKey, 1, m1).toHex(),
             QByteArrayLiteral("549b34c8f3a74407623c7947fbec527d01d8a6ba2d4c495a4450e6d8480d1603"));

    const QCA::SecureArray m2(
        QByteArray::fromHex("617f4920841e671df8c90ebca16c370302460a8e13ba4a32adaad460ac9400d0"));
    QCOMPARE(QcaOtr::akeSignatureMac(QByteArray::fromHex("deadbeef"), m2).toHex(),
             QByteArrayLiteral("8614e209a0009f7b8189ba83c88079b7cfb4cc95"));

    QVERIFY(QcaOtr::akeSignatureDigest(QCA::BigInteger(8), QCA::BigInteger(16), publicKey, 0, m1).isEmpty());
}

void AkeTest::innerAuthenticator()
{
    QcaOtr::DsaPrivateKey privateKey;
    privateKey.domain.p = QCA::BigInteger(23);
    privateKey.domain.q = QCA::BigInteger(11);
    privateKey.domain.g = QCA::BigInteger(2);
    privateKey.x = QCA::BigInteger(3);
    const QcaOtr::DsaPublicKey publicKey = QcaOtr::dsaPublicKey(privateKey);

    QCOMPARE(QcaOtr::dsaPublicKeyFingerprint(publicKey).toHex(),
             QByteArrayLiteral("69b8710fa263a4f067eef5404043c21dc88e534f"));

    const QCA::SecureArray macKey(
        QByteArray::fromHex("fabfea340eddca81e5cef27fa7b5335313498b1ee3a1b9a1d49291e684835c2b"));
    const QCA::SecureArray encryptionKey(QByteArray::fromHex("fea6bc3378decf96c4bfb74f2d9fced0"));

    QByteArray encrypted;
    QVERIFY(QcaOtr::createAkeAuthenticator(privateKey,
                                            1,
                                            QCA::BigInteger(8),
                                            QCA::BigInteger(16),
                                            macKey,
                                            encryptionKey,
                                            &encrypted));
    QVERIFY(!encrypted.isEmpty());

    QcaOtr::AkeAuthenticator decoded;
    QByteArray fingerprint;
    QVERIFY(QcaOtr::verifyAkeAuthenticator(encrypted,
                                            QCA::BigInteger(8),
                                            QCA::BigInteger(16),
                                            macKey,
                                            encryptionKey,
                                            &decoded,
                                            &fingerprint));
    QCOMPARE(decoded.keyId, quint32(1));
    QCOMPARE(decoded.publicKey.domain.p, privateKey.domain.p);
    QCOMPARE(decoded.publicKey.domain.q, privateKey.domain.q);
    QCOMPARE(decoded.publicKey.domain.g, privateKey.domain.g);
    QCOMPARE(decoded.publicKey.y, publicKey.y);
    QCOMPARE(fingerprint, QcaOtr::dsaPublicKeyFingerprint(publicKey));

    QByteArray damaged = encrypted;
    damaged[damaged.size() / 2] = static_cast<char>(damaged.at(damaged.size() / 2) ^ 0x40);
    QVERIFY(!QcaOtr::verifyAkeAuthenticator(damaged,
                                             QCA::BigInteger(8),
                                             QCA::BigInteger(16),
                                             macKey,
                                             encryptionKey,
                                             &decoded));

    QVERIFY(!QcaOtr::verifyAkeAuthenticator(encrypted,
                                             QCA::BigInteger(16),
                                             QCA::BigInteger(8),
                                             macKey,
                                             encryptionKey,
                                             &decoded));
}

void AkeTest::messageEncoding()
{
    constexpr quint32 sender = 0x01020304;
    constexpr quint32 receiver = 0xa1b2c3d4;

    QcaOtr::DhCommitMessage commit;
    commit.senderInstance = sender;
    commit.receiverInstance = receiver;
    commit.encryptedGx = QByteArray::fromHex("deadbeef");
    for (int i = 0; i < 32; ++i)
        commit.hashedGx.append(static_cast<char>(i));

    bool ok = false;
    const QByteArray encodedCommit = QcaOtr::Wire::encodeDhCommitMessage(commit, &ok);
    QVERIFY(ok);
    QCOMPARE(encodedCommit.toHex(),
             QByteArrayLiteral(
                 "00030201020304a1b2c3d400000004deadbeef00000020000102030405060708090a0b0c0d0e0f"
                 "101112131415161718191a1b1c1d1e1f"));
    QcaOtr::DhCommitMessage decodedCommit;
    QVERIFY(QcaOtr::Wire::decodeDhCommitMessage(encodedCommit, &decodedCommit));
    QCOMPARE(decodedCommit.senderInstance, sender);
    QCOMPARE(decodedCommit.receiverInstance, receiver);
    QCOMPARE(decodedCommit.encryptedGx, commit.encryptedGx);
    QCOMPARE(decodedCommit.hashedGx, commit.hashedGx);

    QcaOtr::DhKeyMessage dhKey;
    dhKey.senderInstance = sender;
    dhKey.receiverInstance = receiver;
    dhKey.dhPublicValue = QCA::BigInteger(128);
    const QByteArray encodedDhKey = QcaOtr::Wire::encodeDhKeyMessage(dhKey, &ok);
    QVERIFY(ok);
    QCOMPARE(encodedDhKey.toHex(), QByteArrayLiteral("00030a01020304a1b2c3d40000000180"));
    QcaOtr::DhKeyMessage decodedDhKey;
    QVERIFY(QcaOtr::Wire::decodeDhKeyMessage(encodedDhKey, &decodedDhKey));
    QCOMPARE(decodedDhKey.dhPublicValue, dhKey.dhPublicValue);

    QcaOtr::RevealSignatureMessage reveal;
    reveal.senderInstance = sender;
    reveal.receiverInstance = receiver;
    for (int i = 0; i < 16; ++i)
        reveal.revealedKey.append(static_cast<char>(i));
    reveal.encryptedSignature = QByteArray::fromHex("aa55");
    reveal.mac = QByteArray(20, '\x11');
    const QByteArray encodedReveal = QcaOtr::Wire::encodeRevealSignatureMessage(reveal, &ok);
    QVERIFY(ok);
    QCOMPARE(encodedReveal.toHex(),
             QByteArrayLiteral(
                 "00031101020304a1b2c3d400000010000102030405060708090a0b0c0d0e0f00000002aa55"
                 "1111111111111111111111111111111111111111"));
    QcaOtr::RevealSignatureMessage decodedReveal;
    QVERIFY(QcaOtr::Wire::decodeRevealSignatureMessage(encodedReveal, &decodedReveal));
    QCOMPARE(decodedReveal.revealedKey, reveal.revealedKey);
    QCOMPARE(decodedReveal.encryptedSignature, reveal.encryptedSignature);
    QCOMPARE(decodedReveal.mac, reveal.mac);

    QcaOtr::SignatureMessage signature;
    signature.senderInstance = sender;
    signature.receiverInstance = receiver;
    signature.encryptedSignature = QByteArray::fromHex("010203");
    signature.mac = QByteArray(20, '\x22');
    const QByteArray encodedSignature = QcaOtr::Wire::encodeSignatureMessage(signature, &ok);
    QVERIFY(ok);
    QCOMPARE(encodedSignature.toHex(),
             QByteArrayLiteral("00031201020304a1b2c3d4000000030102032222222222222222222222222222222222222222"));
    QcaOtr::SignatureMessage decodedSignature;
    QVERIFY(QcaOtr::Wire::decodeSignatureMessage(encodedSignature, &decodedSignature));
    QCOMPARE(decodedSignature.encryptedSignature, signature.encryptedSignature);
    QCOMPARE(decodedSignature.mac, signature.mac);
}

void AkeTest::rejectsMalformedMessages()
{
    QcaOtr::DhCommitMessage commit;
    commit.senderInstance = 0x00000100;
    commit.receiverInstance = 0;
    commit.encryptedGx = QByteArray::fromHex("00000001");
    commit.hashedGx = QByteArray(32, '\x42');

    bool ok = false;
    QByteArray encoded = QcaOtr::Wire::encodeDhCommitMessage(commit, &ok);
    QVERIFY(ok);

    QcaOtr::DhCommitMessage decoded;
    QByteArray wrongVersion = encoded;
    wrongVersion[1] = '\x02';
    QVERIFY(!QcaOtr::Wire::decodeDhCommitMessage(wrongVersion, &decoded));

    QByteArray badSender = encoded;
    badSender.replace(3, 4, QByteArray(4, '\0'));
    QVERIFY(!QcaOtr::Wire::decodeDhCommitMessage(badSender, &decoded));

    QByteArray trailing = encoded;
    trailing.append('\0');
    QVERIFY(!QcaOtr::Wire::decodeDhCommitMessage(trailing, &decoded));

    QByteArray truncated = encoded;
    truncated.chop(1);
    QVERIFY(!QcaOtr::Wire::decodeDhCommitMessage(truncated, &decoded));

    QcaOtr::DhKeyMessage invalidDh;
    invalidDh.senderInstance = 0x00000100;
    invalidDh.dhPublicValue = QCA::BigInteger(1);
    QVERIFY(QcaOtr::Wire::encodeDhKeyMessage(invalidDh, &ok).isEmpty());
    QVERIFY(!ok);

    QcaOtr::RevealSignatureMessage badReveal;
    badReveal.senderInstance = 0x00000100;
    badReveal.revealedKey = QByteArray(15, '\0');
    badReveal.mac = QByteArray(20, '\0');
    QVERIFY(QcaOtr::Wire::encodeRevealSignatureMessage(badReveal, &ok).isEmpty());
    QVERIFY(!ok);

    QcaOtr::SignatureMessage badSignature;
    badSignature.senderInstance = 0x00000100;
    badSignature.mac = QByteArray(19, '\0');
    QVERIFY(QcaOtr::Wire::encodeSignatureMessage(badSignature, &ok).isEmpty());
    QVERIFY(!ok);
}

QTEST_GUILESS_MAIN(AkeTest)
#include "aketest.moc"
