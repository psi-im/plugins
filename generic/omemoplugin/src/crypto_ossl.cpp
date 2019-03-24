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

#include <QVector>

#include "crypto.h"
extern "C" {
#include <openssl/hmac.h>
#include <openssl/rand.h>
}

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
#define OSSL_110
#endif

namespace psiomemo {
  void Crypto::doInit() {
    OpenSSL_add_all_algorithms();
    if (RAND_status() == 0) {
      qsrand(static_cast<uint>(time(nullptr)));
      char buf[128];
      for (char &n : buf) {
        n = static_cast<char>(qrand());
      }
      RAND_seed(buf, 128);
    }
  }

  bool Crypto::isSupported() {
    return true;
  }

  QByteArray Crypto::randomBytes(int length) {
    QVector<uint8_t> vector(length);
    while (RAND_bytes(vector.data(), length) != 1);
    return toQByteArray(vector.data(), static_cast<size_t>(vector.size()));
  }

  uint32_t Crypto::randomInt() {
    uint8_t data[4];
    while (RAND_bytes(data, 4) != 1);
    return ((uint32_t)data[0]) | ((uint32_t)data[1]) << 8 | ((uint32_t)data[2]) << 16 | ((uint32_t)data[3]) << 24;
  }

  int random(uint8_t *data, size_t len, void *user_data) {
    Q_UNUSED(user_data);
    while (RAND_bytes(data, static_cast<int>(len)) != 1);
    return SG_SUCCESS;
  }

  int hmac_sha256_init(void **context, const uint8_t *key, size_t key_len, void *user_data) {
    Q_UNUSED(user_data);
#ifdef OSSL_110
    auto ctx = HMAC_CTX_new();
#else
    auto ctx = static_cast<HMAC_CTX *>(OPENSSL_malloc(sizeof(HMAC_CTX)));
#endif
    if (ctx != nullptr) {
      *context = ctx;

#ifndef OSSL_110
      HMAC_CTX_init(ctx);
#endif
      if (HMAC_Init_ex(ctx, key, static_cast<int>(key_len), EVP_sha256(), nullptr) == 1) {
        return SG_SUCCESS;
      }
    }
    return SG_ERR_INVAL;
  }

  int hmac_sha256_update(void *context, const uint8_t *data, size_t data_len, void *user_data) {
    Q_UNUSED(user_data);
    return HMAC_Update(static_cast<HMAC_CTX *>(context), data, data_len) == 1 ? SG_SUCCESS : SG_ERR_INVAL;
  }

  int hmac_sha256_final(void *context, signal_buffer **output, void *user_data) {
    Q_UNUSED(user_data);
    int length = EVP_MD_size(EVP_sha256());
    QVector<uint8_t> vector(length);
    int res = HMAC_Final(static_cast<HMAC_CTX *>(context), vector.data(), nullptr);
    *output = signal_buffer_create(vector.data(), static_cast<size_t>(vector.size()));
    return res == 1 ? SG_SUCCESS : SG_ERR_INVAL;
  }

  void hmac_sha256_cleanup(void *context, void *user_data) {
    Q_UNUSED(user_data);
    auto ctx = static_cast<HMAC_CTX *>(context);
    if (ctx != nullptr) {
#ifdef OSSL_110
      HMAC_CTX_reset(ctx);
#else
      HMAC_CTX_cleanup(ctx);
      EVP_MD_CTX_cleanup(&ctx->i_ctx);
      EVP_MD_CTX_cleanup(&ctx->o_ctx);
      EVP_MD_CTX_cleanup(&ctx->md_ctx);
      OPENSSL_free(ctx);
#endif
    }
  }

  int sha512_digest_init(void **context, void *user_data) {
    Q_UNUSED(user_data);
    auto ctx = EVP_MD_CTX_create();
    if (ctx != nullptr) {
      *context = ctx;

      if (EVP_DigestInit(ctx, EVP_sha512()) == 1) {
        return SG_SUCCESS;
      }
    }
    return SG_ERR_INVAL;
  }

  int sha512_digest_update(void *context, const uint8_t *data, size_t data_len, void *user_data) {
    Q_UNUSED(user_data);
    return EVP_DigestUpdate(static_cast<EVP_MD_CTX *>(context), data, data_len) == 1 ? SG_SUCCESS : SG_ERR_INVAL;
  }

  int sha512_digest_final(void *context, signal_buffer **output, void *user_data) {
    Q_UNUSED(user_data);
    int length = EVP_MD_size(EVP_sha512());
    QVector<uint8_t> vector(length);
    int res = EVP_DigestFinal(static_cast<EVP_MD_CTX *>(context), vector.data(), nullptr);
    *output = signal_buffer_create(vector.data(), static_cast<size_t>(vector.size()));
    return res == 1 ? SG_SUCCESS : SG_ERR_INVAL;
  }

  void sha512_digest_cleanup(void *context, void *user_data) {
    Q_UNUSED(user_data);
    EVP_MD_CTX_destroy(static_cast<EVP_MD_CTX *>(context));
  }

  QPair<QByteArray, QByteArray> aes(Crypto::Direction direction, const EVP_CIPHER *cipher, bool cbcMode, const QByteArray &key,
                                    const QByteArray &iv, const QByteArray &ciphertext, const QByteArray &inputTag) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);

    bool isEncode = direction == Crypto::Encode;
    auto initF = isEncode ? EVP_EncryptInit_ex : EVP_DecryptInit_ex;
    auto updateF = isEncode ? EVP_EncryptUpdate : EVP_DecryptUpdate;
    auto finalF = isEncode ? EVP_EncryptFinal_ex : EVP_DecryptFinal_ex;

    initF(ctx, cipher, nullptr, nullptr, nullptr);
    if (!inputTag.isNull()) {
      EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.length(), nullptr);
    }
    initF(ctx, nullptr, nullptr, reinterpret_cast<const unsigned char *>(key.data()),
          reinterpret_cast<const unsigned char *>(iv.data()));
    EVP_CIPHER_CTX_set_padding(ctx, cbcMode ? 1 : 0);

    int res;
    int length;
    QVector<uint8_t> vector(ciphertext.length() + EVP_CIPHER_CTX_block_size(ctx));


    res = updateF(ctx, vector.data(), &length, reinterpret_cast<const unsigned char *>(ciphertext.data()), ciphertext.length());

    if (res == 1) {
      if (!isEncode && !inputTag.isNull()) {
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, inputTag.length(), (void *) inputTag.data());
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


  QPair<QByteArray, QByteArray> Crypto::aes_gcm(Crypto::Direction direction,
                                                const QByteArray &iv,
                                                const QByteArray &key,
                                                const QByteArray &input,
                                                const QByteArray &inputTag) {
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

  int aes(Crypto::Direction direction, signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len, const uint8_t *iv,
          size_t iv_len, const uint8_t *ciphertext, size_t ciphertext_len) {
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

    QByteArray result = aes(direction,
                            cipher,
                            cipherMode == SG_CIPHER_AES_CBC_PKCS5,
                            toQByteArray(key, key_len),
                            toQByteArray(iv, iv_len),
                            toQByteArray(ciphertext, ciphertext_len),
                            QByteArray()).first;
    if (result.isNull()) {
      return SG_ERR_UNKNOWN;
    }
    *output = signal_buffer_create(reinterpret_cast<const uint8_t *>(result.data()),
                                   static_cast<size_t>(result.length()));
    return SG_SUCCESS;
  }

  int aes_decrypt(signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len, const uint8_t *iv,
                  size_t iv_len, const uint8_t *ciphertext, size_t ciphertext_len, void *user_data) {
    Q_UNUSED(user_data);
    return aes(Crypto::Decode, output, cipherMode, key, key_len, iv, iv_len, ciphertext, ciphertext_len);
  }

  int aes_encrypt(signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len, const uint8_t *iv,
                  size_t iv_len, const uint8_t *plaintext, size_t plaintext_len, void *user_data) {
    Q_UNUSED(user_data);
    return aes(Crypto::Encode, output, cipherMode, key, key_len, iv, iv_len, plaintext, plaintext_len);
  }
}
