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

#include <QDateTime>
#include "juickparser.h"

static const QString juickLink("http://juick.com/%1");

class JuickParser::Private
{
public:
	Private()
	      : tagRx		("^\\s*(?!\\*\\S+\\*)(\\*\\S+)")
	      , pmRx		("^\\nPrivate message from (@.+):(.*)$")
	      , postRx		("\\n@(\\S*):( \\*[^\\n]*){0,1}\\n(.*)\\n\\n(#\\d+)\\s(http://\\S*)\\n$")
	      , replyRx		("\\nReply by @(.*):\\n>(.{,50})\\n\\n(.*)\\n\\n(#\\d+/\\d+)\\s(http://\\S*)\\n$")
//	      , regx		("(\\s+)(#\\d+(?:\\S+)|#\\d+/\\d+(?:\\S+)|@\\S+|_[^\\n]+_|\\*[^\\n]+\\*|/[^\\n]+/|http://\\S+|ftp://\\S+|https://\\S+){1}(\\s+)")
	      , rpostRx		("\\nReply posted.\\n(#.*)\\s(http://\\S*)\\n$")
	      , threadRx	("^\\n@(\\S*):( \\*[^\\n]*){0,1}\\n(.*)\\n(#\\d+)\\s(http://juick.com/\\S+)\\n(.*)")
	      , userRx		("^\\nBlog: http://.*")
	      , yourMesRecRx	("\\n@(\\S*)( recommended your post )(#\\d+)\\.\\s+(http://juick.com/\\S+).*")
	      , singleMsgRx	("^\\n@(\\S+):( \\*[^\\n]*){0,1}\\n(.*)\\n(#\\d+) (\\(.*;{0,1}\\s*(?:\\d+ repl(?:ies|y)){0,1}\\) ){0,1}(http://juick.com/\\S+)\\n$")
	      , lastMsgRx	("^\\n(Last (?:popular ){0,1}messages:)(.*)")
	      , juboRx		("^\\n([^\\n]*)\\n@(\\S*):( [^\\n]*){0,1}\\n(.*)\\n(#\\d+)\\s(http://juick.com/\\S+)\\n$")
	      , msgPostRx	("\\nNew message posted.\\n(#.*)\\s(http://\\S*)\\n$")
//	      , delMsgRx	("^\\nMessage #\\d+ deleted.\\n$")
//	      , delReplyRx	("^\\nReply #\\d+/\\d+ deleted.\\n$")
//	      , idRx		("(#\\d+)(/\\d+){0,1}(\\S+){0,1}")
//	      , nickRx		("(@[\\w\\-\\.@\\|]*)(\\b.*)")
	      , recomendRx	("^\\nRecommended by @(\\S+):\\s+@(\\S+):( \\*[^\\n]+){0,1}\\n+(.*)\\s+(#\\d+) (\\(\\d+ repl(?:ies|y)\\) ){0,1}(http://\\S+)\\s+$")
	      , topTag		("Top 20 tags:")
	{
		pmRx.setMinimal(true);
		replyRx.setMinimal(true);
//		regx.setMinimal(true);
		postRx.setMinimal(true);
		singleMsgRx.setMinimal(true);
		juboRx.setMinimal(true);
	}

	QRegExp tagRx, pmRx, postRx,replyRx,/*regx,*/rpostRx,threadRx, userRx, yourMesRecRx;
	QRegExp singleMsgRx,lastMsgRx,juboRx,msgPostRx,/*delMsgRx,delReplyRx,idRx,nickRx,*/recomendRx;
	const QString topTag;
};



JuickParser::JuickParser(QDomElement *elem)
	: elem_(elem)
{
	if(!d)
		d = new Private();

	juickElement_ = findElement("juick", "http://juick.com/message");

	QString msg = "\n" + originMessage() + "\n";
	msg.replace("&gt;",">");
	msg.replace("&lt;","<");

	//Порядок обработки имеет значение
	if (d->recomendRx.indexIn(msg) != -1) {
			type_ = JM_Recomendation;
			infoText_ = QObject::tr("Recommended by @%1").arg(d->recomendRx.cap(1));
			JuickMessage m(d->recomendRx.cap(2), d->recomendRx.cap(5), d->recomendRx.cap(3).trimmed().split(" ", QString::SkipEmptyParts),
				       d->recomendRx.cap(4), d->recomendRx.cap(7), d->recomendRx.cap(6));
			messages_.append(m);
	}
	else if (d->pmRx.indexIn(msg) != -1) {
		type_ = JM_Private;
	}
	else if (d->userRx.indexIn(msg) != -1) {
		type_ = JM_User_Info;
	}
	else if(d->lastMsgRx.indexIn(msg) != -1) {
		type_ = JM_10_Messages;
		infoText_ =  d->lastMsgRx.cap(1);
		QString mes = d->lastMsgRx.cap(2);
		while(d->singleMsgRx.indexIn(mes) != -1) {
			JuickMessage m(d->singleMsgRx.cap(1), d->singleMsgRx.cap(4), d->singleMsgRx.cap(2).trimmed().split(" ", QString::SkipEmptyParts),
				       d->singleMsgRx.cap(3), d->singleMsgRx.cap(6), d->singleMsgRx.cap(5));
			messages_.append(m);
			mes = mes.right(mes.size() - d->singleMsgRx.matchedLength());
		}
	}
	else if (msg.indexOf(d->topTag) != -1) {
		type_ = JM_Tags_Top;
		infoText_ = d->topTag;
		QStringList tags;
		msg = msg.right(msg.size() - d->topTag.size() - 1);
		while (d->tagRx.indexIn(msg, 0) != -1) {
			tags.append(d->tagRx.cap(1));
			msg = msg.right(msg.size() - d->tagRx.matchedLength());
		}
		JuickMessage m(QString(), QString(), tags, QString(), QString(), QString());
		messages_.append(m);
	}
	else if (d->postRx.indexIn(msg) != -1) {
		type_ = JM_Message;
		infoText_ = QString();
		JuickMessage m(d->postRx.cap(1), d->postRx.cap(4), d->postRx.cap(2).trimmed().split(" ", QString::SkipEmptyParts),
			       d->postRx.cap(3), d->postRx.cap(5), QString());
		messages_.append(m);
	}
	else if (d->replyRx.indexIn(msg) != -1) {
		type_ = JM_Reply;
		infoText_ = d->replyRx.cap(2);
		JuickMessage m(d->replyRx.cap(1), d->replyRx.cap(4), QStringList(),
			       d->replyRx.cap(3), d->replyRx.cap(5), QString());
		messages_.append(m);
	}
	else if (d->rpostRx.indexIn(msg) != -1) {
		type_ = JM_Reply_Posted;
		infoText_ = QObject::tr("Reply posted.");
		JuickMessage m(QString(), d->rpostRx.cap(1), QStringList(),
			       QString(), d->rpostRx.cap(2), QString());
		messages_.append(m);
	}
	else if (d->msgPostRx.indexIn(msg) != -1) {
		type_ = JM_Message_Posted;
		infoText_ = QObject::tr("New message posted.");
		JuickMessage m(QString(), d->msgPostRx.cap(1), QStringList(),
			       QString(), d->msgPostRx.cap(2), QString());
		messages_.append(m);
	}
	else if (d->threadRx.indexIn(msg) != -1) {
		type_ = JM_All_Messages;
		infoText_ = QString();
		JuickMessage m(d->threadRx.cap(1), d->threadRx.cap(4), d->threadRx.cap(2).trimmed().split(" ", QString::SkipEmptyParts),
			       d->threadRx.cap(3), d->threadRx.cap(5), msg.right(msg.size() - d->threadRx.matchedLength() + d->threadRx.cap(6).length()));
		messages_.append(m);
	}
	else if (d->singleMsgRx.indexIn(msg) != -1) {
		type_ = JM_Post_View;
		infoText_ = QString();
		JuickMessage m(d->singleMsgRx.cap(1), d->singleMsgRx.cap(4), d->singleMsgRx.cap(2).trimmed().split(" ", QString::SkipEmptyParts),
			       d->singleMsgRx.cap(3), d->singleMsgRx.cap(6), d->singleMsgRx.cap(5));
		messages_.append(m);
	}
	else if (d->juboRx.indexIn(msg) != -1) {
		type_ = JM_Jubo;
		JuickMessage m(d->juboRx.cap(2), d->juboRx.cap(5), d->juboRx.cap(3).trimmed().split(" ", QString::SkipEmptyParts),
			       d->juboRx.cap(4), d->juboRx.cap(6), QString());
		messages_.append(m);
		infoText_ = d->juboRx.cap(1);
	}
	else if(d->yourMesRecRx.indexIn(msg) != -1) {
		type_ = JM_Your_Post_Recommended;
		JuickMessage m(d->yourMesRecRx.cap(1), d->yourMesRecRx.cap(3), QStringList(),
			       d->yourMesRecRx.cap(2), d->yourMesRecRx.cap(4), QObject::tr(" recommended your post "));
		messages_.append(m);
	}
	else {
		type_ = JM_Other;
	}

//				//mood
//				QRegExp moodRx("\\*mood:\\s(\\S*)\\s(.*)\\n(.*)");
//				//geo
//				QRegExp geoRx("\\*geo:\\s(.*)\\n(.*)");
//				//tune
//				QRegExp tuneRx("\\*tune:\\s(.*)\\n(.*)");
//				if (moodRx.indexIn(recomendRx.cap(4)) != -1){
//					body.appendChild(doc.createElement("br"));
//					QDomElement bold = doc.createElement("b");
//					bold.appendChild(doc.createTextNode("mood: "));
//					body.appendChild(bold);
//					QDomElement img = doc.createElement("icon");
//					img.setAttribute("name","mood/"+moodRx.cap(1).left(moodRx.cap(1).size()-1).toLower());
//					img.setAttribute("text",moodRx.cap(1));
//					body.appendChild(img);
//					body.appendChild(doc.createTextNode(" "+moodRx.cap(2)));
//					msg = " " + moodRx.cap(3) + " ";
//				} else if(geoRx.indexIn(recomendRx.cap(4)) != -1) {
//					body.appendChild(doc.createElement("br"));
//					QDomElement bold = doc.createElement("b");
//					bold.appendChild(doc.createTextNode("geo: "+ geoRx.cap(1) ));
//					body.appendChild(bold);
//					msg = " " + geoRx.cap(2) + " ";
//				} else if(tuneRx.indexIn(recomendRx.cap(4)) != -1) {
//					body.appendChild(doc.createElement("br"));
//					QDomElement bold = doc.createElement("b");
//					bold.appendChild(doc.createTextNode("tune: "+ tuneRx.cap(1) ));
//					body.appendChild(bold);
//					msg = " " + tuneRx.cap(2) + " ";
//				}
}

void JuickParser::reset()
{
	delete d;
	d = 0;
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

QString JuickParser::nick() const
{
	QString nick;
	if(hasJuckNamespace()) {
		nick = juickElement_.attribute("uname");
	}
	return nick;
}

QString JuickParser::originMessage() const
{
	return elem_->firstChildElement("body").text();
}

QString JuickParser::timeStamp() const
{
	QString ts;
	if(hasJuckNamespace()) {
		ts = juickElement_.attribute("ts");
		if(!ts.isEmpty()) {
			QDateTime dt = QDateTime::fromString(ts, "yyyy-MM-dd hh:mm:ss");
			static qlonglong offset = -1;
			if(offset == -1) {
				QDateTime cur = QDateTime::currentDateTime();
				QDateTime utc = cur.toUTC();
				utc.setTimeSpec(Qt::LocalTime);
				offset = utc.secsTo(cur);
			}
			dt = dt.addSecs(offset);
			ts = dt.toString("yyyy-MM-dd hh:mm:ss");
		}
	}
	return ts;
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

JuickParser::Private* JuickParser::d = 0;
