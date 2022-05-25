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

#include "crypto.h"
#include <QtCrypto>
#include <qdebug.h>

using namespace QCA;

namespace psiomemo {
void Crypto::doInit() { }

bool Crypto::isSupported()
{
    QStringList requiredQcaFeatures;
    requiredQcaFeatures << "hmac(sha256)"
                        << "sha512" << Cipher::withAlgorithms("aes128", Cipher::GCM, Cipher::DefaultPadding)
                        << Cipher::withAlgorithms("aes128", Cipher::CBC, Cipher::DefaultPadding)
                        << Cipher::withAlgorithms("aes192", Cipher::CBC, Cipher::DefaultPadding)
                        << Cipher::withAlgorithms("aes256", Cipher::CBC, Cipher::DefaultPadding);
    if (!QCA::isSupported(requiredQcaFeatures)) {
        qWarning("Required QCA features are not supported:");
        qWarning() << requiredQcaFeatures;
        return false;
    }
    return true;
}

QString getCipherName(int keyLength)
{
    switch (keyLength) {
    case 16:
        return "aes128";
    case 24:
        return "aes192";
    case 32:
        return "aes256";
    default:
        return QString();
    }
}

QPair<QByteArray, QByteArray> Crypto::aes_gcm(Crypto::Direction direction, const QByteArray &iv, const QByteArray &key,
                                              const QByteArray &input, const QByteArray &tag)
{
    Cipher     cipher(getCipherName(key.size()), Cipher::GCM, Cipher::NoPadding,
                  direction == Encode ? QCA::Encode : QCA::Decode, key, iv, AuthTag(tag));
    QByteArray cryptoText = cipher.process(input).toByteArray();
    return qMakePair(cryptoText, cipher.tag().toByteArray());
}

QByteArray Crypto::randomBytes(int length) { return Random::randomArray(length).toByteArray(); }

uint32_t Crypto::randomInt() { return static_cast<uint32_t>(Random::randomInt()); }

int random(uint8_t *data, size_t len, void *user_data)
{
    Q_UNUSED(user_data);
    SecureArray array = Random::randomArray(static_cast<int>(len));
    memcpy(data, array.data(), len);
    return SG_SUCCESS;
}

int hmac_sha256_init(void **context, const uint8_t *key, size_t key_len, void *user_data)
{
    Q_UNUSED(user_data);
    *context = new MessageAuthenticationCode("hmac(sha256)", SymmetricKey(toQByteArray(key, key_len)));
    return SG_SUCCESS;
}

int sha512_digest_init(void **context, void *user_data)
{
    Q_UNUSED(user_data);
    *context = new Hash("sha512");
    return SG_SUCCESS;
}

int algo_update(void *context, const uint8_t *data, size_t data_len, void *user_data)
{
    Q_UNUSED(user_data);
    auto mac = static_cast<BufferedComputation *>(context);
    mac->update(MemoryRegion(toQByteArray(data, data_len)));
    return SG_SUCCESS;
}

int hmac_sha256_update(void *context, const uint8_t *data, size_t data_len, void *user_data)
{
    return algo_update(context, data, data_len, user_data);
}

int sha512_digest_update(void *context, const uint8_t *data, size_t data_len, void *user_data)
{
    return algo_update(context, data, data_len, user_data);
}

int algo_final(void *context, signal_buffer **output, void *user_data)
{
    Q_UNUSED(user_data);
    auto         mac    = static_cast<BufferedComputation *>(context);
    MemoryRegion result = mac->final();
    *output             = signal_buffer_create(reinterpret_cast<const uint8_t *>(result.constData()),
                                               static_cast<size_t>(result.size()));
    return SG_SUCCESS;
}

int hmac_sha256_final(void *context, signal_buffer **output, void *user_data)
{
    return algo_final(context, output, user_data);
}

int sha512_digest_final(void *context, signal_buffer **output, void *user_data)
{
    return algo_final(context, output, user_data);
}

void algo_cleanup(void *context, void *user_data)
{
    Q_UNUSED(user_data);
    auto mac = static_cast<BufferedComputation *>(context);
    delete mac;
}

void hmac_sha256_cleanup(void *context, void *user_data) { algo_cleanup(context, user_data); }

void sha512_digest_cleanup(void *context, void *user_data) { algo_cleanup(context, user_data); }

int aes(Crypto::Direction direction, signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len,
        const uint8_t *iv, size_t iv_len, const uint8_t *ciphertext, size_t ciphertext_len)
{
    QString cipherName = getCipherName(static_cast<int>(key_len));
    if (cipherName.isNull()) {
        return SG_ERR_UNKNOWN;
    }

    Cipher::Mode mode;
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

    Cipher       cipher(cipherName, mode, Cipher::DefaultPadding, direction == Crypto::Encode ? Encode : Decode,
                        toQByteArray(key, key_len), toQByteArray(iv, iv_len));
    MemoryRegion result = cipher.process(toQByteArray(ciphertext, ciphertext_len));
    if (!cipher.ok()) {
        return SG_ERR_UNKNOWN;
    }
    *output = signal_buffer_create(reinterpret_cast<const uint8_t *>(result.constData()),
                                   static_cast<size_t>(result.size()));

    return SG_SUCCESS;
}

int aes_decrypt(signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len, const uint8_t *iv,
                size_t iv_len, const uint8_t *ciphertext, size_t ciphertext_len, void *user_data)
{
    Q_UNUSED(user_data);
    return aes(Crypto::Decode, output, cipherMode, key, key_len, iv, iv_len, ciphertext, ciphertext_len);
}

int aes_encrypt(signal_buffer **output, int cipherMode, const uint8_t *key, size_t key_len, const uint8_t *iv,
                size_t iv_len, const uint8_t *plaintext, size_t plaintext_len, void *user_data)
{
    Q_UNUSED(user_data);
    return aes(Crypto::Encode, output, cipherMode, key, key_len, iv, iv_len, plaintext, plaintext_len);
}
}
