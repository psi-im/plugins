/*
 * optionsparser.cpp - plugin
 * Copyright (C) 2010  Khryukin Evgeny
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "optionsparser.h"
#include <QStringList>
#include <QSize>
#include <QKeySequence>
#include <QRect>

OptionsParser::OptionsParser(QObject *parent)
        :QObject(parent)
{
}


OptionsParser* OptionsParser::instance()
{
    if(!instance_)
        instance_ = new OptionsParser();

    return instance_;
}

//stolen from varianttree.cpp

void OptionsParser::variantToElement(const QVariant& var, QDomElement& e)
{
        QString type = var.typeName();
        if (type == "QVariantList") {
                foreach(QVariant v, var.toList()) {
                        QDomElement item_element = e.ownerDocument().createElement("item");
                        variantToElement(v,item_element);
                        e.appendChild(item_element);
                }
        }
        else if (type == "QStringList") {
                foreach(QString s, var.toStringList()) {
                        QDomElement item_element = e.ownerDocument().createElement("item");
                        QDomText text = e.ownerDocument().createTextNode(s);
                        item_element.appendChild(text);
                        e.appendChild(item_element);
                }
        }
        else if (type == "QSize") {
                QSize size = var.toSize();
                QDomElement width_element = e.ownerDocument().createElement("width");
                width_element.appendChild(e.ownerDocument().createTextNode(QString::number(size.width())));
                e.appendChild(width_element);
                QDomElement height_element = e.ownerDocument().createElement("height");
                height_element.appendChild(e.ownerDocument().createTextNode(QString::number(size.height())));
                e.appendChild(height_element);
        }
        else if (type == "QRect") {
                QRect rect = var.toRect();
                QDomElement x_element = e.ownerDocument().createElement("x");
                x_element.appendChild(e.ownerDocument().createTextNode(QString::number(rect.x())));
                e.appendChild(x_element);
                QDomElement y_element = e.ownerDocument().createElement("y");
                y_element.appendChild(e.ownerDocument().createTextNode(QString::number(rect.y())));
                e.appendChild(y_element);
                QDomElement width_element = e.ownerDocument().createElement("width");
                width_element.appendChild(e.ownerDocument().createTextNode(QString::number(rect.width())));
                e.appendChild(width_element);
                QDomElement height_element = e.ownerDocument().createElement("height");
                height_element.appendChild(e.ownerDocument().createTextNode(QString::number(rect.height())));
                e.appendChild(height_element);
        }
        else if (type == "QByteArray") {
                QDomText text = e.ownerDocument().createTextNode(Base64::encode(var.toByteArray()));
                e.appendChild(text);
        }
        else if (type == "QKeySequence") {
                QKeySequence k = var.value<QKeySequence>();
                QDomText text = e.ownerDocument().createTextNode(k.toString());
                e.appendChild(text);
        }
        else {
                QDomText text = e.ownerDocument().createTextNode(var.toString());
                e.appendChild(text);
        }
        e.setAttribute("type",type);
}

QVariant OptionsParser::elementToVariant(const QDomElement& e)
{
        QVariant value;
        QString type = e.attribute("type");
        if (type == "QStringList") {
                QStringList list;
                for (QDomNode node = e.firstChild(); !node.isNull(); node = node.nextSibling()) {
                        QDomElement e = node.toElement();
                        if (!e.isNull() && e.tagName() == "item") {
                                list += e.text();
                        }
                }
                value = list;
        }
        else if (type == "QVariantList") {
                QVariantList list;
                for (QDomNode node = e.firstChild(); !node.isNull(); node = node.nextSibling()) {
                        QDomElement e = node.toElement();
                        if (!e.isNull() && e.tagName() == "item") {
                                QVariant v = elementToVariant(e);
                                if (v.isValid())
                                        list.append(v);
                        }
                }
                value = list;
        }
        else if (type == "QSize") {
                int width = 0, height = 0;
                for (QDomNode node = e.firstChild(); !node.isNull(); node = node.nextSibling()) {
                        QDomElement e = node.toElement();
                        if (!e.isNull()) {
                                if (e.tagName() == "width") {
                                        width = e.text().toInt();
                                }
                                else if (e.tagName() == "height") {
                                        height = e.text().toInt();
                                }
                        }
                }
                value = QVariant(QSize(width,height));
        }
        else if (type == "QRect") {
                int x = 0, y = 0, width = 0, height = 0;
                for (QDomNode node = e.firstChild(); !node.isNull(); node = node.nextSibling()) {
                        QDomElement e = node.toElement();
                        if (!e.isNull()) {
                                if (e.tagName() == "width") {
                                        width = e.text().toInt();
                                }
                                else if (e.tagName() == "height") {
                                        height = e.text().toInt();
                                }
                                else if (e.tagName() == "x") {
                                        x = e.text().toInt();
                                }
                                else if (e.tagName() == "y") {
                                        y = e.text().toInt();
                                }
                        }
                }
                value = QVariant(QRect(x,y,width,height));
        }
        else if (type == "QByteArray") {
                value = QByteArray();
                for (QDomNode node = e.firstChild(); !node.isNull(); node = node.nextSibling()) {
                        if (node.isText()) {
                                value = Base64::decode(node.toText().data());
                                break;
                        }
                }
        }
        else { // Standard values
                QVariant::Type varianttype;
                bool known = true;

                if (type=="QString") {
                        varianttype = QVariant::String;
                } else if (type=="bool") {
                        varianttype = QVariant::Bool;
                } else if (type=="int") {
                        varianttype = QVariant::Int;
                } else if (type == "QKeySequence") {
                        varianttype = QVariant::KeySequence;
                } else if (type == "QColor") {
                        varianttype = QVariant::Color;
                } else {
                        known = false;
                }

                if (known) {
                        for (QDomNode node = e.firstChild(); !node.isNull(); node = node.nextSibling()) {
                                if ( node.isText() )
                                        value=node.toText().data();
                        }

                        if (!value.isValid())
                                value = QString("");

                        value.convert(varianttype);
                } else {
                        value = QVariant();
                }
        }
        return value;
}

OptionsParser *OptionsParser::instance_ = NULL;


//-----------------------
//---------Base64--------
//-----------------------
QString Base64::encode(const QByteArray &s)
{
        int i;
        int len = s.size();
        char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
        int a, b, c;

        QByteArray p;
        p.resize((len+2)/3*4);
        int at = 0;
        for( i = 0; i < len; i += 3 ) {
                a = ((unsigned char)s[i] & 3) << 4;
                if(i + 1 < len) {
                        a += (unsigned char)s[i + 1] >> 4;
                        b = ((unsigned char)s[i + 1] & 0xF) << 2;
                        if(i + 2 < len) {
                                b += (unsigned char)s[i + 2] >> 6;
                                c = (unsigned char)s[i + 2] & 0x3F;
                        }
                        else
                                c = 64;
                }
                else {
                        b = c = 64;
                }

                p[at++] = tbl[(unsigned char)s[i] >> 2];
                p[at++] = tbl[a];
                p[at++] = tbl[b];
                p[at++] = tbl[c];
        }
        return QString::fromAscii(p);
}

QByteArray Base64::decode(const QString& input)
{
        QByteArray s(QString(input).remove('\n').toUtf8());
        QByteArray p;

        // -1 specifies invalid
        // 64 specifies eof
        // everything else specifies data

        char tbl[] = {
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
                52,53,54,55,56,57,58,59,60,61,-1,-1,-1,64,-1,-1,
                -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
                15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
                -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
                41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        };

        // this should be a multiple of 4
        int len = s.size();

        if(len % 4) {
                return p;
        }

        p.resize(len / 4 * 3);

        int i;
        int at = 0;

        int a, b, c, d;
        c = d = 0;

        for( i = 0; i < len; i += 4 ) {
                a = tbl[(int)s[i]];
                b = tbl[(int)s[i + 1]];
                c = tbl[(int)s[i + 2]];
                d = tbl[(int)s[i + 3]];
                if((a == 64 || b == 64) || (a < 0 || b < 0 || c < 0 || d < 0)) {
                        p.resize(0);
                        return p;
                }
                p[at++] = ((a & 0x3F) << 2) | ((b >> 4) & 0x03);
                p[at++] = ((b & 0x0F) << 4) | ((c >> 2) & 0x0F);
                p[at++] = ((c & 0x03) << 6) | ((d >> 0) & 0x3F);
        }

        if(c & 64)
                p.resize(at - 2);
        else if(d & 64)
                p.resize(at - 1);

        return p;
}
