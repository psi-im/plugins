/*
 * models.cpp - plugin
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

#include "models.h"
#include <QDateTime>
#include <QDir>
#include <QPixmap>

//----------------------------------------
//--ClearingModel-------------------------
//----------------------------------------
ClearingModel::ClearingModel(QString dir, QObject* parent)
        :QAbstractTableModel(parent)
 {
     headers << tr("")
             << tr("Nick")
             << tr("Domain")
             << tr("Size")
             << tr("Creation Date");

     dir_ = dir;
     QDir Dir(dir_);
     files = Dir.entryList(QDir::Files);
     selected.clear();
 }

QVariant ClearingModel::headerData ( int section, Qt::Orientation orientation, int role) const
 {
     if (role == Qt::DisplayRole) {
                if (orientation == Qt::Horizontal) {
                        return headers.at(section);
                } else {
                        return section+1;
                }
        } else
            return QVariant();
 }

Qt::ItemFlags ClearingModel::flags ( const QModelIndex & index ) const
 {
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

     if(index.column() == 0)
         flags |= Qt::ItemIsUserCheckable;

     return flags;
 }

int ClearingModel::columnCount(const QModelIndex & /*parent*/) const
 {
     return headers.size();
 }

int ClearingModel::rowCount(const QModelIndex &/* parent*/) const
 {
   return files.size();
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
     switch(i) {
             case(0):
                if (role == Qt::CheckStateRole) {
                        return selected.contains(fileName(index)) ? 2:0;
                } else if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole)
                        return QVariant("");
             case(1):
                if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole) {
                    if(filename.contains("_at_"))
                        return QVariant(filename.split("_at_").first());
                    else
                        return QVariant();
                }
             case(2):
                if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole)
                    return QVariant(filename.split("_at_").last());
             case(3):
                 if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole)
                    return QVariant(fileSize(index));
             case(4):
                 if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole)
                    return QVariant(fileDate(index));
        }
    return QVariant();
 }

QString ClearingModel::fileName(const QModelIndex & index) const
{
    if(!index.isValid())
        return QString();

    return files.at(index.row());
}

QString ClearingModel::filePass(const QModelIndex & index) const
{
    if(!index.isValid())
        return QString();

    return dir_ + QDir::separator() + files.at(index.row());
}

int ClearingModel::fileSize(const QModelIndex & index) const
{
    if(!index.isValid())
        return 0;

    QFile file(filePass(index));

    return file.size();
}

QString ClearingModel::fileDate(const QModelIndex & index) const
{
    if(!index.isValid())
        return QString();

    return QFileInfo(filePass(index)).created().toString("yyyy-MM-dd");
}

bool ClearingModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
   if(!index.isValid() || role != Qt::EditRole || index.column() != 0)
        return false;

    QString file = fileName(index);
    switch(value.toInt()) {
        case(0):
                if(selected.contains(file))
                    selected.remove(file);
                break;
        case(2):
                if(!selected.contains(file))
                    selected << file;
                break;
        case(3):
                if(selected.contains(file))
                    selected.remove(file);
                else
                    selected << file;
                break;
     }

    emit dataChanged(index, index);
    emit updateLabel(0);

    return false;
}

void ClearingModel::deleteSelected()
{
   emit layoutAboutToBeChanged ();

    foreach(QString fileName, selected) {
        QFile file(dir_+QDir::separator()+fileName);
        if(file.open(QIODevice::ReadWrite)) {
            file.remove();
            files.removeAt(files.indexOf(fileName));
        }
    }
    selected.clear();
    emit layoutChanged();
    emit updateLabel(0);
}

void ClearingModel::reset()
{
    selected.clear();
    files.clear();
    QAbstractTableModel::reset();
}

void ClearingModel::setDir(QString dir)
{
     dir_ = dir;
     QDir Dir(dir_);
     files = Dir.entryList(QDir::Files);
     emit layoutChanged();
}

int ClearingModel::selectedCount(const QModelIndex &) const
{
    return selected.size();
}

void ClearingModel::selectAll()
{
    selected.clear();
    selected = files.toSet();
    emit updateLabel(0);
    emit layoutChanged();
}

void ClearingModel::unselectAll()
{
    selected.clear();
    emit updateLabel(0);
    emit layoutChanged();
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



//----------------------------------------
//--ClearingAvatarModel-------------------
//----------------------------------------
ClearingAvatarModel::ClearingAvatarModel(QStringList dir, QObject* parent)
        :QAbstractTableModel(parent)
 {
     headers << tr("")
             << tr("Avatar")
             << tr("Size")
             << tr("Creation Date");

     dir_ = dir;
     foreach(QString dirName, dir_){
         QDir Dir(dirName);
         foreach(QString fileName, Dir.entryList(QDir::Files)) {
             files.append(Dir.absolutePath()+QDir::separator()+fileName);
         }
     }

     selected.clear();
 }

QVariant ClearingAvatarModel::headerData ( int section, Qt::Orientation orientation, int role) const
 {
     if (role == Qt::DisplayRole) {
                if (orientation == Qt::Horizontal) {
                        return headers.at(section);
                } else {
                        return section+1;
                }
        } else
            return QVariant();
 }

Qt::ItemFlags ClearingAvatarModel::flags ( const QModelIndex & index ) const
 {
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

     if(index.column() == 0)
         flags |= Qt::ItemIsUserCheckable;

     return flags;
 }

int ClearingAvatarModel::columnCount(const QModelIndex & /*parent*/) const
 {
     return headers.size();
 }

int ClearingAvatarModel::rowCount(const QModelIndex &/* parent*/) const
 {
   return files.size();
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
                        return selected.contains(filename) ? 2:0;
                } else if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole)
                        return QVariant("");
             case(1):
                if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole) {
                    QPixmap pix = QPixmap(filename);
                    if(pix.isNull())
                        return QVariant();
                    return QVariant(pix);
                }
             case(2):
                 if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole)
                    return QVariant(fileSize(index));
             case(3):
                 if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole)
                    return QVariant(fileDate(index));
        }
    return QVariant();
 }

QString ClearingAvatarModel::filePass(const QModelIndex & index) const
{
    if(!index.isValid())
        return QString();

    return files.at(index.row());
}

int ClearingAvatarModel::fileSize(const QModelIndex & index) const
{
    if(!index.isValid())
        return 0;

    QFile file(filePass(index));

    return file.size();
}

QString ClearingAvatarModel::fileDate(const QModelIndex & index) const
{
    if(!index.isValid())
        return QString();

    return QFileInfo(filePass(index)).created().toString("yyyy-MM-dd");
}

bool ClearingAvatarModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if(!index.isValid() || role != Qt::EditRole || index.column() != 0)
        return false;

    QString file = filePass(index);
    switch(value.toInt()) {
        case(0):
                if(selected.contains(file))
                    selected.remove(file);
                break;
        case(2):
                if(!selected.contains(file))
                    selected << file;
                break;
        case(3):
                if(selected.contains(file))
                    selected.remove(file);
                else
                    selected << file;
                break;
     }

    emit dataChanged(index, index);
    emit updateLabel(0);

    return false;
}

void ClearingAvatarModel::deleteSelected()
{
   emit layoutAboutToBeChanged ();

    foreach(QString fileName, selected) {
        QFile file(fileName);
        if(file.open(QIODevice::ReadWrite)) {
            file.remove();
            files.removeAt(files.indexOf(fileName));
        }
    }
    selected.clear();
    emit layoutChanged();
    emit updateLabel(0);
}

void ClearingAvatarModel::reset()
{
    selected.clear();
    files.clear();
    QAbstractTableModel::reset();
}

void ClearingAvatarModel::setDir(QStringList dir)
{
     dir_.clear();
     dir_ = dir;
     foreach(QString dirName, dir_){
         QDir Dir(dirName);
         foreach(QString fileName, Dir.entryList(QDir::Files)) {
             files.append(Dir.absolutePath()+QDir::separator()+fileName);
         }
     }

     emit layoutChanged();
}

int ClearingAvatarModel::selectedCount(const QModelIndex &) const
{
    return selected.size();
}

void ClearingAvatarModel::selectAll()
{
    selected.clear();
    selected = files.toSet();
    emit updateLabel(0);
    emit layoutChanged();
}

void ClearingAvatarModel::unselectAll()
{
    selected.clear();
    emit updateLabel(0);
    emit layoutChanged();
}



//----------------------------------------
//--ClearingOptionsModel------------------
//----------------------------------------
ClearingOptionsModel::ClearingOptionsModel(QString fileName, QObject* parent)
        :QAbstractTableModel(parent)
        ,fileName_(fileName)
 {
     headers << tr("")
             << tr("Options")
             << tr("Values");

     parser_ = new OptionsParser(fileName_, this);
     options = parser_->getMissingNodesString();
 }

QVariant ClearingOptionsModel::headerData ( int section, Qt::Orientation orientation, int role) const
 {
     if (role == Qt::DisplayRole) {
                if (orientation == Qt::Horizontal) {
                        return headers.at(section);
                } else {
                        return section+1;
                }
        } else
            return QVariant();
 }

Qt::ItemFlags ClearingOptionsModel::flags ( const QModelIndex & index ) const
 {
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

     if(index.column() == 0)
         flags |= Qt::ItemIsUserCheckable;

     return flags;
 }

int ClearingOptionsModel::columnCount(const QModelIndex & /*parent*/) const
 {
     return headers.size();
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
                        return selected.contains(options.at(index.row())) ? 2:0;
                } else if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignRight | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole)
                        return QVariant("");
             case(1):
                if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignLeft | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole) {
                        return QVariant(options.at(index.row()));
                }
             case(2):
                if (role == Qt::TextAlignmentRole) {
                        return (int)(Qt::AlignLeft | Qt::AlignVCenter);
                } else if (role == Qt::DisplayRole) {
                        QDomNode node = parser_->nodeByString(options.at(index.row()));
                        return QVariant(node.toElement().text());
                }
        }
    return QVariant();
 }

bool ClearingOptionsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if(!index.isValid() || role != Qt::EditRole || index.column() != 0)
        return false;

    QString optionName = options.at(index.row());
     switch(value.toInt()) {
        case(0):
                if(selected.contains(optionName))
                    selected.remove(optionName);
                break;
        case(2):
                if(!selected.contains(optionName))
                    selected << optionName;
                break;
        case(3):
                if(selected.contains(optionName))
                    selected.remove(optionName);
                else
                    selected << optionName;
                break;
     }

    emit dataChanged(index, index);
    emit updateLabel(0);

    return false;
}

void ClearingOptionsModel::deleteSelected()
{
   emit layoutAboutToBeChanged ();


    reset();
    setFile(fileName_);

    emit updateLabel(0);
}

void ClearingOptionsModel::reset()
{
    selected.clear();
    options.clear();
    QAbstractTableModel::reset();
}

void ClearingOptionsModel::setFile(QString fileName)
{
    fileName_ = fileName;
    delete(parser_);
    parser_ = new OptionsParser(fileName_, this);
    options = parser_->getMissingNodesString();

    emit layoutChanged();
}

int ClearingOptionsModel::selectedCount(const QModelIndex &) const
{
    return selected.size();
}

void ClearingOptionsModel::selectAll()
{
    selected.clear();
    selected = options.toSet();
    emit updateLabel(0);
    emit layoutChanged();
}

void ClearingOptionsModel::unselectAll()
{
    selected.clear();
    emit updateLabel(0);
    emit layoutChanged();
}
