#pragma once

#include <QByteArray>
#include <QtCrypto>

namespace QcaOtr {

struct DsaDomain
{
    QCA::BigInteger p;
    QCA::BigInteger q;
    QCA::BigInteger g;
};

struct DsaPrivateKey
{
    DsaDomain       domain;
    QCA::BigInteger x;
};

struct DsaPublicKey
{
    DsaDomain       domain;
    QCA::BigInteger y;
};

struct DsaSignature
{
    QCA::BigInteger r;
    QCA::BigInteger s;
};

DsaPublicKey dsaPublicKey(const DsaPrivateKey &privateKey);
bool dsaSignDigest(const DsaPrivateKey &privateKey, const QByteArray &digest, DsaSignature *signature);
bool dsaVerifyDigest(const DsaPublicKey &publicKey, const QByteArray &digest, const DsaSignature &signature);

QByteArray sha256(const QByteArray &data);
QByteArray hmacSha256(const QCA::SecureArray &key, const QByteArray &data);

// OTRv3 uses AES-128 in CTR mode. Encryption and decryption are the same
// operation, so this helper intentionally has no direction argument.
bool aes128Ctr(const QCA::SecureArray &key,
               const QByteArray &counter,
               const QByteArray &input,
               QByteArray *output);

} // namespace QcaOtr
