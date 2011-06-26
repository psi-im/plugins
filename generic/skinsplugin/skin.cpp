/*
 * skin.cpp - plugin
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

#include "skin.h"
#include <QDir>
#include <QMessageBox>
#include <QDomDocument>

void Skin::setFile(QString file)
{
    filePass_ = file;
}

QString Skin::filePass()
{
    return filePass_;
}

QString Skin::skinFolder()
{
    QString folder = filePass_;
    int index = folder.lastIndexOf("/");
    folder.chop(folder.size() - index);
    return folder;
}

QPixmap Skin::previewPixmap()
{
    QDir dir(skinFolder());
    QString skinName = name();
    QPixmap pix = QPixmap();
    foreach(QString fileName, dir.entryList(QDir::Files)) {
        if((fileName.endsWith(".png", Qt::CaseInsensitive) || fileName.endsWith(".jpg", Qt::CaseInsensitive))
            && skinName.left(skinName.length()-4) == fileName.left(fileName.length()-4)) {
            pix = QPixmap(dir.absolutePath() + "/" + fileName);
            break;
        }
    }
    return pix;
}

QString Skin::name()
{
    QString name = filePass_;
    int index = name.lastIndexOf("/");
    index = name.size() - index - 1;
    name = name.right(index);
    return name;
}



//---------------------------------
//---Previewer---------------------
//---------------------------------
Previewer::Previewer(Skin *skin, QWidget *parent)
        : QDialog(parent)
        , skin_(skin)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    ui_.setupUi(this);    

    connect(ui_.pb_close, SIGNAL(released()), this, SLOT(close()));
    connect(ui_.pb_apply, SIGNAL(released()), this, SIGNAL(applySkin()));

}

bool Previewer::loadSkinInformation()
{
    QFile file(skin_->filePass());
    QDomDocument doc;
    if(!doc.setContent(&file)) {
        QMessageBox::warning(this, tr("Preview Skin"), tr("Skin is not valid!"));
        return false;
    }
    QDomElement elem = doc.documentElement();
    if(elem.tagName() != "skin") {        
        QMessageBox::warning(this, tr("Preview Skin"), tr("Skin is not valid!"));
        return false;
    }
    ui_.lbl_author->setText( elem.attribute("author") );
    ui_.lbl_version->setText( elem.attribute("version") );
    ui_.lbl_name->setText( elem.attribute("name") );
    QPixmap pix = skin_->previewPixmap();
    if(!pix.isNull())
        ui_.lbl_preview->setPixmap(pix);

    return true;
}




//-----------------------------
//------GetSkinName------------
//-----------------------------
GetSkinName::GetSkinName(QString name, QString author, QString version, QWidget *parent)
        :QDialog(parent)
{    
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    ui_.setupUi(this);

    connect(ui_.pb_cancel, SIGNAL(released()), this, SLOT(close()));
    connect(ui_.pb_ok, SIGNAL(released()), this, SLOT(okPressed()));

    ui_.le_name->setText(name);
    ui_.le_author->setText(author);
    ui_.le_version->setText(version);
}

void GetSkinName::okPressed()
{
    emit ok(ui_.le_name->text(), ui_.le_author->text(), ui_.le_version->text());
    close();
}

