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

QString GpgProcess::findBin() const
{
	QString bin;

#ifdef Q_OS_WIN
	HKEY root;
	root = HKEY_CURRENT_USER;

	HKEY hkey = 0;
	const char *path = "Software\\GNU\\GnuPG";
	if(RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS)
	{
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, path, 0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS)
		{
			hkey = 0;
		}
	}

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
	}

#endif

	QFileInfo fi;
#ifdef Q_OS_MAC
	// mac-gpg
	fi.setFile("/usr/local/bin/gpg");
	if(fi.exists())
		bin = fi.filePath();
#endif

	// prefer bundled gpg
#ifdef Q_OS_WIN
	QString suffix=".exe";
#else
	QString suffix="";
#endif

	fi.setFile(QCoreApplication::applicationDirPath() + "/gpg" + suffix);
	if (fi.exists()) {
		bin = fi.filePath();
	}
	else {
		fi.setFile(QCoreApplication::applicationDirPath() + "/gpg2" + suffix);
		if (fi.exists()) {
			bin = fi.filePath();
		}
		else {
			bin = "gpg" + suffix;
		}
	}

	return bin;
}

QString GpgProcess::info()
{
	QStringList arguments;
	arguments << "--version"
			  << "--no-tty";
	start(arguments);
	waitForFinished();
	return QString("%1 %2\n%3").arg(_bin).arg(arguments.join(" ")).arg(QString::fromLocal8Bit(readAll()));
}
