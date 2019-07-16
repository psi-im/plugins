/*
 * models.cpp - plugin
 * Copyright (C) 2009-2010  Evgeny Khryukin
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

#include "models.h"
#include "optionsparser.h"

#include <QDateTime>
#include <QDir>
#include <QPixmap>


//----------------------------------------
//--BaseModel-----------------------------
//----------------------------------------
void BaseModel::reset()
{
    selected_.clear();
    beginResetModel();
    endResetModel();
}

int BaseModel::selectedCount(const QModelIndex &) const
{
    return selected_.size();
}

void BaseModel::selectAll(const QModelIndexList& list)
{
    emit layoutAboutToBeChanged();
    selected_.clear();
    selected_ = list.toSet();
    emit updateLabel(0);
    emit layoutChanged();
}

void BaseModel::unselectAll()
{
    emit layoutAboutToBeChanged();
    selected_.clear();
    emit updateLabel(0);
    emit layoutChanged();
}

bool BaseModel::isSelected(const QModelIndex &index) const
{
    return selected_.contains(index);
}

Qt::ItemFlags BaseModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    if(index.column() == 0)
        flags |= Qt::ItemIsUserCheckable;

    return flags;
}

bool BaseModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if(!index.isValid() || role != Qt::EditRole || index.column() != 0)
        return false;

    switch(value.toInt()) {
    case(0):
        if(selected_.contains(index))
            selected_.remove(index);
        break;
    case(2):
        if(!selected_.contains(index))
            selected_ << index;
        break;
    case(3):
        if(selected_.contains(index))
            selected_.remove(index);
        else
            selected_ << index;
        break;
    }

    emit dataChanged(index, index);
    emit updateLabel(0);

    return true;
}

QVariant BaseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            return headers.at(section);
        }
        else {
            return section+1;
        }
    } else
        return QVariant();
}

int BaseModel::columnCount(const QModelIndex & /*parent*/) const
{
    return headers.size();
}


//---------------------------------------
//------BaseFileModel--------------------
//---------------------------------------
void BaseFileModel::reset()
{
    files_.clear();
    BaseModel::reset();
}


int BaseFileModel::rowCount(const QModelIndex &/* parent*/) const
{
    return files_.size();
}

QString BaseFileModel::fileName(const QModelIndex & index) const
{
    if(!index.isValid())
        return QString();

    int i = index.row();
    if(i < 0 || i >= files_.size())
        return QString();

    QString name = files_.at(i);
    return name.split("/", QString::SkipEmptyParts).last();
}

QString BaseFileModel::filePass(const QModelIndex & index) const
{
    if(!index.isValid())
        return QString();

    int i = index.row();
    if(i < 0 || i >= files_.size())
        return QString();

    return files_.at(i);
}

int BaseFileModel::fileSize(const QModelIndex & index) const
{
    if(!index.isValid())
        return 0;

    QFile file(filePass(index));

    return int(file.size());
}

QString BaseFileModel::fileDate(const QModelIndex & index) const
{
    QString result;
    if(!index.isValid())
        return result;
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    result = QFileInfo(filePass(index)).created().toString("yyyy-MM-dd");
#else
    result = QFileInfo(filePass(index)).birthTime().toString("yyyy-MM-dd");
#endif
    return result;
}

void BaseFileModel::deleteSelected()
{
    emit layoutAboutToBeChanged ();

    foreach(const QModelIndex& index, selected_) {
        const QString fileName = filePass(index);
        if(fileName.isEmpty())
            continue;

        QFile file(fileName);
        if(file.open(QIODevice::ReadWrite)) {
            file.remove();
        }
    }

    setDirs(dirs_);

    emit updateLabel(0);
}

void BaseFileModel::setDirs(const QStringList& dirs)
{
    reset();
    dirs_ = dirs;
    foreach(const QString& dirName, dirs_) {
        QDir Dir(dirName);
        foreach(const QString& fileName, Dir.entryList(QDir::Files)) {
            files_.append(Dir.absoluteFilePath(fileName));
        }
    }

    emit layoutChanged();
}


//----------------------------------------
//--ClearingModel-------------------------
//----------------------------------------
ClearingModel::ClearingModel(const QString& dir, QObject* parent)
    : BaseFileModel(parent)
{
    headers << tr("")
            << tr("Nick")
            << tr("Domain")
            << tr("Size")
            << tr("Creation Date");

    setDirs({dir});
}

QVariant ClearingModel::data(const QModelIndex & index, int role) const
{
    if(!index.isValid())
        return QVariant();

    int i = index.column();
    QString filename = fileName(index);
    filename = filename.replace("%5f", "_");
    filename = filename.replace("%2d", "-");
    filename = filename.replace("%25", "@");
    switch(i)
    {
    case(0):
        if (role == Qt::CheckStateRole) {
            return isSelected(index) ? 2:0;
        } else if (role == Qt::TextAlignmentRole) {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole)
            return QVariant();
        break;
    case(1):
        if (role == Qt::TextAlignmentRole) {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole) {
            if(filename.contains("_at_"))
                return QVariant(filename.split("_at_").first());
            else
                return QVariant();
        }
        break;
    case(2):
        if (role == Qt::TextAlignmentRole) {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole)
            return QVariant(filename.split("_at_").last());
        break;
    case(3):
        if (role == Qt::TextAlignmentRole) {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole)
            return QVariant(fileSize(index));
        break;
    case(4):
        if (role == Qt::TextAlignmentRole) {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole)
            return QVariant(fileDate(index));
        break;
    }
    return QVariant();
}

//----------------------------------------
//--ClearingVcardModel--------------------
//----------------------------------------
QVariant ClearingVcardModel::data(const QModelIndex & index, int role) const
{
    if(index.column() == 2) {
        if (role == Qt::DisplayRole) {
            QString domain = fileName(index).split("_at_").last();
            domain.chop(4);
            domain = domain.replace("%5f", "_");
            domain = domain.replace("%2d", "-");
            domain = domain.replace("%25", "@");
            return QVariant(domain);
        }
    }
    return ClearingModel::data(index, role);
}


//----------------------------------------
//--ClearingHistoryModel------------------
//----------------------------------------
QVariant ClearingHistoryModel::data(const QModelIndex & index, int role) const
{
    QString filename = fileName(index);
    filename = filename.replace("%5f", "_");
    filename = filename.replace("%2d", "-");
    filename = filename.replace("%25", "@");
    if (role == Qt::DisplayRole) {
        if(index.column() == 2) {
            QString domain;
            if(filename.contains("_in_")){
                domain = filename.split("_in_").last();
                domain = domain.replace("_at_", "@");
            }
            else {
                domain = filename.split("_at_").last();
                domain.remove(".history");
            }
            return QVariant(domain);
        }
        else if(index.column() == 1) {
            QString jid;
            if(filename.contains("_in_")){
                jid = filename.split("_in_").first();
                jid = jid.replace("_at_", "@");
            }
            else {
                if(filename.contains("_at_"))
                    return QVariant(filename.split("_at_").first());
                else
                    return QVariant();
            }
        }
    }

    return ClearingModel::data(index, role);
}



//----------------------------------------
//--ClearingAvatarModel-------------------
//----------------------------------------
ClearingAvatarModel::ClearingAvatarModel(const QStringList& dir, QObject* parent)
    : BaseFileModel(parent)
{
    headers << tr("")
            << tr("Avatar")
            << tr("Size")
            << tr("Creation Date");

    setDirs(dir);
}

QVariant ClearingAvatarModel::data(const QModelIndex & index, int role) const
{
    if(!index.isValid())
        return QVariant();

    int i = index.column();
    QString filename = filePass(index);
    switch(i) {
    case(0):
        if (role == Qt::CheckStateRole) {
            return isSelected(index) ? 2:0;
        } else if (role == Qt::TextAlignmentRole) {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole)
            return QVariant();
        break;
    case(1):
        if (role == Qt::TextAlignmentRole) {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole) {
            QPixmap pix = QPixmap(filename);
            if(pix.isNull())
                return QVariant();
            return QVariant(pix);
        }
        break;
    case(2):
        if (role == Qt::TextAlignmentRole) {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole)
            return QVariant(fileSize(index));
        break;
    case(3):
        if (role == Qt::TextAlignmentRole) {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole)
            return QVariant(fileDate(index));
        break;
    }
    return QVariant();
}

//----------------------------------------
//--ClearingOptionsModel------------------
//----------------------------------------
ClearingOptionsModel::ClearingOptionsModel(const QString& fileName, QObject* parent)
    : BaseModel(parent)
    , fileName_(fileName)
{
    headers << tr("")
            << tr("Options")
            << tr("Values");

    parser_ = new OptionsParser(fileName_, this);
    options = parser_->getMissingNodesString();
}

int ClearingOptionsModel::rowCount(const QModelIndex &/* parent*/) const
{
    return options.size();
}

QVariant ClearingOptionsModel::data(const QModelIndex & index, int role) const
{
    if(!index.isValid())
        return QVariant();

    int i = index.column();
    switch(i) {
    case(0):
        if (role == Qt::CheckStateRole) {
            return isSelected(index) ? 2:0;
        } else if (role == Qt::TextAlignmentRole) {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole)
            return QVariant();
        break;
    case(1):
        if (role == Qt::TextAlignmentRole) {
            return int(Qt::AlignLeft | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole) {
            return QVariant(options.at(index.row()));
        }
        break;
    case(2):
        if (role == Qt::TextAlignmentRole) {
            return int(Qt::AlignLeft | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole) {
            QDomNode node = parser_->nodeByString(options.at(index.row()));
            return QVariant(node.toElement().text());
        }
        break;
    }
    return QVariant();
}

void ClearingOptionsModel::deleteSelected()
{
    emit layoutAboutToBeChanged ();

    setFile(fileName_);

    emit updateLabel(0);
}

void ClearingOptionsModel::reset()
{
    options.clear();
    BaseModel::reset();
}

void ClearingOptionsModel::setFile(const QString& fileName)
{
    emit layoutAboutToBeChanged ();

    reset();
    fileName_ = fileName;
    delete parser_;
    parser_ = new OptionsParser(fileName_, this);
    options = parser_->getMissingNodesString();

    emit layoutChanged();
}




//----------------------------------------
//--ClearingProxyModel--------------------
//----------------------------------------
ClearingProxyModel::ClearingProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

bool ClearingProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &parent) const
{
    QModelIndex index1 = sourceModel()->index(sourceRow, 1, parent);
    QModelIndex index2 = sourceModel()->index(sourceRow, 2, parent);
    bool b = index1.data(Qt::DisplayRole).toString().contains(filterRegExp());
    bool c = index2.data(Qt::DisplayRole).toString().contains(filterRegExp());
    return (b | c);
}
