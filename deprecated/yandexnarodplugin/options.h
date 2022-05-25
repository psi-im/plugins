/*
    proxy.h

    Copyright (c) 2011 by Evgeny Khryukin

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/

#ifndef PROXY_H
#define PROXY_H

#include "QVariant"
#include <QNetworkCookie>
#include <QNetworkProxy>

class ApplicationInfoAccessingHost;
class OptionAccessingHost;

#define CONST_COOKIES "cookies"
#define CONST_LOGIN "login"
#define CONST_PASS_OLD "pass"
#define CONST_PASS "pass-encoded"
#define CONST_TEMPLATE "template"
#define CONST_LAST_FOLDER "lastfolder"
#define CONST_WIDTH "width"
#define CONST_HEIGHT "height"
#define POPUP_OPTION_NAME ".popupinterval"
#define VERSION "0.1.4"

#define O_M(x) Options::message(x)

enum MessageType { MAuthStart, MAuthOk, MAuthError, MCancel, MChooseFile, MUploading, MError, MRemoveCookie };

class Options : public QObject {
    Q_OBJECT
public:
    static Options *instance();
    static void     reset();
    static QString  message(MessageType type);

    static QString encodePassword(const QString &pass);
    static QString decodePassword(const QString &pass);

    void                  setApplicationInfoAccessingHost(ApplicationInfoAccessingHost *host);
    void                  setOptionAccessingHost(OptionAccessingHost *host);
    void                  setOption(const QString &name, const QVariant &value);
    QVariant              getOption(const QString &name, const QVariant &def = QVariant::Invalid);
    QNetworkProxy         getProxy() const;
    bool                  useProxy() const;
    void                  saveCookies(const QList<QNetworkCookie> &cooks);
    QList<QNetworkCookie> loadCookies();

private:
    static Options *instance_;
    Options();
    virtual ~Options();

    ApplicationInfoAccessingHost *appInfo;
    OptionAccessingHost          *options;
};

#endif // PROXY_H
