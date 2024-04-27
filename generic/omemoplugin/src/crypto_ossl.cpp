/*
 * OMEMO Plugin for Psi
 * Copyright (C) 2018-2024 Vyacheslav Karpukhin, Psi IM team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "crypto_ossl.h"

#include <QRandomGenerator>
#include <QVector>

namespace {

}

namespace psiomemo {
CryptoOssl::CryptoOssl() noexcept
{
    OpenSSL_add_all_algorithms();
    if (RAND_status() == 0) {
        quint32 buf[32];
        auto    rg = QRandomGenerator::global();
        rg->fillRange(buf);
        RAND_seed(buf, sizeof(buf));
    }
#ifdef OSSL_300
    m_mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
#endif
}

CryptoOssl::~CryptoOssl() noexcept
{
#ifdef OSSL_300
    EVP_MAC_free(m_mac);
#endif
}

bool CryptoOssl::isSupported() const
{
#ifdef OSSL_300
    return m_mac != nullptr;
#else
    return true;
#endif
}

QByteArray CryptoOssl::randomBytes(int length)
{
    QVector<uint8_t> vector(length);
    while (RAND_bytes(vector.data(), length) != 1)
        ;
    return toQByteArray(vector.data(), static_cast<size_t>(vector.size()));
}

uint32_t CryptoOssl::randomInt()
{
    uint8_t data[4];
    while (RAND_bytes(data, 4) != 1)
        ;
    return ((uint32_t)data[0]) | ((uint32_t)data[1]) << 8 | ((uint32_t)data[2]) << 16 | ((uint32_t)data[3]) << 24;
}

int CryptoOssl::random(uint8_t *data, size_t len)
{
    while (RAND_bytes(data, static_cast<int>(len)) != 1)
        ;
    return SG_SUCCESS;
}

int CryptoOssl::hmac_sha256_init(void **context, const uint8_t *key, size_t key_len)
{
#ifdef OSSL_300
    auto ctx = EVP_MAC_CTX_new(m_mac);
#elif defined(OSSL_110)
    auto ctx = HMAC_CTX_new();
#else
    auto ctx = static_cast<HMAC_CTX *>(OPENSSL_malloc(sizeof(HMAC_CTX)));
#endif
    if (ctx == nullptr) {
        qDebug("omemo: failed to create mac context");
        return SG_ERR_INVAL;
    }

#ifdef OSSL_300
    OSSL_PARAM params[]
        = { OSSL_PARAM_construct_utf8_string("digest", (char *)"sha256", 0), OSSL_PARAM_construct_end() };
    if (!EVP_MAC_init(ctx, (const unsigned char *)key, static_cast<int>(key_len), params)) {
        qDebug("omemo: EVP_MAC_init failed");
        EVP_MAC_CTX_free(ctx);
        return SG_ERR_INVAL;
    }

#elif defined(OSSL_110)
    if (HMAC_Init_ex(ctx, key, static_cast<int>(key_len), EVP_sha256(), nullptr) != 1) {
        qDebug("omemo: HMAC_Init_ex failed");
        return SG_ERR_INVAL;
    }
#else
    HMAC_CTX_init(ctx);
#endif
    *context = ctx;
    return SG_SUCCESS;
}

int CryptoOssl::hmac_sha256_update(void *context, const uint8_t *data, size_t data_len)
{
#ifdef OSSL_300
    return EVP_MAC_update(static_cast<EVP_MAC_CTX *>(context), data, data_len) == 1 ? SG_SUCCESS : SG_ERR_INVAL;
#else
    return HMAC_Update(static_cast<HMAC_CTX *>(context), data, data_len) == 1 ? SG_SUCCESS : SG_ERR_INVAL;
#endif
}

int CryptoOssl::hmac_sha256_final(void *context, signal_buffer **output)
{

    int        length = EVP_MD_size(EVP_sha256());
    QByteArray vector(length, Qt::Uninitialized);
#ifdef OSSL_300
    size_t out_length;
    int    res = EVP_MAC_final(static_cast<EVP_MAC_CTX *>(context), reinterpret_cast<unsigned char *>(vector.data()),
                               &out_length, length);

#else
    int res = HMAC_Final(static_cast<HMAC_CTX *>(context), reinterpret_cast<unsigned char *>(vector.data()), nullptr);
#endif
    *output
        = signal_buffer_create(reinterpret_cast<unsigned char *>(vector.data()), static_cast<size_t>(vector.size()));
    return res == 1 ? SG_SUCCESS : SG_ERR_INVAL;
}

void CryptoOssl::hmac_sha256_cleanup(void *context)
{
    if (context != nullptr) {
#ifdef OSSL_300
        auto ctx = static_cast<EVP_MAC_CTX *>(context);
        EVP_MAC_CTX_free(ctx);
#elif defined(OSSL_110)
        auto ctx = static_cast<HMAC_CTX *>(context);
        HMAC_CTX_reset(ctx);
#else
        auto ctx = static_cast<HMAC_CTX *>(context);
        HMAC_CTX_cleanup(ctx);
        EVP_MD_CTX_cleanup(&ctx->i_ctx);
        EVP_MD_CTX_cleanup(&ctx->o_ctx);
        EVP_MD_CTX_cleanup(&ctx->md_ctx);
        OPENSSL_free(ctx);
#endif
    }
}

int CryptoOssl::sha512_digest_init(void **context)
{

    auto ctx = EVP_MD_CTX_create();
    if (ctx != nullptr) {
        *context = ctx;

        if (EVP_DigestInit(ctx, EVP_sha512()) == 1) {
            return SG_SUCCESS;
        }
    }
    return SG_ERR_INVAL;
}

int CryptoOssl::sha512_digest_update(void *context, const uint8_t *data, size_t data_len)
{

    return EVP_DigestUpdate(static_cast<EVP_MD_CTX *>(context), data, data_len) == 1 ? SG_SUCCESS : SG_ERR_INVAL;
}

int CryptoOssl::sha512_digest_final(void *context, signal_buffer **output)
{

    int              length = EVP_MD_size(EVP_sha512());
    QVector<uint8_t> vector(length);
    int              res = EVP_DigestFinal(static_cast<EVP_MD_CTX *>(context), vector.data(), nullptr);
    *output              = signal_buffer_create(vector.data(), static_cast<size_t>(vector.size()));
    return res == 1 ? SG_SUCCESS : SG_ERR_INVAL;
}

void CryptoOssl::sha512_digest_cleanup(void *context) { EVP_MD_CTX_destroy(static_cast<EVP_MD_CTX *>(context)); }

std::pair<QByteArray, QByteArray> CryptoOssl::aes(Crypto::Direction direction, const EVP_CIPHER *cipher, bool cbcMode,
                                                  const QByteArray &key, const QByteArray &iv,
                                                  const QByteArray &ciphertext, const QByteArray &inputTag)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);

    bool isEncode = direction == Crypto::Encode;
    auto initF    = isEncode ? EVP_EncryptInit_ex : EVP_DecryptInit_ex;
    auto updateF  = isEncode ? EVP_EncryptUpdate : EVP_DecryptUpdate;
    auto finalF   = isEncode ? EVP_EncryptFinal_ex : EVP_DecryptFinal_ex;

    initF(ctx, cipher, nullptr, nullptr, nullptr);
    if (!inputTag.isNull()) {
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.length(), nullptr);
    }
    initF(ctx, nullptr, nullptr, reinterpret_cast<const unsigned char *>(key.data()),
          reinterpret_cast<const unsigned char *>(iv.data()));
    EVP_CIPHER_CTX_set_padding(ctx, cbcMode ? 1 : 0);

    int              res;
    int              length;
    QVector<uint8_t> vector(ciphertext.length() + EVP_CIPHER_CTX_block_size(ctx));

    res = updateF(ctx, vector.data(), &length, reinterpret_cast<const unsigned char *>(ciphertext.data()),
                  ciphertext.length());

    if (res == 1) {
        if (!isEncode && !inputTag.isNull()) {
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, inputTag.length(), (void *)inputTag.data());
        }
        int finalLength;
        res = finalF(ctx, vector.data() + length, &finalLength);
        length += finalLength;
    }

    QByteArray cryptoText;
    QByteArray outTag;

    if (res == 1) {
        if (isEncode && !cbcMode) {
            QVector<uint8_t> tagVector(inputTag.length());
            EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, inputTag.length(), tagVector.data());
            outTag = toQByteArray(tagVector.data(), static_cast<size_t>(tagVector.size()));
        }
        cryptoText = toQByteArray(vector.data(), static_cast<size_t>(length));
    }

    EVP_CIPHER_CTX_cleanup(ctx);
    EVP_CIPHER_CTX_free(ctx);

    return qMakePair(cryptoText, outTag);
}

std::pair<QByteArray, QByteArray> CryptoOssl::aes_gcm(Crypto::Direction direction, const QByteArray &iv,
                                                      const QByteArray &key, const QByteArray &input,
                                                      const QByteArray &inputTag)
{
    const EVP_CIPHER *cipher;
    switch (key.size()) {
    case 16:
        cipher = EVP_aes_128_gcm();
        break;
    case 24:
        cipher = EVP_aes_192_gcm();
        break;
    case 32:
        cipher = EVP_aes_256_gcm();
        break;
    default:
        return qMakePair(QByteArray(), QByteArray());
    }
    return aes(direction, cipher, false, key, iv, input, inputTag);
}

int CryptoOssl::aes(Crypto::Direction direction, signal_buffer **output, int cipherMode, const uint8_t *key,
                    size_t key_len, const uint8_t *iv, size_t iv_len, const uint8_t *ciphertext, size_t ciphertext_len)
{
    const EVP_CIPHER *cipher;
    switch (key_len) {
    case 16:
        cipher = cipherMode == SG_CIPHER_AES_CBC_PKCS5 ? EVP_aes_128_cbc() : EVP_aes_128_ctr();
        break;
    case 24:
        cipher = cipherMode == SG_CIPHER_AES_CBC_PKCS5 ? EVP_aes_192_cbc() : EVP_aes_192_ctr();
        break;
    case 32:
        cipher = cipherMode == SG_CIPHER_AES_CBC_PKCS5 ? EVP_aes_256_cbc() : EVP_aes_256_ctr();
        break;
    default:
        return SG_ERR_INVAL;
    }

    QByteArray result = aes(direction, cipher, cipherMode == SG_CIPHER_AES_CBC_PKCS5, toQByteArray(key, key_len),
                            toQByteArray(iv, iv_len), toQByteArray(ciphertext, ciphertext_len), QByteArray())
                            .first;
    if (result.isNull()) {
        return SG_ERR_UNKNOWN;
    }
    *output
        = signal_buffer_create(reinterpret_cast<const uint8_t *>(result.data()), static_cast<size_t>(result.length()));
    return SG_SUCCESS;
}

int CryptoOssl::decrypt(signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len, const uint8_t *iv,
                        size_t iv_len, const uint8_t *ciphertext, size_t ciphertext_len)
{

    return aes(Crypto::Decode, output, cipherMode, key, key_len, iv, iv_len, ciphertext, ciphertext_len);
}

int CryptoOssl::encrypt(signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len, const uint8_t *iv,
                        size_t iv_len, const uint8_t *plaintext, size_t plaintext_len)
{

    return aes(Crypto::Encode, output, cipherMode, key, key_len, iv, iv_len, plaintext, plaintext_len);
}
}
