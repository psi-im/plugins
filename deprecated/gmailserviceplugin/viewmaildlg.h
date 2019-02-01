/*
 * viewmaildlg.h - plugin
 * Copyright (C) 2011 Evgeny Khryukin
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef VIEWMAILDLG_H
#define VIEWMAILDLG_H

#include "ui_viewmaildlg.h"
#include "iconfactoryaccessinghost.h"

struct MailItem {
    QString account;
    QString from;
    QString subject;
    QString text;
    QString url;
};

class ViewMailDlg : public QDialog
{
    Q_OBJECT
public:
    ViewMailDlg(const QList<MailItem> &l, IconFactoryAccessingHost* host, QWidget *p = 0);
    ~ViewMailDlg() {}

    void appendItems(QList<MailItem> l);
    QString caption() const;

    static QString mailItemToText(const MailItem& mi);

private slots:
    void updateButtons();
    void showNext();
    void showPrev();
    void browse();
    void anchorClicked(const QUrl& url);

private:
    void showItem(int num);
    void updateCaption();

protected:
    void wheelEvent(QWheelEvent *e);

private:
    Ui::ViewMailDlg ui_;
    QList<MailItem> items_;
    int currentItem_;
};

#endif // VIEWMAILDLG_H
