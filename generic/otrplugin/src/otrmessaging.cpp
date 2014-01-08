/*
 * otrmessaging.cpp - Interface to libotr
 *
 * Off-the-Record Messaging plugin for Psi+
 * Copyright (C) 2007-2011  Timo Engel (timo-e@freenet.de)
 *                    2011  Florian Fieber
 *                    2014  Boris Pek (tehnick-8@mail.ru)
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "otrmessaging.h"
#include "otrinternal.h"

#include <QString>
#include <QList>
#include <QHash>

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
      trust(fp.trust)
{

}

Fingerprint::Fingerprint(unsigned char* fingerprint,
                         QString account, QString username,
                         QString trust)
    : fingerprint(fingerprint),
      account(account),
      username(username),
      trust(trust)
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

QString OtrMessaging::encryptMessage(const QString& account,
                                     const QString& contact,
                                     const QString& message)
{
    return m_impl->encryptMessage(account, contact, message);
}

//-----------------------------------------------------------------------------

OtrMessageType OtrMessaging::decryptMessage(const QString& account,
                                            const QString& contact,
                                            const QString& message,
                                            QString& decrypted)
{
    return m_impl->decryptMessage(account, contact, message, decrypted);
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

void OtrMessaging::startSession(const QString& account, const QString& contact)
{
    m_impl->startSession(account, contact);
}

//-----------------------------------------------------------------------------

void OtrMessaging::endSession(const QString& account, const QString& contact)
{
    m_impl->endSession(account, contact);
}

//-----------------------------------------------------------------------------

void OtrMessaging::expireSession(const QString& account, const QString& contact)
{
    m_impl->expireSession(account, contact);
}

//-----------------------------------------------------------------------------

void OtrMessaging::startSMP(const QString& account, const QString& contact,
                            const QString& question, const QString& secret)
{
    m_impl->startSMP(account, contact, question, secret);
}

//-----------------------------------------------------------------------------

void OtrMessaging::continueSMP(const QString& account, const QString& contact,
                               const QString& secret)
{
    m_impl->continueSMP(account, contact, secret);
}

//-----------------------------------------------------------------------------

void OtrMessaging::abortSMP(const QString& account, const QString& contact)
{
    m_impl->abortSMP(account, contact);
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

bool OtrMessaging::displayOtrMessage(const QString& account,
                                     const QString& contact,
                                     const QString& message)
{
    return m_callback->displayOtrMessage(account, contact, message);
}

//-----------------------------------------------------------------------------

void OtrMessaging::stateChange(const QString& account, const QString& contact,
                               OtrStateChange change)
{
    return m_callback->stateChange(account, contact, change);
}

//-----------------------------------------------------------------------------

QString OtrMessaging::humanAccount(const QString& accountId)
{
    return m_callback->humanAccount(accountId);
}

//-----------------------------------------------------------------------------

QString OtrMessaging::humanContact(const QString& accountId,
                                   const QString& contact)
{
    return m_callback->humanContact(accountId, contact);
}

//-----------------------------------------------------------------------------

} // namespace psiotr
