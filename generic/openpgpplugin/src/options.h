/*
 * options.h - plugin widget
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
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

#include <QStandardItemModel>
#include <QWidget>

class Model;
class GPGProc;
class OptionAccessingHost;
class AccountInfoAccessingHost;
class PsiAccountControllingHost;

namespace Ui {
class Options;
}

class Options : public QWidget {
    Q_OBJECT

public:
    explicit Options(QWidget *parent = nullptr);
    ~Options();

    void setOptionAccessingHost(OptionAccessingHost *host);
    void setAccountInfoAccessingHost(AccountInfoAccessingHost *host);
    void setPsiAccountControllingHost(PsiAccountControllingHost *host);

    void loadSettings();
    void saveSettings();

public slots:
    void addKey();
    void deleteKey();

    void importKeyFromFile();
    void importKeyFromClipboard();

    void exportKeyToFile();
    void exportKeyToClipboard();

    void showInfo();
    void updateAllKeys();
    void allKeysTableModelUpdated();

private slots:
    void updateAccountsList();
    void updateKnownKeys();
    void updateOwnKeys();
    void deleteKnownKey();
    void deleteOwnKey();
    void chooseKey();
    void copyKnownFingerprint();
    void copyOwnFingerprint();
    void contextMenuKnownKeys(const QPoint &pos);
    void contextMenuOwnKeys(const QPoint &pos);
    void openGpgAgentConfig();
    void loadGpgAgentConfigData();
    void updateGpgAgentConfig(const int pwdExpirationTime);

private:
    void copyFingerprintFromTable(QStandardItemModel *tableModel, const QModelIndexList &indexesList, const int column);

private:
    Ui::Options *              m_ui                  = nullptr;
    GPGProc *                  m_gpgProc             = nullptr;
    OptionAccessingHost *      m_optionHost          = nullptr;
    AccountInfoAccessingHost * m_accountInfo         = nullptr;
    PsiAccountControllingHost *m_accountHost         = nullptr;
    Model *                    m_allKeysTableModel   = nullptr;
    QStandardItemModel *       m_knownKeysTableModel = nullptr;
    QStandardItemModel *       m_ownKeysTableModel   = nullptr;
};
