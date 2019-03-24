/*
 * edititemdlg.h - plugin
 * Copyright (C) 2010  Evgeny Khryukin
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef EDITITEMDLG_H
#define EDITITEMDLG_H

#include "ui_edititemdlg.h"

class OptionAccessingHost;
class IconFactoryAccessingHost;

#define constLastFile "lastfile"

class EditItemDlg : public QDialog
{
    Q_OBJECT
public:
    EditItemDlg(IconFactoryAccessingHost* icoHost, OptionAccessingHost *psiOptions_, QWidget *p = 0);
    void init(const QString &settings);

signals:
    void dlgAccepted(const QString&);
    void testSound(const QString&);

private slots:
    void accept();
    void getFileName();
    void doTestSound();

private:
    Ui::EditItemDlg ui_;
    OptionAccessingHost *psiOptions;
};

#endif // EDITITEMDLG_H
