/*
 * OtrMessaging.cpp - interface to libotr
 * Copyright (C) 2007  Timo Engel (timo-e@freenet.de)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include "OtrMessaging.hpp"
#include "OtrInternal.hpp"

namespace psiotr
{

Fingerprint::Fingerprint()
    : fingerprint(NULL)
{

}

Fingerprint::Fingerprint(const Fingerprint &fp)
    : fingerprint(fp.fingerprint),
      account(fp.account),
      username(fp.username),
      fingerprintHuman(fp.fingerprintHuman),
      trust(fp.trust),
      messageState(fp.messageState)
{

}

Fingerprint::Fingerprint(unsigned char* fingerprint,
                         QString account, QString username,
                         QString trust, QString messageState)
    : fingerprint(fingerprint),
      account(account),
      username(username),
      trust(trust),
      messageState(messageState)
{
    fingerprintHuman = OtrInternal::humanFingerprint(fingerprint);
}

//-----------------------------------------------------------------------------

OtrMessaging::OtrMessaging(OtrCallback* callback, OtrPolicy policy)
    : m_otrPolicy(policy),
      m_impl(new OtrInternal(callback, m_otrPolicy)),
      m_callback(callback)
{
}

//-----------------------------------------------------------------------------

OtrMessaging::~OtrMessaging()
{
    delete m_impl;
}

//-----------------------------------------------------------------------------

QString OtrMessaging::encryptMessage(const QString& from, const QString& to,
                                     const QString& message)
{
    return m_impl->encryptMessage(from, to, message);
}

//-----------------------------------------------------------------------------

bool OtrMessaging::decryptMessage(const QString& from, const QString& to,
                                  const QString& message, QString& decrypted)
{
    return m_impl->decryptMessage(from, to, message, decrypted);
}

//-----------------------------------------------------------------------------

QList<Fingerprint> OtrMessaging::getFingerprints()
{
    return m_impl->getFingerprints();
}

//-----------------------------------------------------------------------------

void OtrMessaging::verifyFingerprint(const psiotr::Fingerprint& fingerprint,
                                     bool verified)
{
    m_impl->verifyFingerprint(fingerprint, verified);
}

//-----------------------------------------------------------------------------

void OtrMessaging::deleteFingerprint(const psiotr::Fingerprint& fingerprint)
{
    m_impl->deleteFingerprint(fingerprint);
}

//-----------------------------------------------------------------------------

QHash<QString, QString> OtrMessaging::getPrivateKeys()
{
    return m_impl->getPrivateKeys();
}

//-----------------------------------------------------------------------------

void OtrMessaging::deleteKey(const QString& account)
{
    m_impl->deleteKey(account);
}

//-----------------------------------------------------------------------------

void OtrMessaging::startSession(const QString& account, const QString& jid)
{
    m_impl->startSession(account, jid);
}

//-----------------------------------------------------------------------------

void OtrMessaging::endSession(const QString& account, const QString& jid)
{
    m_impl->endSession(account, jid);
}

//-----------------------------------------------------------------------------

void OtrMessaging::expireSession(const QString& account, const QString& jid)
{
    m_impl->expireSession(account, jid);
}

//-----------------------------------------------------------------------------

void OtrMessaging::startSMP(const QString& account, const QString& jid,
                            const QString& question, const QString& secret)
{
    m_impl->startSMP(account, jid, question, secret);
}

//-----------------------------------------------------------------------------

void OtrMessaging::continueSMP(const QString& account, const QString& jid,
                               const QString& secret)
{
    m_impl->continueSMP(account, jid, secret);
}

//-----------------------------------------------------------------------------

void OtrMessaging::abortSMP(const QString& account, const QString& jid)
{
    m_impl->abortSMP(account, jid);
}

//-----------------------------------------------------------------------------

OtrMessageState OtrMessaging::getMessageState(const QString& account,
                                              const QString& contact)
{
    return m_impl->getMessageState(account, contact);
}

//-----------------------------------------------------------------------------

QString OtrMessaging::getMessageStateString(const QString& account,
                                            const QString& contact)
{
    return m_impl->getMessageStateString(account, contact);
}

//-----------------------------------------------------------------------------

QString OtrMessaging::getSessionId(const QString& account,
                                   const QString& contact)
{
    return m_impl->getSessionId(account, contact);
}

//-----------------------------------------------------------------------------

psiotr::Fingerprint OtrMessaging::getActiveFingerprint(const QString& account,
                                                       const QString& contact)
{
    return m_impl->getActiveFingerprint(account, contact);
}

//-----------------------------------------------------------------------------

bool OtrMessaging::isVerified(const QString& account, const QString& contact)
{
    return m_impl->isVerified(account, contact);
}

//-----------------------------------------------------------------------------

bool OtrMessaging::smpSucceeded(const QString& account, const QString& contact)
{
    return m_impl->smpSucceeded(account, contact);
}

//-----------------------------------------------------------------------------

void OtrMessaging::setPolicy(psiotr::OtrPolicy policy)
{
    m_otrPolicy = policy;
}

//-----------------------------------------------------------------------------

OtrPolicy OtrMessaging::getPolicy()
{
    return m_otrPolicy;
}

//-----------------------------------------------------------------------------

void OtrMessaging::generateKey(const QString& account)
{
    m_impl->generateKey(account);
}

//-----------------------------------------------------------------------------

QString OtrMessaging::humanAccount(const QString accountId)
{
    return m_callback->humanAccount(accountId);
}

//-----------------------------------------------------------------------------

} // namespace psiotr
