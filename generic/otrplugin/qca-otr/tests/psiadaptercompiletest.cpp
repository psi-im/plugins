/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#include "htmlnormalizer.h"
#include "otrmessaging.h"

#include <QByteArray>

int main()
{
    const QByteArray value = QByteArray::fromHex("00112233445566778899aabbccddeeff00112233");
    const psiotr::Fingerprint fingerprint(value, QStringLiteral("account"), QStringLiteral("peer"), {});
    if (!fingerprint.isValid() || fingerprint.value != value || fingerprint.fingerprintHuman.isEmpty())
        return 1;

    const psiotr::Fingerprint copy = fingerprint;
    if (!copy.isValid() || copy.value != value)
        return 2;

    HtmlNormalizer normalizer(QStringLiteral("<body><b>broken <i>rich text</body>"));
    const QString normalized = normalizer.output();
    if (!normalized.contains(QStringLiteral("broken")) || !normalized.contains(QStringLiteral("rich text")))
        return 3;

    return 0;
}
