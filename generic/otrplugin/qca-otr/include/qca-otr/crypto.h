/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QByteArray>
#include <QtCrypto>

namespace QcaOtr {

/** DSA domain parameters used by OTRv3 identity keys. */
struct DsaDomain
{
    QCA::BigInteger p;
    QCA::BigInteger q;
    QCA::BigInteger g;
};

/** DSA private identity key. The private exponent must be treated as secret. */
struct DsaPrivateKey
{
    DsaDomain domain;
    QCA::BigInteger x;
};

/** DSA public identity key. */
struct DsaPublicKey
{
    DsaDomain domain;
    QCA::BigInteger y;
};

/** DSA signature pair. */
struct DsaSignature
{
    QCA::BigInteger r;
    QCA::BigInteger s;
};

/** Derives the public key corresponding to @p privateKey. */
DsaPublicKey dsaPublicKey(const DsaPrivateKey &privateKey);

/** Signs a precomputed digest with @p privateKey. */
bool dsaSignDigest(const DsaPrivateKey &privateKey, const QByteArray &digest, DsaSignature *signature);

/** Verifies a precomputed digest/signature pair. */
bool dsaVerifyDigest(const DsaPublicKey &publicKey, const QByteArray &digest, const DsaSignature &signature);

/** Returns SHA-1 of ordinary, non-secret input. */
QByteArray sha1(const QByteArray &data);

/** Returns SHA-256 of ordinary, non-secret input. */
QByteArray sha256(const QByteArray &data);

/** Returns SHA-1 while keeping the input/output in secure memory. */
QCA::SecureArray sha1Secure(const QCA::SecureArray &data);

/** Returns SHA-256 while keeping the input/output in secure memory. */
QCA::SecureArray sha256Secure(const QCA::SecureArray &data);

/** Computes HMAC-SHA1 with a key kept in secure memory. */
QByteArray hmacSha1(const QCA::SecureArray &key, const QByteArray &data);

/** Computes HMAC-SHA256 with a key kept in secure memory. */
QByteArray hmacSha256(const QCA::SecureArray &key, const QByteArray &data);

/**
 * Applies the OTRv3 AES-128-CTR transform.
 *
 * CTR encryption and decryption are the same operation; @p counter must contain
 * the protocol's 16-byte counter block.
 */
bool aes128Ctr(const QCA::SecureArray &key,
               const QByteArray &counter,
               const QByteArray &input,
               QByteArray *output);

} // namespace QcaOtr
