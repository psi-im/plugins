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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef PSIOMEMO_CRYPTO_H
#define PSIOMEMO_CRYPTO_H

#include <QtCrypto>
extern "C" {
#include <signal_protocol.h>
};

#define OMEMO_AES_128_KEY_LENGTH 16
#define OMEMO_AES_GCM_IV_LENGTH  16
#define OMEMO_AES_GCM_TAG_LENGTH 16

namespace psiomemo {
  class Crypto {
  public:
    static void initCryptoProvider(signal_context *pContext);

    static bool isSupported();

    static QPair<QByteArray, QCA::AuthTag>
    aes_gcm(QCA::Direction direction,
            const QCA::InitializationVector &iv,
            const QCA::SymmetricKey &key,
            const QByteArray &input,
            const QCA::AuthTag &tag = QCA::AuthTag(OMEMO_AES_GCM_TAG_LENGTH));
  };

  int random(uint8_t *data, size_t len, void *user_data);
  int hmac_sha256_init(void **context, const uint8_t *key, size_t key_len, void *user_data);
  int sha512_digest_init(void **context, void *user_data);
  int algo_update(void *context, const uint8_t *data, size_t data_len, void *user_data);
  int algo_final(void *context, signal_buffer **output, void *user_data);
  void algo_cleanup(void *context, void *user_data);
  int aes_decrypt(signal_buffer **output,
                  int cipherMode,
                  const uint8_t *key, size_t key_len,
                  const uint8_t *iv, size_t iv_len,
                  const uint8_t *ciphertext, size_t ciphertext_len,
                  void *user_data);
  int aes_encrypt(signal_buffer **output, int cipherMode,
                  const uint8_t *key, size_t key_len,
                  const uint8_t *iv, size_t iv_len,
                  const uint8_t *plaintext, size_t plaintext_len,
                  void *user_data);
  QByteArray toQByteArray(const uint8_t *key, size_t key_len);
}

#endif //PSIOMEMO_CRYPTO_H
