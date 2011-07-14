/*
 * PsiOtrConfig.hpp - configuration dialogs for Psi OTR plugin
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef PSIOTRCONFIG_HPP_
#define PSIOTRCONFIG_HPP_

#include "OtrMessaging.hpp"

#include <QWidget>
#include <QModelIndex>

class OptionAccessingHost;
class QCheckBox;
class QStandardItemModel;
class QTableView;

// ---------------------------------------------------------------------------

namespace psiotr
{

// ---------------------------------------------------------------------------

const QString PSI_CONFIG_POLICY = "plugins.psi-otr.otr-policy";

// ---------------------------------------------------------------------------

/** 
* This dialog appears in the 'Plugins' section of the Psi configuration.
*/
class ConfigDialog : public QWidget
{
Q_OBJECT

public:
    ConfigDialog(OtrMessaging* otr, OptionAccessingHost* optionHost,
                 QWidget* parent = 0);

private:
    OtrMessaging*        m_otr;
    OptionAccessingHost* m_optionHost;
};

// ---------------------------------------------------------------------------

/** 
* Configure OTR policy.
*/
class ConfigOtrWidget : public QWidget
{
Q_OBJECT

public:
    ConfigOtrWidget(OptionAccessingHost* optionHost,
                    OtrMessaging* otr,
                    QWidget* parent = 0);

private:
    OptionAccessingHost* m_optionHost;
    OtrMessaging*        m_otr;

    QCheckBox*   m_polEnable;
    QCheckBox*   m_polAuto;
    QCheckBox*   m_polRequire;

private slots:
    void handlePolicyChange();
};

// ---------------------------------------------------------------------------

/** 
* Show fingerprint of your contacts.
*/
class FingerprintWidget : public QWidget
{
Q_OBJECT

public:
    FingerprintWidget(OtrMessaging* otr, QWidget* parent = 0);

protected:
    void updateData();

private:
    OtrMessaging*       m_otr;
    QTableView*         m_table;
    QStandardItemModel* m_tableModel;
    QModelIndex         m_selectIndex;
    QList<Fingerprint>  m_fingerprints;

private slots:
    void forgetFingerprint();
    void verifyFingerprint();
    void tableClicked(const QModelIndex& index);
};

// ---------------------------------------------------------------------------

/** 
* Display a table with account and fingerprint of private key.
*/
class PrivKeyWidget : public QWidget
{
Q_OBJECT

public:
    PrivKeyWidget(OtrMessaging* otr, QWidget* parent);

private:
    OtrMessaging* m_otr;
};

//-----------------------------------------------------------------------------

} // namespace psiotr

#endif
