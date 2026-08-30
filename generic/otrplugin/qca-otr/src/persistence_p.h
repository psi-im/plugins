/*
 * SPDX-FileCopyrightText: 2026 Sergei Ilinykh
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <QByteArray>
#include <QFile>
#include <QFileDevice>
#include <QSaveFile>
#include <QString>

namespace QcaOtr::Persistence::Private {

inline void setError(QString *error, const QString &message)
{
    if (error && error->isEmpty())
        *error = message;
}

inline bool validTextField(const QByteArray &value)
{
    return !value.isEmpty() && !value.contains('\0') && !value.contains('\r') && !value.contains('\n');
}

inline bool validTabField(const QByteArray &value)
{
    return validTextField(value) && !value.contains('\t');
}

inline int hexValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

inline bool isHex(char c)
{
    return hexValue(c) >= 0;
}

inline bool readFile(const QString &path, QByteArray *data, QString *error)
{
    if (!data)
        return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(error, file.errorString());
        return false;
    }
    *data = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        setError(error, file.errorString());
        return false;
    }
    return true;
}

inline bool writeFileAtomically(const QString &path, const QByteArray &data, QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(error, file.errorString());
        return false;
    }
    if (file.write(data) != data.size()) {
        const QString message = file.errorString();
        file.cancelWriting();
        setError(error, message);
        return false;
    }
    if (!file.commit()) {
        setError(error, file.errorString());
        return false;
    }
    return true;
}

} // namespace QcaOtr::Persistence::Private
