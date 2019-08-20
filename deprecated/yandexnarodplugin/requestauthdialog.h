/*
 * requestauthdialog.h - plugin
 * Copyright (C) 2008  Alexander Kazarin <boiler@co.ru>
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

#ifndef REQUESTAUTHDIALOG_H
#define REQUESTAUTHDIALOG_H

#include "ui_requestauthdialog.h"

#include <QNetworkCookie>

class QNetworkAccessManager;
class QNetworkReply;

class requestAuthDialog : public QDialog {
    Q_OBJECT;

public:
    requestAuthDialog(QWidget *parent = 0);
    ~requestAuthDialog();
    void setLogin(const QString& login) { ui.editLogin->setText(login); ui.editPasswd->setFocus(); }
    void setPasswd(const QString& passwd) { ui.editPasswd->setText(passwd); ui.editPasswd->setFocus(); }
    QString getLogin() const { return ui.editLogin->text(); }
    QString getPasswd() const { return ui.editPasswd->text(); }
    bool getRemember() const { return ui.cbRemember->isChecked(); }
    QString getCode() const { return ui.editCaptcha->text(); }
    void setCaptcha(const QList<QNetworkCookie>& cooks, const QString& url);

private slots:
    void reply(QNetworkReply* r);

private:
    Ui::requestAuthDialogClass ui;
    QNetworkAccessManager *manager_;

};

#endif // REQUESTAUTHDIALOG_H
