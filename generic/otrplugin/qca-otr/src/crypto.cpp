#include "qca-otr/crypto.h"

namespace QcaOtr {
namespace {

const QCA::BigInteger Zero(0);
const QCA::BigInteger One(1);
const QCA::BigInteger Two(2);

QCA::BigInteger unsignedInteger(const QByteArray &bytes)
{
    QByteArray positive;
    positive.reserve(bytes.size() + 1);
    positive.append('\0');
    positive.append(bytes);
    return QCA::BigInteger(QCA::SecureArray(positive));
}

bool validDomain(const DsaDomain &domain)
{
    return domain.p > Two && domain.q > One && domain.g > One && domain.g < domain.p;
}

QCA::BigInteger randomScalar(const QCA::BigInteger &q)
{
    QCA::BigInteger range(q);
    range -= One;

    const int byteCount = q.toArray().size() + 1;
    QCA::SecureArray bytes = QCA::Random::randomArray(byteCount);
    if (!bytes.isEmpty())
        bytes[0] = 0;

    QCA::BigInteger value(bytes);
    value %= range;
    value += One;
    return value;
}

} // namespace

QCA::BigInteger positiveMod(const QCA::BigInteger &value, const QCA::BigInteger &modulus)
{
    if (modulus <= Zero)
        return Zero;

    QCA::BigInteger result(value);
    result %= modulus;
    if (result < Zero)
        result += modulus;
    return result;
}

QCA::BigInteger modPow(const QCA::BigInteger &base,
                       const QCA::BigInteger &exponent,
                       const QCA::BigInteger &modulus)
{
    if (modulus <= Zero || exponent < Zero)
        return Zero;

    QCA::BigInteger result(1);
    result %= modulus;

    QCA::BigInteger factor = positiveMod(base, modulus);
    QCA::BigInteger power(exponent);

    while (power > Zero) {
        QCA::BigInteger bit(power);
        bit %= Two;
        if (bit != Zero) {
            result *= factor;
            result %= modulus;
        }

        power /= Two;
        if (power != Zero) {
            factor *= factor;
            factor %= modulus;
        }
    }

    return result;
}

bool modInverse(const QCA::BigInteger &value, const QCA::BigInteger &modulus, QCA::BigInteger *inverse)
{
    if (!inverse || modulus <= One)
        return false;

    QCA::BigInteger t(0);
    QCA::BigInteger newT(1);
    QCA::BigInteger r(modulus);
    QCA::BigInteger newR = positiveMod(value, modulus);

    while (newR != Zero) {
        QCA::BigInteger quotient(r);
        quotient /= newR;

        QCA::BigInteger product(quotient);
        product *= newT;
        QCA::BigInteger nextT(t);
        nextT -= product;
        t = newT;
        newT = nextT;

        product = quotient;
        product *= newR;
        QCA::BigInteger nextR(r);
        nextR -= product;
        r = newR;
        newR = nextR;
    }

    if (r != One)
        return false;

    *inverse = positiveMod(t, modulus);
    return true;
}

DsaPublicKey dsaPublicKey(const DsaPrivateKey &privateKey)
{
    DsaPublicKey result;
    result.domain = privateKey.domain;
    if (validDomain(privateKey.domain) && privateKey.x > Zero && privateKey.x < privateKey.domain.q)
        result.y = modPow(privateKey.domain.g, privateKey.x, privateKey.domain.p);
    return result;
}

bool dsaSignDigest(const DsaPrivateKey &privateKey, const QByteArray &digest, DsaSignature *signature)
{
    if (!signature || !validDomain(privateKey.domain) || privateKey.x <= Zero ||
        privateKey.x >= privateKey.domain.q) {
        return false;
    }

    const QCA::BigInteger m = positiveMod(unsignedInteger(digest), privateKey.domain.q);

    // r == 0 or s == 0 is valid reason to choose a fresh DSA nonce. Both are
    // vanishingly unlikely for real OTR parameters, but keep a finite guard.
    for (int attempt = 0; attempt < 128; ++attempt) {
        const QCA::BigInteger k = randomScalar(privateKey.domain.q);

        QCA::BigInteger kInverse;
        if (!modInverse(k, privateKey.domain.q, &kInverse))
            continue;

        QCA::BigInteger r = modPow(privateKey.domain.g, k, privateKey.domain.p);
        r %= privateKey.domain.q;
        if (r == Zero)
            continue;

        QCA::BigInteger xr(privateKey.x);
        xr *= r;
        xr += m;
        xr %= privateKey.domain.q;

        QCA::BigInteger s(kInverse);
        s *= xr;
        s %= privateKey.domain.q;
        if (s == Zero)
            continue;

        signature->r = r;
        signature->s = s;
        return true;
    }

    return false;
}

bool dsaVerifyDigest(const DsaPublicKey &publicKey, const QByteArray &digest, const DsaSignature &signature)
{
    if (!validDomain(publicKey.domain) || publicKey.y <= Zero || publicKey.y >= publicKey.domain.p ||
        signature.r <= Zero || signature.r >= publicKey.domain.q || signature.s <= Zero ||
        signature.s >= publicKey.domain.q) {
        return false;
    }

    QCA::BigInteger w;
    if (!modInverse(signature.s, publicKey.domain.q, &w))
        return false;

    QCA::BigInteger m = positiveMod(unsignedInteger(digest), publicKey.domain.q);

    QCA::BigInteger u1(m);
    u1 *= w;
    u1 %= publicKey.domain.q;

    QCA::BigInteger u2(signature.r);
    u2 *= w;
    u2 %= publicKey.domain.q;

    QCA::BigInteger v = modPow(publicKey.domain.g, u1, publicKey.domain.p);
    QCA::BigInteger yPart = modPow(publicKey.y, u2, publicKey.domain.p);
    v *= yPart;
    v %= publicKey.domain.p;
    v %= publicKey.domain.q;

    return v == signature.r;
}

QByteArray sha256(const QByteArray &data)
{
    if (!QCA::isSupported("sha256"))
        return {};

    QCA::Hash hash(QStringLiteral("sha256"));
    return hash.hash(data).toByteArray();
}

QByteArray hmacSha256(const QCA::SecureArray &key, const QByteArray &data)
{
    if (!QCA::isSupported("hmac(sha256)"))
        return {};

    QCA::MessageAuthenticationCode mac(QStringLiteral("hmac(sha256)"), QCA::SymmetricKey(key));
    mac.update(data);
    return mac.final().toByteArray();
}

bool aes128Ctr(const QCA::SecureArray &key,
               const QByteArray &counter,
               const QByteArray &input,
               QByteArray *output)
{
    if (!output || key.size() != 16 || counter.size() != 16 || !QCA::isSupported("aes128-ctr"))
        return false;

    QCA::Cipher cipher(QStringLiteral("aes128"),
                       QCA::Cipher::CTR,
                       QCA::Cipher::NoPadding,
                       QCA::Encode,
                       QCA::SymmetricKey(key),
                       QCA::InitializationVector(counter));

    QByteArray result = cipher.update(input).toByteArray();
    if (!cipher.ok())
        return false;

    result += cipher.final().toByteArray();
    if (!cipher.ok())
        return false;

    *output = result;
    return true;
}

} // namespace QcaOtr
