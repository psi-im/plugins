/*
 * optionsparser.h - plugin
 * Copyright (C) 2009-2010  Evgeny Khryukin
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

#ifndef OPTIONSPARSER_H
#define OPTIONSPARSER_H

#include <QDomElement>
#include <QMap>
#include <QObject>
#include <QStringList>

class OptionsParser : public QObject
{
    Q_OBJECT

public:
    OptionsParser(const QString& fileName, QObject *parent = 0);
    QStringList getMissingNodesString() const;
    QList<QDomNode> getMissingNodes() const;
    QDomNode nodeByString(const QString& key) const;


private:
    QString fileName_;
    QDomElement optionsElement_, defaultsElement_;
    QMap<QString, QDomNode> missingNodes;

    void findMissingOptions(const QDomElement& optElement, QString *root);
    bool findNode(const QDomElement& elem) const;
};

#endif // OPTIONSPARSER_H
