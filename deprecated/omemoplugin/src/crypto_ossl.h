/*
 * OMEMO Plugin for Psi
 * Copyright (C) 2018-2024 Psi IM team
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

#pragma once

#include "crypto.h"

extern "C" {
#include <openssl/hmac.h>
#include <openssl/rand.h>
}

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
#define OSSL_110
#endif
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#define OSSL_300
#endif

namespace psiomemo {

class CryptoOssl : public CryptoImpl {
public:
    CryptoOssl() noexcept;
    ~CryptoOssl() noexcept;

    bool isSupported() const override;

    // signal context
    int  random(uint8_t *data, size_t len) override;
    int  hmac_sha256_init(void **context, const uint8_t *key, size_t key_len) override;
    int  hmac_sha256_update(void *context, const uint8_t *data, size_t data_len) override;
    int  hmac_sha256_final(void *context, signal_buffer **output) override;
    void hmac_sha256_cleanup(void *context) override;
    int  sha512_digest_init(void **context) override;
    int  sha512_digest_update(void *context, const uint8_t *data, size_t data_len) override;
    int  sha512_digest_final(void *context, signal_buffer **output) override;
    void sha512_digest_cleanup(void *context) override;
    int  decrypt(signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len, const uint8_t *iv,
                 size_t iv_len, const uint8_t *ciphertext, size_t ciphertext_len) override;
    int  encrypt(signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len, const uint8_t *iv,
                 size_t iv_len, const uint8_t *plaintext, size_t plaintext_len) override;

    // aux methods
    std::pair<QByteArray, QByteArray>
    aes_gcm(Crypto::Direction direction, const QByteArray &iv, const QByteArray &key, const QByteArray &input,
            const QByteArray &tag = QByteArray(OMEMO_AES_GCM_TAG_LENGTH, Qt::Uninitialized)) override;

    QByteArray randomBytes(int length) override;
    uint32_t   randomInt() override;

private:
    std::pair<QByteArray, QByteArray> aes(Crypto::Direction direction, const EVP_CIPHER *cipher, bool cbcMode,
                                          const QByteArray &key, const QByteArray &iv, const QByteArray &ciphertext,
                                          const QByteArray &inputTag);
    int aes(Crypto::Direction direction, signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len,
            const uint8_t *iv, size_t iv_len, const uint8_t *ciphertext, size_t ciphertext_len);

#ifdef OSSL_300
    EVP_MAC *m_mac;
#endif
};

}
