/*
 * common.cpp - plugin
 * Copyright (C) 2009-2010  Khryukin Evgeny
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

#include "common.h"
#include <QDir>
#include <QtGui>
#include <QDomElement>



//---------------------------------------------
//--------------HistoryView--------------------
//---------------------------------------------
HistoryView::HistoryView(const QString& filename, QWidget *parent)
	:QDialog(parent, Qt::Window)
{

        setAttribute(Qt::WA_DeleteOnClose);
	QFile file(filename);
	if(file.open(QIODevice::ReadOnly)) {
		setWindowTitle(filename.split(QDir::separator()).takeLast());
		QVBoxLayout *layout = new QVBoxLayout(this);
		QTextEdit *textWid = new QTextEdit();
		QString text;
		QTextStream in(&file);
		in.setCodec("UTF-8");
		text = in.readAll();
		textWid->setText(text);
		QTextCursor cur = textWid->textCursor();
		cur.setPosition(text.length());
		textWid->setTextCursor(cur);
		layout->addWidget(textWid);
		QPushButton *Close = new QPushButton(tr("Close"));
		QHBoxLayout *butLayout = new QHBoxLayout();
		butLayout->addStretch();
		butLayout->addWidget(Close);
		butLayout->addStretch();
		layout->addLayout(butLayout);
		connect(Close, SIGNAL(released()), this, SLOT(close()));
		resize(800, 500);
		show();
	}
	else
		close();
}



//---------------------------------------------
//----------------vCardView--------------------
//---------------------------------------------
vCardView::vCardView(const QString& filename, QWidget *parent)
	:QDialog(parent, Qt::Window)
{

        setAttribute(Qt::WA_DeleteOnClose);
        QFile file(filename);
        if(file.open(QIODevice::ReadOnly)) {
		setWindowTitle(filename.split(QDir::separator()).takeLast());
		QVBoxLayout *layout = new QVBoxLayout(this);
		QGridLayout *main = new QGridLayout;
		QLineEdit *name = new QLineEdit;
		QLineEdit *nick = new QLineEdit;
		QLineEdit *birth = new QLineEdit;
		QLineEdit *email = new QLineEdit;
		main->addWidget(new QLabel(tr("Full Name:")),0,0);
		main->addWidget(name, 0, 1);
		main->addWidget(new QLabel(tr("Nick:")), 1, 0);
		main->addWidget(nick, 1, 1);
		main->addWidget(new QLabel(tr("Birthday:")), 2, 0);
		main->addWidget(birth, 2, 1);
		main->addWidget(new QLabel(tr("E-Mail:")), 3, 0);
		main->addWidget(email, 3, 1);
		QTextStream in(&file);
		in.setCodec("UTF-8");
		QDomDocument text;
		text.setContent(in.readAll());
		QDomElement VCard = text.documentElement();
		nick->setText(VCard.firstChildElement("NICKNAME").text());
		QString Name = VCard.firstChildElement("FN").text();
		if(Name.isEmpty()) {
			QDomElement n = VCard.firstChildElement("N");
			Name = n.firstChildElement("FAMILY").text() + " " + n.firstChildElement("GIVEN").text();
		}
		name->setText(Name);
		birth->setText(VCard.firstChildElement("BDAY").text());
		email->setText(VCard.firstChildElement("EMAIL").firstChildElement("USERID").text());


		QPushButton *Close = new QPushButton(tr("Close"));
		QHBoxLayout *butLayout = new QHBoxLayout();

		layout->addLayout(main);
		butLayout->addStretch();
		butLayout->addWidget(Close);
		butLayout->addStretch();
		layout->addLayout(butLayout);
		connect(Close, SIGNAL(released()), this, SLOT(close()));
		setFixedSize(400,200);
		show();
	}
	else
		close();
}



//---------------------------------------------
//----------------AvatarView-------------------
//---------------------------------------------
AvatarView::AvatarView( const QPixmap &pix, QWidget *parent)
        : QDialog(parent)
        , pix_(pix)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(tr("Avatar"));
	QVBoxLayout *layout = new QVBoxLayout(this);
	QLabel *pixLabel = new QLabel;
	pixLabel->setPixmap(pix);
	pbSave = new QPushButton;
	pbSave->setFixedSize(25,25);
	pbSave->setToolTip(tr("Save Image"));
	layout->addWidget(pbSave);
	layout->addWidget(pixLabel);
	connect(pbSave, SIGNAL(released()), this, SLOT(save()));
	adjustSize();
}

void AvatarView::setIcon(const QIcon& ico)
{
	pbSave->setIcon(ico);
}

void AvatarView::save()
{
	QFileDialog dialog(this);
	dialog.setModal(true);
	QString filename = dialog.getSaveFileName(this, tr("Save Avatar"),"", tr("Images (*.png *.gif *.jpg *.jpeg)"));
	if(filename.isEmpty())
		return;

	QImage image = pix_.toImage();
	image.save(filename);
}
