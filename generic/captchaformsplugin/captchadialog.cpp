/*
 * captchadialog.cpp - plugin
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

#include "captchadialog.h"

CaptchaDialog::CaptchaDialog(QString id, QWidget *p)
	: QDialog(p)
	, id_(id)
{
	setAttribute(Qt::WA_DeleteOnClose);
	ui_.setupUi(this);
	toggleTEVisible(false);

	connect(ui_.buttonBox, SIGNAL(accepted()), SLOT(okPressed()));
	connect(ui_.buttonBox, SIGNAL(rejected()), SLOT(cancelPressed()));
	connect(ui_.cb_message, SIGNAL(toggled(bool)), SLOT(toggleTEVisible(bool)));
}

void CaptchaDialog::setQuestion(const QString &quest)
{
	ui_.lb_question->setText(quest);
	adjustSize();
}

void CaptchaDialog::toggleTEVisible(bool b)
{
	ui_.textEdit->setVisible(b);
	adjustSize();
}

void CaptchaDialog::setPixmap(const QPixmap &pix)
{
	ui_.lb_image->setText("");
	ui_.lb_image->setFixedSize(pix.size());
	ui_.lb_image->setPixmap(pix);
	adjustSize();
}

void CaptchaDialog::setText(const QString &text)
{
	ui_.lb_image->setText(text);
	adjustSize();
}

void CaptchaDialog::setBody(const QString &body)
{
	ui_.textEdit->setPlainText(body);
}

void CaptchaDialog::okPressed()
{
	QString text = ui_.le_answer->text();
	if(text.isEmpty())
		emit cancel(id_);
	else
		emit ok(id_, text);

	close();
}
void CaptchaDialog::cancelPressed()
{
	emit cancel(id_);
	close();
}
