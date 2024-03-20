/*
 * JuickParser - plugin
 * Copyright (C) 2012 Evgeny Khryukin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "juickparser.h"
#include <QDateTime>
#include <QObject>
#include <QRegularExpression>

static const QString juickLink("https://juick.com/%1");

class JuickParser::Private {
public:
    Private() :
        tagRx("^\\s*(?!\\*\\S+\\*)(\\*\\S+)"), pmRx("^\\nPrivate message from (@.+):(.*)$"),
        postRx("\\n@(\\S*):( \\*[^\\n]*){0,1}\\n(.*)\\n\\n(#\\d+)\\s(https://\\S*)\\n$"),
        replyRx("\\nReply by @(.*):\\n>(.{,50})\\n\\n(.*)\\n\\n(#\\d+/\\d+)\\s(https://\\S*)\\n$")
        //          , regx
        //          ("(\\s+)(#\\d+(?:\\S+)|#\\d+/\\d+(?:\\S+)|@\\S+|_[^\\n]+_|\\*[^\\n]+\\*|/[^\\n]+/|http://\\S+|ftp://\\S+|https://\\S+){1}(\\s+)")
        ,
        rpostRx("\\nReply posted.\\n(#.*)\\s(https://\\S*)\\n$"),
        threadRx("^\\n@(\\S*):( \\*[^\\n]*){0,1}\\n(.*)\\n(#\\d+)\\s(https://juick.com/\\S+)\\n(.*)"),
        userRx("^\\nBlog: https://.*"),
        yourMesRecRx("\\n@(\\S*)( recommended your post )(#\\d+)\\.\\s+(https://juick.com/\\S+).*"),
        singleMsgRx("^\\n@(\\S+):( \\*[^\\n]*){0,1}\\n(.*)\\n(#\\d+) (\\(.*;{0,1}\\s*(?:\\d+ repl(?:ies|y)){0,1}\\) "
                    "){0,1}(https://juick.com/\\S+)\\n$"),
        lastMsgRx("^\\n(Last (?:popular ){0,1}messages:)(.*)"),
        juboRx("^\\n([^\\n]*)\\n@(\\S*):( [^\\n]*){0,1}\\n(.*)\\n(#\\d+)\\s(https://juick.com/\\S+)\\n$"),
        msgPostRx("\\nNew message posted.\\n(#.*)\\s(https://\\S*)\\n$")
        //          , delMsgRx    ("^\\nMessage #\\d+ deleted.\\n$")
        //          , delReplyRx    ("^\\nReply #\\d+/\\d+ deleted.\\n$")
        //          , idRx        ("(#\\d+)(/\\d+){0,1}(\\S+){0,1}")
        //          , nickRx        ("(@[\\w\\-\\.@\\|]*)(\\b.*)")
        ,
        recomendRx("^\\nRecommended by @(\\S+):\\s+@(\\S+):( \\*[^\\n]+){0,1}\\n+(.*)\\s+(#\\d+) (\\(\\d+ "
                   "repl(?:ies|y)\\) ){0,1}(https://\\S+)\\s+$"),
        topTag("Top 20 tags:")
    {
        pmRx.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
        replyRx.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
        //        regx.setMinimal(true);
        postRx.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
        singleMsgRx.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
        juboRx.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
    }

    QRegularExpression tagRx, pmRx, postRx, replyRx, /*regx,*/ rpostRx, threadRx, userRx, yourMesRecRx;
    QRegularExpression singleMsgRx, lastMsgRx, juboRx, msgPostRx, /*delMsgRx,delReplyRx,idRx,nickRx,*/ recomendRx;
    const QString      topTag;
};

JuickParser::JuickParser(QDomElement *elem) : elem_(elem)
{
    if (!d)
        d = new Private();

    juickElement_ = findElement("juick", "https://juick.com/message");
    userElement_  = juickElement_.firstChildElement("user");

    QString msg = "\n" + originMessage() + "\n";
    msg.replace("&gt;", ">");
    msg.replace("&lt;", "<");
    auto match = d->recomendRx.match(msg);
    // Порядок обработки имеет значение
    if (match.hasMatch()) {
        type_     = JM_Recomendation;
        infoText_ = QObject::tr("Recommended by @%1").arg(match.captured(1));
        JuickMessage m(match.captured(2), match.captured(5), match.captured(3).trimmed().split(" ", Qt::SkipEmptyParts),
                       match.captured(4), match.captured(7), match.captured(6));
        messages_.append(m);
    } else if (d->pmRx.match(msg).hasMatch()) {
        type_ = JM_Private;
    } else if (d->userRx.match(msg).hasMatch()) {
        type_ = JM_User_Info;
    } else if ((match = d->lastMsgRx.match(msg)).hasMatch()) {
        type_       = JM_10_Messages;
        infoText_   = match.captured(1);
        QString mes = match.captured(2);
        while (d->singleMsgRx.match(mes).hasMatch()) {
            match = d->singleMsgRx.match(mes);
            JuickMessage m(match.captured(1), match.captured(4),
                           match.captured(2).trimmed().split(" ", Qt::SkipEmptyParts), match.captured(3),
                           match.captured(6), match.captured(5));
            messages_.append(m);
            mes = mes.right(mes.size() - match.capturedLength(0));
        }
    } else if (msg.indexOf(d->topTag) != -1) {
        type_     = JM_Tags_Top;
        infoText_ = d->topTag;
        QStringList tags;
        msg = msg.right(msg.size() - d->topTag.size() - 1);
        while ((match = d->tagRx.match(msg, 0)).hasMatch()) {
            tags.append(match.captured(1));
            msg = msg.right(msg.size() - match.capturedLength(0));
        }
        JuickMessage m(QString(), QString(), tags, QString(), QString(), QString());
        messages_.append(m);
    } else if ((match = d->postRx.match(msg)).hasMatch()) {
        type_     = JM_Message;
        infoText_ = QString();
        JuickMessage m(match.captured(1), match.captured(4), match.captured(2).trimmed().split(" ", Qt::SkipEmptyParts),
                       match.captured(3), match.captured(5), QString());
        messages_.append(m);
    } else if ((match = d->replyRx.match(msg)).hasMatch()) {
        type_     = JM_Reply;
        infoText_ = match.captured(2);
        JuickMessage m(match.captured(1), match.captured(4), QStringList(), match.captured(3), match.captured(5),
                       QString());
        messages_.append(m);
    } else if ((match = d->rpostRx.match(msg)).hasMatch()) {
        type_     = JM_Reply_Posted;
        infoText_ = QObject::tr("Reply posted.");
        JuickMessage m(QString(), match.captured(1), QStringList(), QString(), match.captured(2), QString());
        messages_.append(m);
    } else if ((match = d->msgPostRx.match(msg)).hasMatch()) {
        type_     = JM_Message_Posted;
        infoText_ = QObject::tr("New message posted.");
        JuickMessage m(QString(), match.captured(1), QStringList(), QString(), match.captured(2), QString());
        messages_.append(m);
    } else if ((match = d->threadRx.match(msg)).hasMatch()) {
        type_     = JM_All_Messages;
        infoText_ = QString();
        JuickMessage m(match.captured(1), match.captured(4), match.captured(2).trimmed().split(" ", Qt::SkipEmptyParts),
                       match.captured(3), match.captured(5),
                       msg.right(msg.size() - match.capturedLength(0) + match.captured(6).length()));
        messages_.append(m);
    } else if ((match = d->singleMsgRx.match(msg)).hasMatch()) {
        type_     = JM_Post_View;
        infoText_ = QString();
        JuickMessage m(match.captured(1), match.captured(4), match.captured(2).trimmed().split(" ", Qt::SkipEmptyParts),
                       match.captured(3), match.captured(6), match.captured(5));
        messages_.append(m);
    } else if ((match = d->juboRx.match(msg)).hasMatch()) {
        type_ = JM_Jubo;
        JuickMessage m(match.captured(2), match.captured(5), match.captured(3).trimmed().split(" ", Qt::SkipEmptyParts),
                       match.captured(4), match.captured(6), QString());
        messages_.append(m);
        infoText_ = match.captured(1);
    } else if ((match = d->yourMesRecRx.match(msg)).hasMatch()) {
        type_ = JM_Your_Post_Recommended;
        JuickMessage m(match.captured(1), match.captured(3), QStringList(), match.captured(2), match.captured(4),
                       QObject::tr(" recommended your post "));
        messages_.append(m);
    } else {
        type_ = JM_Other;
    }

    //                //mood
    //                QRegExp moodRx("\\*mood:\\s(\\S*)\\s(.*)\\n(.*)");
    //                //geo
    //                QRegExp geoRx("\\*geo:\\s(.*)\\n(.*)");
    //                //tune
    //                QRegExp tuneRx("\\*tune:\\s(.*)\\n(.*)");
    //                if (moodRx.indexIn(recomendRx.cap(4)) != -1){
    //                    body.appendChild(doc.createElement("br"));
    //                    QDomElement bold = doc.createElement("b");
    //                    bold.appendChild(doc.createTextNode("mood: "));
    //                    body.appendChild(bold);
    //                    QDomElement img = doc.createElement("icon");
    //                    img.setAttribute("name","mood/"+moodRx.cap(1).left(moodRx.cap(1).size()-1).toLower());
    //                    img.setAttribute("text",moodRx.cap(1));
    //                    body.appendChild(img);
    //                    body.appendChild(doc.createTextNode(" "+moodRx.cap(2)));
    //                    msg = " " + moodRx.cap(3) + " ";
    //                } else if(geoRx.indexIn(recomendRx.cap(4)) != -1) {
    //                    body.appendChild(doc.createElement("br"));
    //                    QDomElement bold = doc.createElement("b");
    //                    bold.appendChild(doc.createTextNode("geo: "+ geoRx.cap(1) ));
    //                    body.appendChild(bold);
    //                    msg = " " + geoRx.cap(2) + " ";
    //                } else if(tuneRx.indexIn(recomendRx.cap(4)) != -1) {
    //                    body.appendChild(doc.createElement("br"));
    //                    QDomElement bold = doc.createElement("b");
    //                    bold.appendChild(doc.createTextNode("tune: "+ tuneRx.cap(1) ));
    //                    body.appendChild(bold);
    //                    msg = " " + tuneRx.cap(2) + " ";
    //                }
}

void JuickParser::reset()
{
    delete d;
    d = nullptr;
}

bool JuickParser::hasJuckNamespace() const { return !juickElement_.isNull(); }

QString JuickParser::avatarLink() const
{
    QString ava;
    if (!userElement_.isNull()) {
        ava = "/as/" + userElement_.attribute("uid") + ".png";
    }
    return ava;
}

QString JuickParser::photoLink() const
{
    QString     photo;
    QDomElement x = findElement("x", "jabber:x:oob");
    if (!x.isNull()) {
        QDomElement url = x.firstChildElement("url");
        if (!url.isNull()) {
            photo = url.text().trimmed();
            if (!photo.endsWith(".jpg", Qt::CaseInsensitive) && !photo.endsWith(".png", Qt::CaseInsensitive))
                photo.clear();
        }
    }

    return photo;
}

QString JuickParser::nick() const
{
    QString nick;
    if (!userElement_.isNull()) {
        nick = userElement_.attribute("uname");
    }
    return nick;
}

QString JuickParser::originMessage() const { return elem_->firstChildElement("body").text(); }

QString JuickParser::timeStamp() const
{
    QString ts;
    if (hasJuckNamespace()) {
        ts = juickElement_.attribute("ts");
        if (!ts.isEmpty()) {
            QDateTime        dt     = QDateTime::fromString(ts, "yyyy-MM-dd hh:mm:ss");
            static qlonglong offset = -1;
            if (offset == -1) {
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
    if (!elem_)
        return QDomElement();

    QDomNode e = elem_->firstChild();
    while (!e.isNull()) {
        if (e.isElement()) {
            QDomElement el = e.toElement();
            if (el.tagName() == tagName && el.namespaceURI() == xmlns)
                return el;
        }
        e = e.nextSibling();
    }
    return QDomElement();
}

JuickParser::Private *JuickParser::d = nullptr;
