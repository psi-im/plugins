/*
 * viewmaildlg.cpp - plugin
 * Copyright (C) 2011 Khryukin Evgeny
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

#include "viewmaildlg.h"
#include <QDesktopServices>

static const QString mailBoxUrl = "https://mail.google.com/mail/#inbox";

ViewMailDlg::ViewMailDlg(QList<MailItem> l, IconFactoryAccessingHost* host, QWidget *p)
	: QDialog(p, Qt::Window)
	, items_(l)
	, currentItem_(-1)
{
	setAttribute(Qt::WA_DeleteOnClose);
	ui_.setupUi(this);
	ui_.tb_next->setIcon(host->getIcon("psi/arrowRight"));
	ui_.tb_prev->setIcon(host->getIcon("psi/arrowLeft"));

	connect(ui_.tb_next, SIGNAL(clicked()), SLOT(showNext()));
	connect(ui_.tb_prev, SIGNAL(clicked()), SLOT(showPrev()));
	connect(ui_.pb_browse, SIGNAL(clicked()), SLOT(browse()));
	connect(ui_.te_text, SIGNAL(anchorClicked(QUrl)), SLOT(anchorClicked(QUrl)));

	if(!items_.isEmpty()) {
		currentItem_ = 0;
		showItem(currentItem_);
	}
}


void ViewMailDlg::appendItems(QList<MailItem> l)
{
	items_.append(l);
	if(currentItem_ == -1) {
		currentItem_ = 0;
		showItem(currentItem_);
	}
	updateButtons();
}

void ViewMailDlg::updateButtons()
{
	if(items_.isEmpty()) {
		ui_.tb_next->setEnabled(false);
		ui_.tb_prev->setEnabled(false);
	}
	else {
		ui_.tb_prev->setEnabled(currentItem_ != 0);
		ui_.tb_next->setEnabled(items_.size()-1 > currentItem_);
	}
}

void ViewMailDlg::showNext()
{
	showItem(++currentItem_);
}

void ViewMailDlg::showPrev()
{
	showItem(--currentItem_);
}

void ViewMailDlg::showItem(int num)
{
	ui_.le_account->clear();
	ui_.le_from->clear();
	ui_.le_subject->clear();
	ui_.te_text->clear();

	if(num != -1
	   && !items_.isEmpty()
	   && num < items_.size()) {
		MailItem me = items_.at(num);
		ui_.le_account->setText(me.account);
		ui_.le_from->setText(me.from);
		ui_.le_subject->setText(me.subject);
		//FIXMI gmail отдает какой-то непонятный урл
		QRegExp re("th=([0-9]+)&");
		QString text = me.text;
		if(me.url.contains(re)) {
			QString url = mailBoxUrl + "/";
			QString tmp = re.cap(1);
			url += QString::number(tmp.toLongLong(), 16);
			text += QString("<br><br><a href=\"%1\">%2</a>").arg(url, tr("Open in browser"));
		}
		ui_.te_text->setHtml(text);
	}
	updateButtons();
}

void ViewMailDlg::browse()
{
	QDesktopServices::openUrl(QUrl(mailBoxUrl));
}

void ViewMailDlg::anchorClicked(const QUrl &url)
{
	if(!url.isEmpty()) {
		QDesktopServices::openUrl(url);
		if(items_.count() < 2) {
			close();
		}
	}
}
