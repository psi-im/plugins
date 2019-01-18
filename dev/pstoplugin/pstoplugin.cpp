#include "pstoplugin.h"

PstoPlugin::PstoPlugin()
    : psto_jids(QStringList() << PSTO_JIDS)
    , post_id_regexp_str("(#[\\w/]+)")
    , user_regexp_str("(\\@[\\w-_]+)")
    , link_regexp_str("((https?|ftp)://\\S+)")
    , quote_regexp_str("^[>] .*$")
    , tag_regexp_str("^[*] .*$")

    // #abcde[/123] http://dfgfdhfdh.psto.net/abcde[#123]
    , post_footer_regexp_str("^(\\#(\\w+)(/(\\d+))?) (http://(.*)psto[.]net/\\2(\\#\\4)?)$")
    , enabled(false)
{
}


QString PstoPlugin::name() const {
    return PLUGIN_NAME;
}

QString PstoPlugin::shortName() const {
    return PLUGIN_NAME_SHORT;
}

QString PstoPlugin::version() const {
    return VERSION;
}

QWidget * PstoPlugin::options() {
    PreferencesWidget * preferences_widget = new PreferencesWidget(
            username_color,
            post_id_color,
            tag_color,
            quote_color,
            message_color
            );
    connect(preferences_widget, SIGNAL(destroyed()), this, SLOT(onOptionsClose()));

    connect(preferences_widget, SIGNAL(usernameColorChanged(QColor)),
            this, SLOT(usernameColorChanged(QColor)));
    connect(preferences_widget, SIGNAL(postColorChanged(QColor)),
            this, SLOT(postColorChanged(QColor)));
    connect(preferences_widget, SIGNAL(tagColorChanged(QColor)),
            this, SLOT(tagColorChanged(QColor)));
    connect(preferences_widget, SIGNAL(quoteColorChanged(QColor)),
            this, SLOT(quoteColorChanged(QColor)));
    connect(preferences_widget, SIGNAL(messageColorChanged(QColor)),
            this, SLOT(messageColorChanged(QColor)));

    return preferences_widget;
}

void PstoPlugin::usernameColorChanged(QColor c) {
    new_username_color = c;
}

void PstoPlugin::postColorChanged(QColor c) {
    new_post_id_color = c;
}

void PstoPlugin::tagColorChanged(QColor c) {
    new_tag_color = c;
}

void PstoPlugin::quoteColorChanged(QColor c) {
    new_quote_color = c;
}

void PstoPlugin::messageColorChanged(QColor c) {
    new_message_color = c;
}

void PstoPlugin::onOptionsClose() {

}

void PstoPlugin::applyOptions() {
    username_color = new_username_color;
    psiOptions->setPluginOption(USERNAME_COLOR, username_color);

    post_id_color = new_post_id_color;
    psiOptions->setPluginOption(POST_ID_COLOR, post_id_color);

    tag_color = new_tag_color;
    psiOptions->setPluginOption(TAG_COLOR, tag_color);

    quote_color = new_quote_color;
    psiOptions->setPluginOption(QUOTE_COLOR, quote_color);

    message_color = new_message_color;
    psiOptions->setPluginOption(MESSAGE_COLOR, message_color);
}

void PstoPlugin::optionChanged(const QString & option) {
    Q_UNUSED(option);
}

QString PstoPlugin::pluginInfo() {
    return "plugin info";
}

bool PstoPlugin::enable() {
    if (psiOptions) {
        enabled = true;
    }
    return enabled;
}

bool PstoPlugin::disable() {
    enabled = false;
    return true;
}

bool PstoPlugin::processMessage(int account, const QString & fromJid,
                                const QString & body,
                                const QString & subject)
{
    Q_UNUSED(account);
    Q_UNUSED(fromJid);
    Q_UNUSED(body);
    Q_UNUSED(subject);
    return false;
}

bool PstoPlugin::processOutgoingMessage(int account, const QString &fromJid,
                                        QString &body, const QString &type,
                                        QString &subject)
{
    Q_UNUSED(account);
    Q_UNUSED(fromJid);
    Q_UNUSED(body);
    Q_UNUSED(type);
    Q_UNUSED(subject);
    return false;
}

bool PstoPlugin::processEvent(int account, QDomElement &e) {
    Q_UNUSED(account);
    Q_UNUSED(e);

    if (!enabled) {
        return false;
    }

    QDomDocument doc = e.ownerDocument();

    QString jid = e.childNodes().at(3).firstChild().nodeValue().split("/").at(0); // always here
    if (psto_jids.contains(jid)) {
        QString full_jid = e.childNodes().at(5).attributes()
                           .namedItem("from").nodeValue();

        QDomElement body = e.childNodes().at(5).firstChildElement(); // the same
        QDomText body_text = body.firstChild().toText();

        QDomElement html = doc.createElement("html");
        html.setAttribute("xmlns", "http://jabber.org/protocol/xhtml-im");
        body.parentNode().appendChild(html);

        QDomElement html_body = doc.createElement("body");
        html_body.setAttribute("xmlns", "http://www.w3.org/1999/xhtml");
        html.appendChild(html_body);

        QStringList message_strings = body_text.nodeValue().split("\n");

        int line_number = 0;
        foreach (QString line, message_strings) {
            processMessageString(line_number, line, full_jid, html_body);
            line_number++;
        }
    }

    return false;
}

void PstoPlugin::setOptionAccessingHost(OptionAccessingHost * host) {
    psiOptions = host;

    QVariant value;

    value = psiOptions->getPluginOption(USERNAME_COLOR);
    if (value.isNull()) {
        username_color = DEFAULT_USERNAME_COLOR;
        psiOptions->setPluginOption(USERNAME_COLOR, DEFAULT_USERNAME_COLOR);
    } else {
        username_color = qvariant_cast<QColor>(value);
    }
    new_username_color = username_color;

    value = psiOptions->getPluginOption(POST_ID_COLOR);
    if (value.isNull()) {
        post_id_color = DEFAULT_POST_ID_COLOR;
        psiOptions->setPluginOption(POST_ID_COLOR, DEFAULT_POST_ID_COLOR);
    } else {
        post_id_color = qvariant_cast<QColor>(value);
    }
    new_post_id_color = post_id_color;

    value = psiOptions->getPluginOption(TAG_COLOR);
    if (value.isNull()) {
        tag_color = DEFAULT_TAG_COLOR;
        psiOptions->setPluginOption(TAG_COLOR, DEFAULT_TAG_COLOR);
    } else {
        tag_color = qvariant_cast<QColor>(value);
    }
    new_tag_color = tag_color;

    value = psiOptions->getPluginOption(QUOTE_COLOR);
    if (value.isNull()) {
        quote_color = DEFAULT_QUOTE_COLOR;
        psiOptions->setPluginOption(QUOTE_COLOR, DEFAULT_QUOTE_COLOR);
    } else {
        quote_color = qvariant_cast<QColor>(value);
    }
    new_quote_color = quote_color;

    value = psiOptions->getPluginOption(MESSAGE_COLOR);
    if (value.isNull()) {
        message_color = DEFAULT_MESSAGE_COLOR;
        psiOptions->setPluginOption(MESSAGE_COLOR, DEFAULT_MESSAGE_COLOR);
    } else {
        message_color = qvariant_cast<QColor>(value);
    }
    new_message_color = message_color;
}



QString PstoPlugin::generateXMPPUrl(const QString & jid, const QString & message) {
    QString message_percent(message.toLatin1().toPercentEncoding());
    return QString("xmpp:%1?message;type=chat;body=%2").arg(jid, message_percent);
}

QDomElement PstoPlugin::generateLinkSpan(const QString & value,
                                         const QString & link_str,
                                         const QColor & color,
                                         QDomDocument & doc,
                                         const bool bold,
                                         const bool italic)
{
    QString style = QString("color: %1").arg(color.name());
    if (bold) {
        style.append("; font-weight: bold");
    }
    if (italic) {
        style.append("; font-style: italic");
    }

    QDomElement span = doc.createElement("span");
    span.setAttribute("style", style);
    span.appendChild(doc.createTextNode(value));

    QDomElement link = doc.createElement("a");
    link.setAttribute("href", link_str);
    link.setAttribute("style", "text-decoration: none");
    link.appendChild(span);

    return link;
}

QDomElement PstoPlugin::generateSpan(const QString & value,
                                     const QColor & color,
                                     QDomDocument & doc,
                                     const bool bold,
                                     const bool italic)
{
    QString style = QString("color: %1").arg(color.name());
    if (bold) {
        style.append("; font-weight: bold");
    }
    if (italic) {
        style.append("; font-style: italic");
    }

    QDomElement span = doc.createElement("span");
    span.setAttribute("style", style);
    span.appendChild(doc.createTextNode(value));

    return span;
}

QDomElement PstoPlugin::generateLink(const QString & value,
                                     const QString & link_str,
                                     QDomDocument & doc)
{
    QDomElement link = doc.createElement("a");
    link.setAttribute("href", link_str);
    link.appendChild(doc.createTextNode(value));

    return link;
}


RegExpPartitions * PstoPlugin::splitRegexpSimple(const QString & source1,
                                                 const QString & regexp_str,
                                                 const PairType value) {
    QString source(source1);
    QRegExp regexp(regexp_str);

    RegExpPartitions* result = new RegExpPartitions();

    int index = regexp.indexIn(source);
    while (index != -1) {
        QString before_match = source.left(index);
        if (!before_match.isEmpty()) {
            result->append(RegExpPair(PairMessage, before_match));
        }

        QString match = regexp.cap(0);
        result->append(RegExpPair(value, match));

        source = source.right(source.length() - index - match.length()); // for a time, without optimizations
        index = regexp.indexIn(source);
    }
    result->append(RegExpPair(PairMessage, source));
    return result;
}

RegExpPartitions * PstoPlugin::splitPostIdRegexp(const QString &source) {
    return splitRegexpSimple(source, post_id_regexp_str, PairPostId);
}

RegExpPartitions * PstoPlugin::splitUserRegexp(const QString &source) {
    return splitRegexpSimple(source, user_regexp_str, PairUser);
}

RegExpPartitions * PstoPlugin::splitLinkRegexp(const QString &source) {
    return splitRegexpSimple(source, link_regexp_str, PairLink);
}

void PstoPlugin::processMessageString(const int /*pos_number*/,
                                      const QString & line,
                                      const QString & psto_jid,
                                      QDomElement &html_body)
{
    QDomDocument doc = html_body.ownerDocument();

//    qDebug() << pos_number << line;

    QRegExp post_footer_regexp(post_footer_regexp_str);
    if (post_footer_regexp.indexIn(line) != -1) {
        QString full_post = post_footer_regexp.cap(1);
        QString post_id = post_footer_regexp.cap(2);
        QString post_link = post_footer_regexp.cap(5);
        bool is_comment = !post_footer_regexp.cap(3).isEmpty();

        // #abcde/123
        html_body.appendChild(generateLinkSpan(
            full_post,
            generateXMPPUrl(psto_jid, full_post),
            post_id_color,
            doc,
            true, false));

        html_body.appendChild(doc.createTextNode(" "));

        // ! #abcde/123
        html_body.appendChild(generateLinkSpan(
            "!",
            generateXMPPUrl(psto_jid, QString("! %1").arg(full_post)),
            post_id_color,
            doc,
            true, false));

        html_body.appendChild(doc.createTextNode(" "));

        if (is_comment) {
            // u #abcde
            html_body.appendChild(generateLinkSpan(
                "U",
                generateXMPPUrl(psto_jid, QString("u #%1").arg(post_id)),
                post_id_color,
                doc,
                true, false));
        } else {
            // s #abcde
            html_body.appendChild(generateLinkSpan(
                "S",
                generateXMPPUrl(psto_jid, QString("s #%1").arg(post_id)),
                post_id_color,
                doc,
                true, false));

            html_body.appendChild(doc.createTextNode(" "));

            // #abcde+
            html_body.appendChild(generateLinkSpan(
                "+",
                generateXMPPUrl(psto_jid, QString("#%1+").arg(post_id)),
                post_id_color,
                doc,
                true, false));
        }

        html_body.appendChild(doc.createTextNode(" "));

        // ~ #abcde/123
        html_body.appendChild(generateLinkSpan(
            "~",
            generateXMPPUrl(psto_jid, QString("~ %1").arg(full_post)),
            post_id_color,
            doc,
            true, false));

        html_body.appendChild(doc.createTextNode(" "));

        html_body.appendChild(generateLink(post_link, post_link, doc));
        return;
    }

    if (/*pos_number == 3 &&*/ QRegExp(tag_regexp_str).indexIn(line) != -1) {
        html_body.appendChild(generateSpan(line, tag_color, doc, false, true));
        html_body.appendChild(doc.createElement("br"));
        return;
    }

    if (QRegExp(quote_regexp_str).indexIn(line) != -1) {
        html_body.appendChild(generateSpan(line, quote_color, doc));
        html_body.appendChild(doc.createElement("br"));
        return;
    }

    RegExpPartitions * link_list = splitLinkRegexp(line);
    foreach (RegExpPair p, (*link_list)) {
        if (p.first == PairLink) {
            html_body.appendChild(generateLink(
                p.second, p.second, doc));

        } else {
            RegExpPartitions * user_list = splitUserRegexp(p.second);
            foreach (RegExpPair p, (*user_list)) {
                if (p.first == PairUser) {
                    html_body.appendChild(generateLinkSpan(
                        p.second,
                        generateXMPPUrl(psto_jid, p.second),
                        username_color,
                        doc));
                } else {
                    RegExpPartitions * post_id_list = splitPostIdRegexp(p.second);
                    foreach (RegExpPair p, (*post_id_list)) {
                        if (p.first == PairPostId) {
                            html_body.appendChild(generateLinkSpan(
                                p.second,
                                generateXMPPUrl(psto_jid, p.second),
                                post_id_color,
                                doc,
                                true, false));
                        } else {
                            html_body.appendChild(doc.createTextNode(p.second));
                        }
                    }
                    delete post_id_list;
                }
            }
            delete user_list;
        }
    }
    delete link_list;

    html_body.appendChild(doc.createElement("br"));
}

QPixmap PstoPlugin::icon() const
{
	return QPixmap(":/icons/psto.png");
}
