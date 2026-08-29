#include "qca-otr/crypto.h"

namespace QcaOtr {
namespace {

QCA::BigInteger zero()
{
    return QCA::BigInteger(0);
}

QCA::BigInteger one()
{
    return QCA::BigInteger(1);
}

QCA::BigInteger two()
{
    return QCA::BigInteger(2);
}

QCA::BigInteger positiveMod(const QCA::BigInteger &value, const QCA::BigInteger &modulus)
{
    if (modulus <= zero())
        return zero();

    QCA::BigInteger result(value);
    result %= modulus;
    if (result < zero())
        result += modulus;
    return result;
}

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
    return domain.p > two() && domain.q > one() && domain.g > one() && domain.g < domain.p;
}

QCA::BigInteger randomScalar(const QCA::BigInteger &q)
{
    QByteArray limit = q.toArray().toByteArray();
    while (!limit.isEmpty() && limit.front() == '\0')
        limit.remove(0, 1);
    if (limit.isEmpty())
        return {};

    // Rejection sampling avoids the modulo bias of reducing a random integer
    // modulo q. DSA nonces are secret values and must be uniformly selected
    // from [1, q - 1].
    for (int attempt = 0; attempt < 256; ++attempt) {
        const QCA::SecureArray random = QCA::Random::randomArray(limit.size());
        if (random.size() != limit.size())
            return {};

        const QCA::BigInteger value = unsignedInteger(random.toByteArray());
        if (value > zero() && value < q)
            return value;
    }

    return {};
}

} // namespace

DsaPublicKey dsaPublicKey(const DsaPrivateKey &privateKey)
{
    DsaPublicKey result;
    result.domain = privateKey.domain;
    if (validDomain(privateKey.domain) && privateKey.x > zero() && privateKey.x < privateKey.domain.q) {
        result.y = QCA::BigIntegerMath::modPow(privateKey.domain.g, privateKey.x, privateKey.domain.p);
    }
    return result;
}

bool dsaSignDigest(const DsaPrivateKey &privateKey, const QByteArray &digest, DsaSignature *signature)
{
    if (!signature || !validDomain(privateKey.domain) || privateKey.x <= zero() ||
        privateKey.x >= privateKey.domain.q) {
        return false;
    }

    const QCA::BigInteger m = positiveMod(unsignedInteger(digest), privateKey.domain.q);

    // r == 0 or s == 0 is valid reason to choose a fresh DSA nonce. Both are
    // vanishingly unlikely for real OTR parameters, but keep a finite guard.
    for (int attempt = 0; attempt < 128; ++attempt) {
        const QCA::BigInteger k = randomScalar(privateKey.domain.q);
        if (k <= zero())
            return false;

        QCA::BigInteger kInverse;
        if (!QCA::BigIntegerMath::modInverse(k, privateKey.domain.q, &kInverse))
            continue;

        QCA::BigInteger r = QCA::BigIntegerMath::modPow(privateKey.domain.g, k, privateKey.domain.p);
        r %= privateKey.domain.q;
        if (r == zero())
            continue;

        QCA::BigInteger xr(privateKey.x);
        xr *= r;
        xr += m;
        xr %= privateKey.domain.q;

        QCA::BigInteger s(kInverse);
        s *= xr;
        s %= privateKey.domain.q;
        if (s == zero())
            continue;

        signature->r = r;
        signature->s = s;
        return true;
    }

    return false;
}

bool dsaVerifyDigest(const DsaPublicKey &publicKey, const QByteArray &digest, const DsaSignature &signature)
{
    if (!validDomain(publicKey.domain) || publicKey.y <= zero() || publicKey.y >= publicKey.domain.p ||
        signature.r <= zero() || signature.r >= publicKey.domain.q || signature.s <= zero() ||
        signature.s >= publicKey.domain.q) {
        return false;
    }

    QCA::BigInteger w;
    if (!QCA::BigIntegerMath::modInverse(signature.s, publicKey.domain.q, &w))
        return false;

    QCA::BigInteger m = positiveMod(unsignedInteger(digest), publicKey.domain.q);

    QCA::BigInteger u1(m);
    u1 *= w;
    u1 %= publicKey.domain.q;

    QCA::BigInteger u2(signature.r);
    u2 *= w;
    u2 %= publicKey.domain.q;

    QCA::BigInteger v = QCA::BigIntegerMath::modPow(publicKey.domain.g, u1, publicKey.domain.p);
    QCA::BigInteger yPart = QCA::BigIntegerMath::modPow(publicKey.y, u2, publicKey.domain.p);
    v *= yPart;
    v %= publicKey.domain.p;
    v %= publicKey.domain.q;

    return v == signature.r;
}

QByteArray sha1(const QByteArray &data)
{
    if (!QCA::isSupported("sha1"))
        return {};

    QCA::Hash hash(QStringLiteral("sha1"));
    return hash.hash(data).toByteArray();
}

QByteArray sha256(const QByteArray &data)
{
    if (!QCA::isSupported("sha256"))
        return {};

    QCA::Hash hash(QStringLiteral("sha256"));
    return hash.hash(data).toByteArray();
}

QByteArray hmacSha1(const QCA::SecureArray &key, const QByteArray &data)
{
    if (!QCA::isSupported("hmac(sha1)"))
        return {};

    QCA::MessageAuthenticationCode mac(QStringLiteral("hmac(sha1)"), QCA::SymmetricKey(key));
    mac.update(data);
    return mac.final().toByteArray();
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
