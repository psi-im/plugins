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
#define OMEMO_AES_GCM_IV_LENGTH  16
#define OMEMO_AES_GCM_TAG_LENGTH 16

namespace psiomemo {
  class Crypto {
  public:
    enum Direction
    {
      Encode,
      Decode
    };

    static void initCryptoProvider(signal_context *pContext);

    static bool isSupported();

    static QPair<QByteArray, QByteArray>
    aes_gcm(Direction direction,
            const QByteArray &iv,
            const QByteArray &key,
            const QByteArray &input,
            const QByteArray &tag = QByteArray(OMEMO_AES_GCM_TAG_LENGTH, Qt::Uninitialized));

    static QByteArray randomBytes(int length);
    static uint32_t randomInt();

  private:
    static void doInit();
  };

  int random(uint8_t *data, size_t len, void *user_data);
  int hmac_sha256_init(void **context, const uint8_t *key, size_t key_len, void *user_data);
  int hmac_sha256_update(void *context, const uint8_t *data, size_t data_len, void *user_data);
  int hmac_sha256_final(void *context, signal_buffer **output, void *user_data);
  void hmac_sha256_cleanup(void *context, void *user_data);
  int sha512_digest_init(void **context, void *user_data);
  int sha512_digest_update(void *context, const uint8_t *data, size_t data_len, void *user_data);
  int sha512_digest_final(void *context, signal_buffer **output, void *user_data);
  void sha512_digest_cleanup(void *context, void *user_data);
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
