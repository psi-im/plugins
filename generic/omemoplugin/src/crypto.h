/*
 * OMEMO Plugin for Psi
 * Copyright (C) 2018 Vyacheslav Karpukhin
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

#ifndef PSIOMEMO_CRYPTO_H
#define PSIOMEMO_CRYPTO_H

#include <QByteArray>
#include <QPair>

extern "C" {
#include <signal_protocol.h>
};

#define OMEMO_AES_128_KEY_LENGTH 16
#define OMEMO_AES_GCM_IV_LENGTH 12
#define OMEMO_AES_GCM_TAG_LENGTH 16

namespace psiomemo {

class CryptoImpl;

// this is basically a singleton created as soon as the plugin is enabled
class Crypto {
public:
    enum Direction { Encode, Decode };

    Crypto();
    ~Crypto();
    bool isSupported();

    void initCryptoProvider(signal_context *pContext);

    QByteArray randomBytes(int length);
    uint32_t   randomInt();
    std::pair<QByteArray, QByteArray>
    aes_gcm(Crypto::Direction direction, const QByteArray &iv, const QByteArray &key, const QByteArray &input,
            const QByteArray &tag = QByteArray(OMEMO_AES_GCM_TAG_LENGTH, Qt::Uninitialized));

private:
    static void doInit();

    std::unique_ptr<CryptoImpl> impl;
};

class CryptoImpl {
public:
    virtual bool isSupported() const = 0;

    // required for signal
    virtual int  random(uint8_t *data, size_t len)                                         = 0;
    virtual int  hmac_sha256_init(void **context, const uint8_t *key, size_t key_len)      = 0;
    virtual int  hmac_sha256_update(void *context, const uint8_t *data, size_t data_len)   = 0;
    virtual int  hmac_sha256_final(void *context, signal_buffer **output)                  = 0;
    virtual void hmac_sha256_cleanup(void *context)                                        = 0;
    virtual int  sha512_digest_init(void **context)                                        = 0;
    virtual int  sha512_digest_update(void *context, const uint8_t *data, size_t data_len) = 0;
    virtual int  sha512_digest_final(void *context, signal_buffer **output)                = 0;
    virtual void sha512_digest_cleanup(void *context)                                      = 0;
    virtual int  decrypt(signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len, const uint8_t *iv,
                         size_t iv_len, const uint8_t *ciphertext, size_t ciphertext_len)
        = 0;
    virtual int encrypt(signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len, const uint8_t *iv,
                        size_t iv_len, const uint8_t *plaintext, size_t plaintext_len)
        = 0;

    // aux methods
    virtual std::pair<QByteArray, QByteArray>
    aes_gcm(Crypto::Direction direction, const QByteArray &iv, const QByteArray &key, const QByteArray &input,
            const QByteArray &tag = QByteArray(OMEMO_AES_GCM_TAG_LENGTH, Qt::Uninitialized))
        = 0;

    virtual QByteArray randomBytes(int length) = 0;
    virtual uint32_t   randomInt()             = 0;
};

QByteArray toQByteArray(const uint8_t *key, size_t key_len);
}

#endif // PSIOMEMO_CRYPTO_H
