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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <QWidget>
#include <QStandardItemModel>

class GPGProc;
class OptionAccessingHost;

namespace Ui {
class Options;
}

class Options : public QWidget {
    Q_OBJECT

public:
    explicit Options(QWidget *parent = nullptr);
    ~Options();

    void setOptionAccessingHost(OptionAccessingHost *host);

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
    void updateOwnKeys();

private slots:
    void deleteOwnKey();
    void copyOwnFingerprint();
    void contextMenu(const QPoint &pos);

private:
    Ui::Options         *m_ui = nullptr;
    GPGProc             *m_gpgProc = nullptr;
    OptionAccessingHost *m_optionHost = nullptr;
    QStandardItemModel  *m_ownKeysTableModel = nullptr;
};

#endif // OPTIONS_H
