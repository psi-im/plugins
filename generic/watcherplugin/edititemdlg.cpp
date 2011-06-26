/*
 * edititemdlg.cpp - plugin
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

#include "edititemdlg.h"
#include <QFileDialog>

const QString splitStr = "&split&";

EditItemDlg::EditItemDlg(IconFactoryAccessingHost* icoHost, OptionAccessingHost *psiOptions_, QWidget *p)
	: QDialog(p, Qt::Window)
	, psiOptions(psiOptions_)
{
	setAttribute(Qt::WA_DeleteOnClose);
	//setModal(true);
	ui_.setupUi(this);
	ui_.tb_open->setIcon(icoHost->getIcon("psi/browse"));
	ui_.tb_test->setIcon(icoHost->getIcon("psi/play"));

	connect(ui_.tb_test, SIGNAL(pressed()), SLOT(doTestSound()));
	connect(ui_.tb_open, SIGNAL(pressed()), SLOT(getFileName()));
}

void EditItemDlg::init(const QString &settings)
{
	QStringList l = settings.split(splitStr);
	if(!l.isEmpty()) {
		ui_.le_jid->setText(l.takeFirst());
		ui_.le_jid->setEnabled(!ui_.le_jid->text().isEmpty());
		ui_.rb_text->setChecked(ui_.le_jid->text().isEmpty());
	}
	if(!l.isEmpty()) {
		ui_.te_text->setText(l.takeFirst());
		ui_.te_text->setEnabled(!ui_.te_text->toPlainText().isEmpty());
		ui_.rb_jid->setChecked(ui_.te_text->toPlainText().isEmpty());
	}
	if(!l.isEmpty())
		ui_.le_sound->setText(l.takeFirst());
	if(!l.isEmpty())
		ui_.cb_always_play->setChecked(l.takeFirst().toInt());
	if(!l.isEmpty())
		ui_.rb_groupchat->setChecked(l.takeFirst().toInt());

}

void EditItemDlg::getFileName()
{
	QString fileName = QFileDialog::getOpenFileName(0,tr("Choose a sound file"),
							psiOptions->getPluginOption(constLastFile, QVariant("")).toString(),
							tr("Sound (*.wav)"));
	if(fileName.isEmpty()) return;
	QFileInfo fi(fileName);
	psiOptions->setPluginOption(constLastFile, QVariant(fi.absolutePath()));
	ui_.le_sound->setText(fileName);
}

void EditItemDlg::doTestSound()
{
	emit testSound(ui_.le_sound->text());
}

void EditItemDlg::accept()
{
	QString str = (ui_.rb_jid->isChecked() ? ui_.le_jid->text() : "") + splitStr;
	str += (ui_.rb_text->isChecked() ? ui_.te_text->toPlainText() : "") + splitStr;
	str += ui_.le_sound->text() + splitStr;
	str += (ui_.cb_always_play->isChecked() ? "1" : "0") + splitStr;
	str += ui_.rb_groupchat->isChecked() ? "1" : "0";
	emit dlgAccepted(str);
	close();
}
