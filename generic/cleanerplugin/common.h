/*
 * common.h - plugin
 * Copyright (C) 2009-2010  Evgeny Khryukin
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

#ifndef COMMON_H
#define COMMON_H



#include <QDialog>
#include "ui_clearingtab.h"


class vCardView : public QDialog
{
    Q_OBJECT

public:
    vCardView(const QString& filename, QWidget *parent = nullptr);

};



class HistoryView : public QDialog
{
    Q_OBJECT

public:
    HistoryView(const QString& file, QWidget *parent = nullptr);

};




class ClearingTab : public QWidget, public Ui::ClearingTab
{
    Q_OBJECT

public:
    ClearingTab(QWidget * parent = nullptr) : QWidget(parent) { setupUi(this); };
};



class AvatarView : public QDialog
{
    Q_OBJECT

public:
        AvatarView(const QPixmap &pix, QWidget *parent = nullptr);
    void setIcon(const QIcon&);

private:
        QPixmap pix_;
    QPushButton *pbSave;

private slots:
        void save();

};

#endif // COMMON_H
