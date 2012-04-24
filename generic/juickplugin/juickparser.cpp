/*
 * JuickParser - plugin
 * Copyright (C) 2012 Khryukin Evgeny
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

#include "juickparser.h"

static const QString juickLink("http://juick.com/%1");


JuickParser::JuickParser()
	: elem_(0)
{
}

JuickParser::JuickParser(QDomElement *elem)
	: elem_(elem)
{
	juickElement_ = findElement("juick", "http://juick.com/message");
}

bool JuickParser::hasJuckNamespace() const
{
	return !juickElement_.isNull();
}

QString JuickParser::avatarLink() const
{
	QString ava;
	if(hasJuckNamespace()) {
		ava = "/as/"+juickElement_.attribute("uid")+".png";
	}
	return ava;
}

QString JuickParser::photoLink() const
{
	QString photo;
	QDomElement x = findElement("x", "jabber:x:oob");
	if(!x.isNull()) {
		QDomElement url = x.firstChildElement("url");
		if(!url.isNull()) {
			photo = url.text().trimmed();
			if(!photo.endsWith(".jpg", Qt::CaseInsensitive))
				photo.clear();
		}
	}

	return photo;
}

QStringList JuickParser::tags() const
{
	QStringList tags;
	if(hasJuckNamespace()) {
		QDomElement tag = juickElement_.firstChildElement("tag");
		while(!tag.isNull()) {
			tags.append(tag.text());
			tag = tag.nextSiblingElement("tag");
		}
	}
	return tags;
}

QString JuickParser::messageId() const
{
	QString id;
	if(hasJuckNamespace()) {
		id = juickElement_.attribute("mid");
	}
	return id;
}

QString JuickParser::body() const
{
	QString body;
	if(hasJuckNamespace()) {
		QDomElement b = juickElement_.firstChildElement("body");
		if(!b.isNull())
			body = b.text();
	}
	return body;
}

QString JuickParser::nick() const
{
	QString nick;
	if(hasJuckNamespace()) {
		nick = juickElement_.attribute("uname");
	}
	return nick;
}

QString JuickParser::link() const
{
	QString link;
	if(hasJuckNamespace()) {
		link = juickLink.arg(messageId());
	}
	return link;
}

QDomElement JuickParser::findElement(const QString &tagName, const QString &xmlns) const
{
	if(!elem_)
		return QDomElement();

	QDomNode e = elem_->firstChild();
	while(!e.isNull()) {
		if(e.isElement()) {
			QDomElement el = e.toElement();
			if(el.tagName() == tagName && el.attribute("xmlns") == xmlns)
				return el;
		}
		e = e.nextSibling();
	}
	return QDomElement();
}
