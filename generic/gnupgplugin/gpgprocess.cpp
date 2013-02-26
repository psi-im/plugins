/*
 * gpgprocess.cpp - QProcess wrapper makes it easy to handle gpg
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QFileInfo>
#include "gpgprocess.h"
#include <QCoreApplication>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

GpgProcess::GpgProcess(QObject *parent)
	: QProcess(parent)
	, _bin("")
{
	_bin = findBin();
}

inline bool checkBin(const QString &bin)
{
	QFileInfo fi(bin);
	return fi.exists();
}

#ifdef Q_OS_WIN
static bool checkReg(QString &bin)
{
	HKEY root;
	root = HKEY_CURRENT_USER;

	HKEY hkey = 0;
	const char *path = "Software\\GNU\\GnuPG";
	const char *path2 = "Software\\Wow6432Node\\GNU\\GnuPG";
	if(RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS) {}
	else if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, path, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS) {}
	else if(RegOpenKeyExA(HKEY_CURRENT_USER, path2, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS) {}
	else if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, path2, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS) {}
	else hkey = 0;

	if (hkey)
	{
		char szValue[256];
		DWORD dwLen = 256;
		if(RegQueryValueExA(hkey, "gpgProgram", NULL, NULL, (LPBYTE)szValue, &dwLen) != ERROR_SUCCESS)
		{
			QString s = QString::fromLatin1(szValue);

			if(!s.isEmpty())
				bin = s;
		}

		RegCloseKey(hkey);
		if (!bin.isEmpty() && checkBin(bin)) {
			return true;
		}
	}
	return false;
}
#endif

QString GpgProcess::findBin() const
{
#ifdef Q_OS_WIN
	QString suffix=".exe";
#else
	QString suffix="";
#endif

	QString bin;
	// prefer bundled gpg
	if (checkBin(bin = QCoreApplication::applicationDirPath() + "/gpg" + suffix)) {}
	else if (checkBin(bin = QCoreApplication::applicationDirPath() + "/gpg2" + suffix)) {}
#ifdef Q_OS_WIN
	else if (checkReg(bin)) {}
#endif
#ifdef Q_OS_MAC
	// mac-gpg
	else if (checkBin(bin = "/usr/local/bin/gpg"));
#endif
	else bin = "gpg";

	return bin;
}

QString GpgProcess::info()
{
	QStringList arguments;
	arguments << "--version"
			  << "--no-tty";
	start(arguments);
	waitForFinished();

	QString message;
	if (error() == FailedToStart) {
		message = trUtf8("Can't start ") + _bin;
	}
	else {
		message = QString("%1 %2\n%3").arg(_bin).arg(arguments.join(" ")).arg(QString::fromLocal8Bit(readAll()));
	}

	return message;
}
