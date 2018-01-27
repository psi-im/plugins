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

#include <qdebug.h>
#include "crypto.h"

using namespace QCA;

namespace psiomemo {
  void Crypto::initCryptoProvider(signal_context *ctx) {
    signal_crypto_provider crypto_provider = {
        .random_func = random,
        .hmac_sha256_init_func = hmac_sha256_init,
        .hmac_sha256_update_func = algo_update,
        .hmac_sha256_final_func = algo_final,
        .hmac_sha256_cleanup_func = algo_cleanup,
        .sha512_digest_init_func = sha512_digest_init,
        .sha512_digest_update_func = algo_update,
        .sha512_digest_final_func = algo_final,
        .sha512_digest_cleanup_func = algo_cleanup,
        .encrypt_func = aes_encrypt,
        .decrypt_func = aes_decrypt,
        .user_data = nullptr
    };

    signal_context_set_crypto_provider(ctx, &crypto_provider);
  }

  bool Crypto::isSupported() {
    QStringList requiredQcaFeatures({"hmac(sha256)", "sha512",
                                     Cipher::withAlgorithms("aes128", Cipher::GCM, Cipher::DefaultPadding),
                                     Cipher::withAlgorithms("aes128", Cipher::CBC, Cipher::DefaultPadding),
                                     Cipher::withAlgorithms("aes192", Cipher::CBC, Cipher::DefaultPadding),
                                     Cipher::withAlgorithms("aes256", Cipher::CBC, Cipher::DefaultPadding)});
    if (!QCA::isSupported(requiredQcaFeatures)) {
      qWarning("Required QCA features are not supported:");
      qWarning() << requiredQcaFeatures;
      return false;
    }
    return true;
  }

  QPair<QByteArray, QCA::AuthTag> Crypto::aes_gcm(QCA::Direction direction,
                                                  const InitializationVector &iv,
                                                  const SymmetricKey &key,
                                                  const QByteArray &input,
                                                  const AuthTag &tag) {
    Cipher cipher("aes128", Cipher::GCM, Cipher::NoPadding, direction, key, iv, tag);
    QByteArray cryptoText = cipher.process(input).toByteArray();
    return qMakePair(cryptoText, cipher.tag());
  }

  int random(uint8_t *data, size_t len, __unused void *user_data) {
    SecureArray array = Random::randomArray(static_cast<int>(len));
    memcpy(data, array.data(), len);
    return SG_SUCCESS;
  }

  int hmac_sha256_init(void **context, const uint8_t *key, size_t key_len, __unused void *user_data) {
    *context = new MessageAuthenticationCode("hmac(sha256)", SymmetricKey(toQByteArray(key, key_len)));
    return SG_SUCCESS;
  }

  int sha512_digest_init(void **context, __unused void *user_data) {
    *context = new Hash("sha512");
    return SG_SUCCESS;
  }

  int algo_update(void *context, const uint8_t *data, size_t data_len, __unused void *user_data) {
    auto mac = static_cast<BufferedComputation *>(context);
    mac->update(MemoryRegion(toQByteArray(data, data_len)));
    return SG_SUCCESS;
  }

  int algo_final(void *context, signal_buffer **output, __unused void *user_data) {
    auto mac = static_cast<BufferedComputation *>(context);
    MemoryRegion result = mac->final();
    *output = signal_buffer_create(reinterpret_cast<const uint8_t *>(result.constData()),
                                   static_cast<size_t>(result.size()));
    return SG_SUCCESS;
  }

  void algo_cleanup(void *context, __unused void *user_data) {
    auto mac = static_cast<BufferedComputation *>(context);
    delete mac;
  }

  int aes(Direction direction, signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len,
          const uint8_t *iv, size_t iv_len, const uint8_t *ciphertext, size_t ciphertext_len) {
    const char *cipherName;
    Cipher::Mode mode;

    switch (key_len) {
      case 16:
        cipherName = "aes128";
        break;
      case 24:
        cipherName = "aes192";
        break;
      case 32:
        cipherName = "aes256";
        break;
      default:
        return SG_ERR_UNKNOWN;
    }

    switch (cipherMode) {
      case SG_CIPHER_AES_CBC_PKCS5:
        mode = Cipher::CBC;
        break;
      case SG_CIPHER_AES_CTR_NOPADDING:
        mode = Cipher::CTR;
        break;
      default:
        return SG_ERR_UNKNOWN;
    }

    Cipher cipher(cipherName, mode, Cipher::DefaultPadding, direction, toQByteArray(key, key_len), toQByteArray(iv, iv_len));
    MemoryRegion result = cipher.process(toQByteArray(ciphertext, ciphertext_len));
    if (!cipher.ok()) {
      return SG_ERR_UNKNOWN;
    }
    *output = signal_buffer_create(reinterpret_cast<const uint8_t *>(result.constData()),
                                   static_cast<size_t>(result.size()));

    return SG_SUCCESS;
  }

  int aes_decrypt(signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len, const uint8_t *iv,
                  size_t iv_len, const uint8_t *ciphertext, size_t ciphertext_len, __unused void *user_data) {
    return aes(Decode, output, cipherMode, key, key_len, iv, iv_len, ciphertext, ciphertext_len);
  }

  int aes_encrypt(signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len, const uint8_t *iv,
                  size_t iv_len, const uint8_t *plaintext, size_t plaintext_len, __unused void *user_data) {
    return aes(Encode, output, cipherMode, key, key_len, iv, iv_len, plaintext, plaintext_len);
  }

  QByteArray toQByteArray(const uint8_t *key, size_t key_len) {
    return QByteArray(reinterpret_cast<const char *>(key), static_cast<int>(key_len));
  }
}