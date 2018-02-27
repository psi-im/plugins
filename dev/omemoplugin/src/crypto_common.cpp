#include "crypto.h"

namespace psiomemo {
  void Crypto::initCryptoProvider(signal_context *ctx) {
    signal_crypto_provider crypto_provider = {
        /*.random_func =*/ random,
        /*.hmac_sha256_init_func =*/ hmac_sha256_init,
        /*.hmac_sha256_update_func =*/ algo_update,
        /*.hmac_sha256_final_func =*/ algo_final,
        /*.hmac_sha256_cleanup_func =*/ algo_cleanup,
        /*.sha512_digest_init_func =*/ sha512_digest_init,
        /*.sha512_digest_update_func =*/ algo_update,
        /*.sha512_digest_final_func =*/ algo_final,
        /*.sha512_digest_cleanup_func =*/ algo_cleanup,
        /*.encrypt_func =*/ aes_encrypt,
        /*.decrypt_func =*/ aes_decrypt,
        /*.user_data =*/ nullptr
    };

    signal_context_set_crypto_provider(ctx, &crypto_provider);
  }

  QByteArray toQByteArray(const uint8_t *key, size_t key_len) {
    return QByteArray(reinterpret_cast<const char *>(key), static_cast<int>(key_len));
  }
}