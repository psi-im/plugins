/*
 * captchadialog.h - plugin
 * Copyright (C) 2010  Khryukin Evgeny
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

#ifndef CAPTCHADIALOG_H
#define CAPTCHADIALOG_H

#include "ui_captchadialog.h"


class CaptchaDialog : public QDialog
{
	Q_OBJECT
public:
	CaptchaDialog(QString id, QWidget *p = 0);
	void setPixmap(const QPixmap &pix);
	void setQuestion(const QString &quest);
	void setBody(const QString &body);
	void setText(const QString& text);

private slots:
	void okPressed();
	void cancelPressed();
	void toggleTEVisible(bool);

signals:
	void ok(QString, QString);
	void cancel(QString);

private:
	Ui::CaptchaDialog ui_;
	QString id_;

};

#endif // CAPTCHADIALOG_H
