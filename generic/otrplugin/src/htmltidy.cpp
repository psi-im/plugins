/*
 * htmltidy.cpp - Tidy html with libtidy
 *
 * Off-the-Record Messaging plugin for Psi
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

#include "htmltidy.h"

#include <QDebug>
#include <QDomDocument>
#include <QDomElement>
#include <QTextStream>

//-----------------------------------------------------------------------------

HtmlTidy::HtmlTidy(const QString &html) : m_tidyDoc(tidyCreate()), m_errorOutput(), m_output(), m_input(html)
{
    tidyOptSetBool(m_tidyDoc, TidyXmlOut, yes);
    tidyOptSetValue(m_tidyDoc, TidyCharEncoding, "utf8");
    tidyOptSetInt(m_tidyDoc, TidyNewline, TidyLF);
    tidyOptSetBool(m_tidyDoc, TidyQuoteNbsp, no);
    tidyOptSetBool(m_tidyDoc, TidyForceOutput, yes);

    tidySetErrorBuffer(m_tidyDoc, &m_errorOutput);

    tidyParseString(m_tidyDoc, m_input.toUtf8().data());
    tidyCleanAndRepair(m_tidyDoc);
}

//-----------------------------------------------------------------------------

HtmlTidy::~HtmlTidy()
{
    tidyRelease(m_tidyDoc);
    tidyBufFree(&m_errorOutput);
}

//-----------------------------------------------------------------------------

QString HtmlTidy::writeOutput()
{
    m_output.clear();
    TidyOutputSink sink;
#ifdef Q_OS_WIN
    sink.putByte = callPutByte;
#else
    sink.putByte = putByte;
#endif
    sink.sinkData = this;
    tidySaveSink(m_tidyDoc, &sink);

    return QString::fromUtf8(m_output);
}

//-----------------------------------------------------------------------------

QString HtmlTidy::output()
{
    QDomDocument document;
    QDomElement  body = output(document);
    QString      s;
    QTextStream  ts(&s);
    body.save(ts, 0);
    return s;
}

//-----------------------------------------------------------------------------

QDomElement HtmlTidy::output(QDomDocument &document)
{
    int     errorLine   = 0;
    int     errorColumn = 0;
    QString errorText;

    QString html = writeOutput();
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
    if (!document.setContent(html, true, &errorText, &errorLine, &errorColumn)) {
#else
    auto result = document.setContent(html, QDomDocument::ParseOption::UseNamespaceProcessing);
    errorLine = result.errorLine;
    errorColumn = result.errorColumn;
    errorText = result.errorMessage;
    if (!result) {
#endif
        qWarning() << "---- parsing error:\n"
                   << html << "\n----\n"
                   << errorText << " line:" << errorLine << " column:" << errorColumn;

        QDomElement domBody = document.createElement("body");
        domBody.appendChild(document.createTextNode(m_input));
        return domBody;
    }

    return document.documentElement().firstChildElement("body");
}

//-----------------------------------------------------------------------------

void HtmlTidy::putByte(void *sinkData, byte bt) { static_cast<HtmlTidy *>(sinkData)->putByte(bt); }

//-----------------------------------------------------------------------------
#ifdef Q_OS_WIN
void TIDY_CALL HtmlTidy::callPutByte(void *sinkData, byte bt)
{
    static_cast<HtmlTidy *>(sinkData)->putByte(sinkData, bt);
}

//-----------------------------------------------------------------------------
#endif
void HtmlTidy::putByte(byte bt) { m_output.append(bt); }

//-----------------------------------------------------------------------------
