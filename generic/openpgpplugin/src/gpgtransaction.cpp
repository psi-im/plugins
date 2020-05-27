/*
 * gpgtransaction.cpp
 * Copyright (C) 2020  Boris Pek <tehnick-8@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * You can also redistribute and/or modify this program under the
 * terms of the Psi License, specified in the accompanied COPYING
 * file, as published by the Psi Project; either dated January 1st,
 * 2005, or (at your option) any later version.
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

#include "gpgtransaction.h"
#include <QDir>
#include <QFile>

namespace OpenPgpPluginNamespace {

int GpgTransaction::m_idCounter = 0;

GpgTransaction::GpgTransaction(const Type type, const QString &keyID, QObject *parent) :
    GpgProcess(parent), m_type(type), m_startCounter(0)
{
    m_id = m_idCounter;
    ++m_idCounter;

    switch (type) {
    case Type::Sign: {
        m_arguments = QStringList { "--no-tty",      "--enable-special-filenames",
                                    "--armor",       "--always-trust",
                                    "--detach-sign", "--default-key",
                                    "0x" + keyID };
    } break;
    case Type::Verify: {
        m_tempFile  = QDir::tempPath() + "/psi.pgp.verify." + QString::number(m_id) + ".txt";
        m_arguments = QStringList {
            "--no-tty", "--enable-special-filenames", "--always-trust", "--status-fd=1", "--verify", "-", m_tempFile
        };
    } break;
    case Type::Encrypt: {
        m_arguments = QStringList { "--no-tty",  "--enable-special-filenames",
                                    "--armor",   "--always-trust",
                                    "--encrypt", "--recipient",
                                    "0x" + keyID };
    } break;
    case Type::Decrypt: {
        m_arguments = QStringList { "--no-tty",  "--enable-special-filenames",
                                    "--armor",   "--always-trust",
                                    "--decrypt", "--recipient",
                                    "0x" + keyID };
    } break;
    case Type::ListAllKeys: {
        m_arguments = QStringList { "--with-fingerprint", "--list-secret-keys", "--with-colons", "--fixed-list-mode" };
    } break;
    }

    connect(this, &QProcess::started, this, &GpgTransaction::processStarted);
    // TODO: update after stopping support of Ubuntu Xenial and WinXP:
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished()));
}

void GpgTransaction::start()
{
    // TODO: rewrite without usage of temporary file!
    if (m_type == Type::Verify) {
        QFile file(m_tempFile);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(m_data);
            file.close();
        }
    }

    ++m_startCounter;
    GpgProcess::start(m_arguments);
}

bool GpgTransaction::executeNow()
{
    disconnect(this, &QProcess::started, this, &GpgTransaction::processStarted);
    // TODO: update after stopping support of Ubuntu Xenial and WinXP:
    disconnect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished()));

    start();
    waitForStarted();
    processStarted();
    waitForFinished();
    processFinished();

    if (m_type == Type::ListAllKeys) {
        if (m_startCounter < 2) {
            waitForStarted();
            processStarted();
            waitForFinished();
            processFinished();
        }
    }

    return success();
}

int GpgTransaction::id() const { return m_id; }

void GpgTransaction::setGpgArguments(const QStringList &arguments)
{
    m_arguments.clear();
    m_arguments = arguments;
}

void GpgTransaction::setStdInString(const QString &str) { m_stdInString = str; }

void GpgTransaction::setOrigMessage(const QString &msg) { m_origMessage = msg; }

void GpgTransaction::setJid(const QString &jid) { m_jid = jid; }

void GpgTransaction::setData(const QByteArray &data) { m_data = data; }

QString GpgTransaction::stdInString() const { return m_stdInString; }

QString GpgTransaction::stdOutString() const { return m_stdOutString; }

QString GpgTransaction::stdErrString() const { return m_stdErrString; }

QString GpgTransaction::origMessage() const { return m_origMessage; }

QString GpgTransaction::jid() const { return m_jid; }

QByteArray GpgTransaction::data() const { return m_data; }

void GpgTransaction::processStarted()
{
    if (!m_stdInString.isEmpty()) {
        write(m_stdInString.toUtf8());
        closeWriteChannel();
    }
}

void GpgTransaction::processFinished()
{
    m_stdOutString.append(QString::fromUtf8(readAllStandardOutput()));
    m_stdErrString.append(QString::fromUtf8(readAllStandardError()));

    if (m_type == Type::ListAllKeys) {
        if (m_startCounter < 2) {
            setGpgArguments(
                QStringList { "--with-fingerprint", "--list-public-keys", "--with-colons", "--fixed-list-mode" });
            start();
            return;
        }
    }

#ifdef Q_OS_WIN
    m_stdOutString.replace("\r", "");
    m_stdErrString.replace("\r", "");
#endif

    if (m_type == Type::Verify) {
        QFile::remove(m_tempFile);
    }
    emit transactionFinished();
}

} // namespace OpenPgpPluginNamespace
