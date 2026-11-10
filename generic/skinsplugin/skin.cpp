/*
 * skin.cpp - plugin
 * Copyright (C) 2010  Evgeny Khryukin
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

#include "skin.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

Skin::Skin(QTreeWidget *parent) : QTreeWidgetItem(parent)
{
    setTextAlignment(2, Qt::AlignHCenter);
}

Skin::Skin(QTreeWidgetItem *parent) : QTreeWidgetItem(parent)
{
    setTextAlignment(2, Qt::AlignHCenter);
}

void Skin::setFile(const QString &file)
{
    filePass_ = file;
    loadMetadata();
}

QString Skin::filePass() const { return filePass_; }

QString Skin::skinFolder() const
{
    return QFileInfo(filePass_).absolutePath();
}

QPixmap Skin::previewPixmap() const
{
    const QDir dir(skinFolder());
    const QString fileBaseName = QFileInfo(filePass_).completeBaseName();
    const QString skinName = name();
    const QStringList imagePatterns = { "*.png", "*.jpg", "*.jpeg" };
    const QStringList imageFiles = dir.entryList(imagePatterns, QDir::Files, QDir::Name);

    for (const QString &imageFile : imageFiles) {
        const QString imageBaseName = QFileInfo(imageFile).completeBaseName();
        if (imageBaseName.compare(fileBaseName, Qt::CaseInsensitive) == 0
            || imageBaseName.compare(skinName, Qt::CaseInsensitive) == 0) {
            QPixmap pixmap(dir.absoluteFilePath(imageFile));
            if (!pixmap.isNull())
                return pixmap;
        }
    }

    return QPixmap();
}

QString Skin::name() const
{
    if (!name_.isEmpty())
        return name_;

    return QFileInfo(filePass_).fileName();
}

QString Skin::author() const { return author_; }

QString Skin::version() const { return version_; }

bool Skin::loadMetadata()
{
    QFile file(filePass_);
    QDomDocument doc;
    if (!file.open(QIODevice::ReadOnly) || !doc.setContent(&file)) {
        name_ = QFileInfo(filePass_).fileName();
        author_.clear();
        version_.clear();
        setText(0, name_);
        setText(1, author_);
        setText(2, version_);
        return false;
    }

    QDomElement elem = doc.documentElement();
    if (elem.tagName() != "skin") {
        name_ = QFileInfo(filePass_).fileName();
        author_.clear();
        version_.clear();
        setText(0, name_);
        setText(1, author_);
        setText(2, version_);
        return false;
    }

    name_ = elem.attribute("name");
    author_ = elem.attribute("author");
    version_ = elem.attribute("version");
    if (name_.isEmpty())
        name_ = QFileInfo(filePass_).fileName();

    setText(0, name_);
    setText(1, author_);
    setText(2, version_);
    if (!author_.isEmpty() || !version_.isEmpty())
        setToolTip(0, QString("%1-%2\n%3").arg(name_, version_, filePass_));
    return true;
}

//---------------------------------
//---Previewer---------------------
//---------------------------------
Previewer::Previewer(Skin *skin, QWidget *parent) : QDialog(parent), skin_(skin)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    ui_.setupUi(this);

    connect(ui_.pb_close, &QPushButton::released, this, &Previewer::close);
    connect(ui_.pb_apply, &QPushButton::released, this, &Previewer::applySkin);
}

bool Previewer::loadSkinInformation()
{
    QFile file(skin_->filePass());
    QDomDocument doc;
    if (!file.open(QIODevice::ReadOnly) || !doc.setContent(&file)) {
        QMessageBox::warning(this, tr("Preview Skin"), tr("Skin is not valid!"));
        return false;
    }
    QDomElement elem = doc.documentElement();
    if (elem.tagName() != "skin") {
        QMessageBox::warning(this, tr("Preview Skin"), tr("Skin is not valid!"));
        return false;
    }
    ui_.lbl_author->setText(elem.attribute("author"));
    ui_.lbl_version->setText(elem.attribute("version"));
    ui_.lbl_name->setText(skin_->name());

    const QPixmap pix = skin_->previewPixmap();
    ui_.lbl_preview->setPixmap(QPixmap());
    if (pix.isNull()) {
        ui_.lbl_preview->setText(tr("Preview unavailable"));
    } else {
        ui_.lbl_preview->setText(QString());
        ui_.lbl_preview->setPixmap(pix);
    }

    return true;
}

//-----------------------------
//------GetSkinName------------
//-----------------------------
GetSkinName::GetSkinName(QString name, QString author, QString version, QWidget *parent) : QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setModal(true);
    ui_.setupUi(this);

    connect(ui_.pb_cancel, &QPushButton::released, this, &GetSkinName::close);
    connect(ui_.pb_ok, &QPushButton::released, this, &GetSkinName::okPressed);

    ui_.le_name->setText(name);
    ui_.le_author->setText(author);
    ui_.le_version->setText(version);
}

void GetSkinName::okPressed()
{
    emit ok(ui_.le_name->text(), ui_.le_author->text(), ui_.le_version->text());
    close();
}
