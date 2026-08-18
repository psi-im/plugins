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

#include "crypto.h"
#include "crypto_ossl.h"

#define CALL(method, ...) reinterpret_cast<psiomemo::CryptoImpl *>(user_data)->method(__VA_ARGS__)

namespace {
int random(uint8_t *data, size_t len, void *user_data) { return CALL(random, data, len); }
int hmac_sha256_init(void **hmac_context, const uint8_t *key, size_t key_len, void *user_data)
{
    return CALL(hmac_sha256_init, hmac_context, key, key_len);
}

int hmac_sha256_update(void *hmac_context, const uint8_t *data, size_t data_len, void *user_data)
{
    return CALL(hmac_sha256_update, hmac_context, data, data_len);
}

int hmac_sha256_final(void *hmac_context, signal_buffer **output, void *user_data)
{
    return CALL(hmac_sha256_final, hmac_context, output);
}

void hmac_sha256_cleanup(void *hmac_context, void *user_data) { return CALL(hmac_sha256_cleanup, hmac_context); }

int sha512_digest_init(void **digest_context, void *user_data) { return CALL(sha512_digest_init, digest_context); }

int sha512_digest_update(void *digest_context, const uint8_t *data, size_t data_len, void *user_data)
{
    return CALL(sha512_digest_update, digest_context, data, data_len);
}

int sha512_digest_final(void *digest_context, signal_buffer **output, void *user_data)
{
    return CALL(sha512_digest_final, digest_context, output);
}

void sha512_digest_cleanup(void *digest_context, void *user_data)
{
    return CALL(sha512_digest_cleanup, digest_context);
}

int encrypt(signal_buffer **output, int cipher, const uint8_t *key, size_t key_len, const uint8_t *iv, size_t iv_len,
            const uint8_t *plaintext, size_t plaintext_len, void *user_data)
{
    return CALL(encrypt, output, cipher, key, key_len, iv, iv_len, plaintext, plaintext_len);
}

int decrypt(signal_buffer **output, int cipher, const uint8_t *key, size_t key_len, const uint8_t *iv, size_t iv_len,
            const uint8_t *ciphertext, size_t ciphertext_len, void *user_data)
{
    return CALL(decrypt, output, cipher, key, key_len, iv, iv_len, ciphertext, ciphertext_len);
}
}

namespace psiomemo {
Crypto::Crypto() : impl(new CryptoOssl()) { }
Crypto::~Crypto() { }

bool Crypto::isSupported() { return impl->isSupported(); }

void Crypto::initCryptoProvider(signal_context *pContext)
{
    signal_crypto_provider crypto_provider = { /*.random_func =*/random,
                                               /*.hmac_sha256_init_func =*/hmac_sha256_init,
                                               /*.hmac_sha256_update_func =*/hmac_sha256_update,
                                               /*.hmac_sha256_final_func =*/hmac_sha256_final,
                                               /*.hmac_sha256_cleanup_func =*/hmac_sha256_cleanup,
                                               /*.sha512_digest_init_func =*/sha512_digest_init,
                                               /*.sha512_digest_update_func =*/sha512_digest_update,
                                               /*.sha512_digest_final_func =*/sha512_digest_final,
                                               /*.sha512_digest_cleanup_func =*/sha512_digest_cleanup,
                                               /*.encrypt_func =*/encrypt,
                                               /*.decrypt_func =*/decrypt,
                                               /*.user_data =*/impl.get() };

    signal_context_set_crypto_provider(pContext, &crypto_provider);
}

QByteArray Crypto::randomBytes(int length) { return impl->randomBytes(length); }

uint32_t Crypto::randomInt() { return impl->randomInt(); }

std::pair<QByteArray, QByteArray> Crypto::aes_gcm(Direction direction, const QByteArray &iv, const QByteArray &key,
                                                  const QByteArray &input, const QByteArray &tag)
{
    return impl->aes_gcm(direction, iv, key, input, tag);
}

QByteArray toQByteArray(const uint8_t *key, size_t key_len)
{
    return QByteArray(reinterpret_cast<const char *>(key), static_cast<int>(key_len));
}

}
