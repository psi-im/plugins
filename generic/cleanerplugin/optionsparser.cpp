/*
 * optionsparser.cpp - plugin
 * Copyright (C) 2009-2010  Khryukin Evgeny
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
#include <QFile>
#include <QTextCodec>

OptionsParser::OptionsParser(QString fileName, QObject *parent)
        : QObject(parent)
        , fileName_(fileName)
{
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);

    QFile optionsFile(fileName_);
    QFile defaultsFile(":/cleanerplugin/default.xml");
    QDomDocument optionsDoc, defaultsDoc;
    optionsDoc.setContent(&optionsFile);
    defaultsDoc.setContent(&defaultsFile);
    QDomElement optionsElement = optionsDoc.documentElement();
    QDomElement defaultsElement = defaultsDoc.documentElement();
    defaultsElement_ = defaultsElement.firstChildElement("options");
    optionsElement_ = optionsElement.firstChildElement("options");

    QString root;
    findMissingOptions(optionsElement_, root);
}

QStringList OptionsParser::getMissingNodesString()
{
    return missingNodes.keys();
}

QList <QDomNode> OptionsParser::getMissingNodes()
{
    return missingNodes.values();
}

QDomNode OptionsParser::nodeByString(QString key)
{
    return missingNodes.value(key);
}

void OptionsParser::findMissingOptions(QDomElement optElement, QString &root)
{
    QDomNode optionNode = optElement.firstChild();
    while(!optionNode.isNull()) {
        if(!findNode(optionNode.toElement())) {
            QString nodeString = root + optElement.tagName() + "." + optionNode.toElement().tagName();
            missingNodes[nodeString] = optionNode;
        }

        QDomNode childNode = optionNode.firstChild();        
        while(!childNode.isNull()) {
            QString childRoot = root + optElement.tagName()+"." + optionNode.toElement().tagName() + ".";
            findMissingOptions(childNode.toElement(), childRoot);
            childNode = childNode.nextSibling();
        }
        optionNode = optionNode.nextSibling();
    }
root += optElement.tagName()+".";
}

bool OptionsParser::findNode(QDomElement elem)
{
    QString tag = elem.tagName();
    if(defaultsElement_.elementsByTagName(tag).isEmpty())
        return false;
    else
        return true;
}

/*QString OptionsParser::nodeToString(QDomNode node)
{
    QString optionText;
    optionText = node.toElement().tagName();
    QDomNode option = node.firstChild();
    while(!option.isNull()) {
        optionText += QString(".") + option.toElement().tagName();
        option = option.nextSibling();
    }
    return optionText;
}*/
