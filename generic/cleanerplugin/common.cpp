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
//--------------ChooseProfile------------------
//---------------------------------------------
ChooseProfile::ChooseProfile(QString profDir, QWidget *parent)
        : QDialog(parent) {

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Choose profile"));
    QHBoxLayout *l = new QHBoxLayout(this);
    combo = new QComboBox();
    combo->setMinimumWidth(250);
    QDir Dir(profDir);
    foreach(QString dir, Dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        combo->addItem(dir);
    }
    tmpDir = combo->currentText();
    QPushButton *ok = new QPushButton(tr("OK"));
    QPushButton *cancel = new QPushButton(tr("Cancel"));
    l->addWidget(combo);
    l->addWidget(ok);
    l->addWidget(cancel);
    l->addStretch();
    connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(profileChanged(int)));
    connect(ok, SIGNAL(released()), this, SLOT(pressOk()));
    connect(cancel, SIGNAL(released()), this, SLOT(close()));
    adjustSize();
    setFixedSize(width(), height());
}

void ChooseProfile::profileChanged(int i) {
    tmpDir = combo->itemText(i);
}

void ChooseProfile::pressOk() {
    emit changeProfile(tmpDir);
    close();
}


//---------------------------------------------
//--------------HistoryView--------------------
//---------------------------------------------
HistoryView::HistoryView(QString filename, QWidget *parent)
        :QDialog(parent, Qt::Window) {

        setAttribute(Qt::WA_DeleteOnClose);
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");
        QTextCodec::setCodecForLocale(codec);
         QFile file(filename);
         if(file.open(QIODevice::ReadOnly)) {
             setWindowTitle(filename.split(QDir::separator()).takeLast());
             QVBoxLayout *layout = new QVBoxLayout(this);
             QTextEdit *textWid = new QTextEdit();
             QString text;
             QTextStream in(&file);
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
     }



//---------------------------------------------
//----------------vCardView--------------------
//---------------------------------------------
vCardView::vCardView(QString filename, QWidget *parent)
        :QDialog(parent, Qt::Window) {

        setAttribute(Qt::WA_DeleteOnClose);
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");
        QTextCodec::setCodecForLocale(codec);
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
    Save = new QPushButton;
    Save->setFixedSize(25,25);
    Save->setToolTip(tr("Save Image"));
    layout->addWidget(Save);
    layout->addWidget(pixLabel);
    connect(Save, SIGNAL(released()), this, SLOT(save()));
    adjustSize();
}

void AvatarView::setIcon(QIcon ico)
{
    Save->setIcon(ico);
}

void AvatarView::save()
{
    QFileDialog *dialog = new QFileDialog(this);
    dialog->setModal(true);
    QString filename = dialog->getSaveFileName(this, tr("Save Avatar"),"", tr("Images (*.png *.gif *.jpg *.jpeg)"));
    QImage image = pix_.toImage();
    image.save(filename);
}


