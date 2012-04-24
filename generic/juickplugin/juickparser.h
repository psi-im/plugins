/*
 * JuickParser - plugin
 * Copyright (C) 2012 Kravtsov Nikolai, Khryukin Evgeny
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */


#ifndef JUICKPARSER_H
#define JUICKPARSER_H

#include <QDomElement>
#include <QStringList>

class JuickParser
{
public:
	JuickParser();
	JuickParser(QDomElement* elem);

	bool hasJuckNamespace() const;
	QString avatarLink() const;
	QString photoLink() const;
	QStringList tags() const;
	QString messageId() const;
	QString body() const;
	QString nick() const;
	QString link() const;
	QDomElement findElement(const QString& tagName, const QString& xmlns) const;

private:


private:
	QDomElement* elem_;
	QDomElement juickElement_;
};

#endif // JUICKPARSER_H
