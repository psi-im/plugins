/*
 * HtmlTidy.hpp - tidy html with libtidy
 * Copyright (C) 2010  Timo Engel (timo-e@freenet.de)
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef HTMLTIDY_HPP_
#define HTMLTIDY_HPP_

#include <string>
#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include <QtXml>

class HtmlTidy
{
public:
    HtmlTidy(const QString& html);
    ~HtmlTidy();
    QString output();
    QDomElement output(QDomDocument& document);
    static void putByte(void* sinkData, byte bt);
    

protected:
    void putByte(byte bt);
    QString writeOutput();
    
    
private:
    TidyDoc     m_tidyDoc;
    TidyBuffer  m_errorOutput;
    QByteArray  m_output;
    QString     m_input;
};

#endif
