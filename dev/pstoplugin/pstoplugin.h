#ifndef PSTOPLUGIN_H
#define PSTOPLUGIN_H

#include <QtCore>
#include <QtGui>


#include <QDomElement>


#include "psiplugin.h"
#include "eventfilter.h"
#include "stanzafilter.h"
#include "optionaccessor.h"
#include "optionaccessinghost.h"
#include "plugininfoprovider.h"
#include "preferenceswidget.h"


#define VERSION "0.0.3"
#define PLUGIN_NAME "Psto Plugin"
#define PLUGIN_NAME_SHORT "psto"


#define DEFAULT_USERNAME_COLOR QColor(0, 85, 255)
#define DEFAULT_POST_ID_COLOR QColor(87, 165, 87)
#define DEFAULT_TAG_COLOR QColor(131, 145, 145)
#define DEFAULT_QUOTE_COLOR QColor(131, 145, 145)
#define DEFAULT_MESSAGE_COLOR QColor(0, 0, 0)

#define USERNAME_COLOR "username_color"
#define POST_ID_COLOR "post_id_color"
#define TAG_COLOR "tag_color"
#define QUOTE_COLOR "quote_color"
#define MESSAGE_COLOR "message_color"


// jid1 << jid2 << ...
#define PSTO_JIDS "psto@psto.net" << "d@psto.net"

enum PairType { PairMessage, PairPostId, PairUser, PairLink };
typedef QPair<PairType, QString> RegExpPair;
typedef QList<RegExpPair> RegExpPartitions;

class PstoPlugin : public QObject
                 , public PsiPlugin
                 , public EventFilter
                 , public OptionAccessor
                 , public StanzaFilter
                 , public PluginInfoProvider
{
    Q_OBJECT
    Q_INTERFACES(PsiPlugin EventFilter StanzaFilter OptionAccessor PluginInfoProvider)

public:
    PstoPlugin();

    virtual QString name() const;
    virtual QString shortName() const;
    virtual QString version() const;
    virtual QWidget * options();
    virtual bool enable();
    virtual bool disable();
    virtual bool processEvent(int account, QDomElement & e);
    virtual bool processMessage(int account, const QString & fromJid,
                                const QString & body,
                                const QString & subject);
    virtual bool processOutgoingMessage(int account, const QString &fromJid,
                                        QString &body, const QString &type,
                                        QString &subject);
    virtual void logout(int) {}

    virtual void setOptionAccessingHost(OptionAccessingHost * host);
    virtual void optionChanged(const QString & option);

    virtual void applyOptions();
    virtual void restoreOptions() {}

    virtual bool incomingStanza(int account, const QDomElement & stanza) {
        Q_UNUSED(account); Q_UNUSED(stanza); return false; }
    virtual bool outgoingStanza(int account, QDomElement & stanza) {
        Q_UNUSED(account); Q_UNUSED(stanza); return false; }

    virtual QString pluginInfo();
	virtual QPixmap icon() const;

private:
    const QStringList psto_jids;
    QColor username_color, new_username_color;
    QColor tag_color, new_tag_color;
    QColor quote_color, new_quote_color;
    QColor message_color, new_message_color;
    QColor post_id_color, new_post_id_color;

    const QString post_id_regexp_str;
    const QString user_regexp_str;
    const QString link_regexp_str;
    const QString quote_regexp_str;
    const QString tag_regexp_str;
    const QString post_footer_regexp_str;

    bool enabled;

    OptionAccessingHost * psiOptions;

    QString generateXMPPUrl(const QString & jid, const QString & message);
    QDomElement generateLinkSpan(const QString & value, const QString & link,
                                 const QColor & color, QDomDocument & doc,
                                 const bool bold = false,
                                 const bool italic = false);
    QDomElement generateSpan(const QString & value, const QColor & color,
                             QDomDocument & doc,
                             const bool bold = false,
                             const bool italic = false);
    QDomElement generateLink(const QString & value, const QString & link,
                             QDomDocument & doc);

    // QPair<is matched?, element>
    RegExpPartitions * splitRegexpSimple(const QString & source,
                                         const QString & regexp,
                                         const PairType value);

    RegExpPartitions * splitPostIdRegexp(const QString & source);
    RegExpPartitions * splitUserRegexp(const QString & source);
    RegExpPartitions * splitLinkRegexp(const QString & source);

    void processMessageString(const int pos_number, const QString & string,
                              const QString & psto_jid, QDomElement & html_body);


private slots:
    void usernameColorChanged(QColor);
    void postColorChanged(QColor);
    void tagColorChanged(QColor);
    void quoteColorChanged(QColor);
    void messageColorChanged(QColor);

    void onOptionsClose();

};


#endif // PSTOPLUGIN_H
