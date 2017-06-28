/*
 * JuickParser - plugin
 * Copyright (C) 2012 Kravtsov Nikolai, Evgeny Khryukin
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

struct JuickMessage {
	JuickMessage(const QString& _unick, const QString& _mes, const QStringList& _tags,
		     const QString& _body, const QString& _link, const QString& info)
		: unick(_unick), messageId(_mes), tags(_tags), body(_body), link(_link), infoText(info)
	{
	}
	QString unick;
	QString messageId;
	QStringList tags;
	QString body;
	QString link;
	QString infoText;
};

typedef QList<JuickMessage> JuickMessages;

class JuickParser
{
public:	
	JuickParser(QDomElement* elem);
	virtual ~JuickParser() {}

	static void reset();

	enum JMType {
		JM_Other = 0,
		JM_10_Messages,
		JM_Tags_Top,
		JM_Recomendation,
		JM_Message,
		JM_Message_Posted,
		JM_Reply,
		JM_Reply_Posted,
		JM_All_Messages,
		JM_Post_View,
		JM_Jubo,
		JM_Private,
		JM_User_Info,
		JM_Your_Post_Recommended
	};

	bool hasJuckNamespace() const;
	QString avatarLink() const;
	QString photoLink() const;
	QString nick() const;
	QString infoText() const { return infoText_; }
	JMType type() const { return type_; }
	JuickMessages getMessages() const { return messages_; }
	QString originMessage() const;
	QString timeStamp() const;
	QDomElement findElement(const QString& tagName, const QString& xmlns) const;	

private:
	JuickParser() {}
	Q_DISABLE_COPY(JuickParser)

private:
	QDomElement* elem_;
	QDomElement juickElement_;
	QDomElement userElement_;
	JMType type_;
	QString infoText_;
	JuickMessages messages_;
	class Private;
	static Private* d;
};

#endif // JUICKPARSER_H
