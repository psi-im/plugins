/*
 * Copyright (C) 2020  Boris Pek <tehnick-8@yandex.ru>
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
 */

#pragma once

#include <QObject>

class OptionAccessingHost;
class AccountInfoAccessingHost;
class PsiAccountControllingHost;
class QDomElement;
class StanzaSendingHost;

class OpenPgpMessaging : public QObject {
    Q_OBJECT

public:
    explicit OpenPgpMessaging() = default;
    ~OpenPgpMessaging() = default;

    void setStanzaSendingHost(StanzaSendingHost *host);
    void setOptionAccessingHost(OptionAccessingHost *host);
    void setAccountInfoAccessingHost(AccountInfoAccessingHost *host);
    void setPsiAccountControllingHost(PsiAccountControllingHost *host);

    bool incomingStanza(int account, const QDomElement &stanza);
    bool outgoingStanza(int account, QDomElement &stanza);

    void sendPublicKey(int account, const QString &toJid,
                       const QString &keyId, const QString &userId);

private:
    bool processOutgoingPresence(int account, QDomElement &stanza);

private:
    OptionAccessingHost       *m_optionHost    = nullptr;
    AccountInfoAccessingHost * m_accountInfo   = nullptr;
    PsiAccountControllingHost *m_accountHost   = nullptr;
    StanzaSendingHost      *   m_stanzaSending = nullptr;
};
