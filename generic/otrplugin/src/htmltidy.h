/*
 * htmltidy.h - Tidy html with libtidy
 *
 * Off-the-Record Messaging plugin for Psi+
 * Copyright (C) 2007-2011  Timo Engel (timo-e@freenet.de)
 *                    2011  Florian Fieber
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

#ifndef HTMLTIDY_H_
#define HTMLTIDY_H_

#include <QByteArray>
#include <QString>

#include <string>

#ifdef LEGACY_TIDY
#ifdef Q_OS_WIN
#include <tidy/buffio.h>
#include <tidy/tidy.h>
#else
#include <buffio.h>
#include <tidy.h>
#endif
#else
#include <tidy.h>
#include <tidybuffio.h>
#endif

class QDomDocument;
class QDomElement;

class HtmlTidy {
public:
    HtmlTidy(const QString &html);
    ~HtmlTidy();
    QString     output();
    QDomElement output(QDomDocument &document);
    static void putByte(void *sinkData, byte bt);
#ifdef Q_OS_WIN
    static void TIDY_CALL callPutByte(void *sinkData, byte bt);
#endif

protected:
    void    putByte(byte bt);
    QString writeOutput();

private:
    TidyDoc    m_tidyDoc;
    TidyBuffer m_errorOutput;
    QByteArray m_output;
    QString    m_input;
};

#endif
