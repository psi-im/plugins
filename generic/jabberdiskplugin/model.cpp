/*
 * model.cpp - plugin
 * Copyright (C) 2011  Evgeny Khryukin
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

#include "model.h"
#include <QApplication>
#include <QStyle>
#include <QMimeData>
//#include <QDebug>

JDModel::JDModel(const QString &diskName, QObject *parent)
    : QAbstractItemModel(parent)
    , diskName_(diskName)
    , rootIndex_(createIndex(0, 0, static_cast<quintptr>(0)))
{
}

JDModel::~JDModel()
{
    removeAll();
}

Qt::ItemFlags JDModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = QAbstractItemModel::flags(index);

    if (!index.isValid())
        return f;

    f |= Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if(index.data(RoleType) == JDItem::File)
        f |= Qt::ItemIsDragEnabled;
    else
        f |= Qt::ItemIsDropEnabled;

    return f;
}

int JDModel::rowCount(const QModelIndex &parent) const
 {
    if(parent == QModelIndex()) {
        return 1;
    }

    int count = 0;
    foreach(const ProxyItem& i, items_) {
        if(i.parent == parent)
            ++count;
    }

    return count;
 }

bool JDModel::hasChildren(const QModelIndex &parent) const
{
    JDItem *i = (JDItem*)parent.internalPointer();
    if(i) {
        if(i->type() == JDItem::File)
            return false;
        foreach(const ProxyItem& item, items_) {
            if(item.item->parent() == i)
                return true;
        }
    }

    return true;
}

QVariant JDModel::headerData(int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const
{
    return QVariant();
}

QModelIndex JDModel::index(int row, int column, const QModelIndex& parent) const
{
    if(column != 0)
        return QModelIndex();

    if(parent == QModelIndex()) {
        if(row == 0)
            return rootIndex();
        else
            return QModelIndex();
    }

    int c = 0;
    foreach(const ProxyItem& i, items_) {
        if(i.parent == parent) {
            if(row == c)
                return i.index;
            ++c;
        }
    }

    return QModelIndex();
}

QModelIndex JDModel::parent(const QModelIndex& index) const
{
    if(!index.isValid())
        return QModelIndex();
    else if(!index.internalPointer())
        return QModelIndex();

    foreach(const ProxyItem& i, items_)
        if(i.index == index)
            return i.parent;

    return QModelIndex();
}

QModelIndex JDModel::indexForItem(JDItem *item) const
{
    foreach(const ProxyItem& i, items_)
        if(i.item == item)
            return i.index;

    return QModelIndex();
}

const QModelIndex JDModel::rootIndex() const
{
    return rootIndex_;
}

const QString JDModel::disk() const
{
    return diskName_.split("@").first();
}

bool JDModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex &parent)
{
    if(!parent.isValid())
        return false;
    if(action != Qt::CopyAction && action != Qt::MoveAction)
        return false;
    if(!data->hasFormat(JDItem::mimeType()))
        return false;

    JDItem *p = 0;
    if(parent != rootIndex()) {
        foreach(const ProxyItem& i, items_) {
            if(i.index == parent) {
                p = i.item;
                break;
            }
        }
    }
    if(p && p->type() == JDItem::File)
        return false;

    JDItem* newItem = new JDItem(JDItem::File, p);
    QByteArray ba = data->data(JDItem::mimeType());
    QDataStream in(&ba, QIODevice::ReadOnly);
    newItem->fromDataStream(&in);

    if(addItem(newItem)) {
        //В DataStream осталась только информация о полном пути
        //к старому элементу, и ее можно получить
        QString path;
        in >> path;
        emit moveItem(path, p ? p->fullPath() : rootPath());
    }

    return true;
}

QMimeData* JDModel::mimeData(const QModelIndexList &indexes) const
{
    if(indexes.isEmpty())
        return 0;

    QMimeData *data = 0;
    QModelIndex i = indexes.first();
    foreach(const ProxyItem& item, items_) {
        if(item.index == i) {
            data = item.item->mimeData();
            break;
        }
    }

    return data;
}

Qt::DropActions JDModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QStringList JDModel::mimeTypes() const
{
    return QStringList(JDItem::mimeType());
}

//bool JDModel::removeRow(int row, const QModelIndex &parent)
//{
//    if(!parent.isValid())
//        return false;
//
//    ProxyItem pi;
//    foreach(const ProxyItem& item, items_) {
//        if(item.parent == parent && item.index.row() == row) {
//            pi = item;
//            break;
//        }
//    }
//    if(pi.isNull())
//        return false;
//
//    emit layoutAboutToBeChanged();
//    items_.removeAll(pi);
//    delete pi.item;
//    emit layoutChanged();
//
//    return true;
//}

QVariant JDModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if(!index.internalPointer()) {
        if(role == Qt::DisplayRole) {
            return diskName_;
        }
        else if(role == RoleFullPath)
            return rootPath();
    }
    else {
        JDItem* i = (JDItem*)index.internalPointer();
        if(i) {
            if(role == Qt::DisplayRole) {
                QString text;
                if(i->type() == JDItem::Dir)
                    text = i->name();
                else
                    text = QString("%1 - %2 [%3] - %4").arg(QString::number(i->number()), i->name(), i->size(), i->description());

                return text;
            }
            else if(role == RoleName)
                return i->name();
            else if(role == RoleSize)
                return i->size();
            else if(role == RoleNumber)
                return i->number();
            else if(role == RoleType)
                return i->type();
            else if(role == Qt::DecorationRole) {
                if(i->type() == JDItem::Dir)
                    return qApp->style()->standardIcon(QStyle::SP_DirIcon);
                else
                    return qApp->style()->standardIcon(QStyle::SP_FileIcon);
            }
            else if(role == RoleFullPath)
                return i->fullPath();
            else if(role == RoleParentPath)
                return i->parentPath();
        }
    }

    return QVariant();
}

bool JDModel::addItem(JDItem *i)
{
    if(items_.contains(i))
        return false;

    ProxyItem pi;
    pi.item = i;

    if(i->parent()) {
        foreach(const ProxyItem& item, items_) {
            if(item.item == i->parent()) {
                pi.parent = item.index;
                break;
            }
        }
    }
    else {
        pi.parent = rootIndex();
    }

    int count = 0;
    foreach(const ProxyItem& item, items_) {
        if(item.item->parent() == i->parent())
            ++count;
    }

    pi.index = createIndex(count, 0, i);

    items_.append(pi);

    emit layoutChanged();
    return true;
}

void JDModel::clear()
{
    beginResetModel();
    removeAll();
    endResetModel();
}

void JDModel::removeAll()
{
//    while(!items_.isEmpty())
//        delete items_.takeFirst().item;
    items_.clear();
}

JDItem* JDModel::findDirItem(const QString &path) const
{
    if(!path.isEmpty()) {
        foreach(const ProxyItem i, items_) {
            if(i.item->type() != JDItem::Dir)
                continue;

            if(i.item->fullPath() == path)
                return i.item;
        }
    }

    return 0;
}

QStringList JDModel::dirs(const QString &path) const
{
    QStringList dirs_;
    foreach(const ProxyItem& i, items_) {
        if(i.item->type() != JDItem::Dir)
            continue;
        if(i.item->parent()) {
            if(i.item->parent()->fullPath() == path)
                dirs_.append(i.item->name());
        }
        else if(path.isEmpty()) {
                dirs_.append(i.item->name());
        }
    }

    return dirs_;
}

void JDModel::addDir(const QString &curPath, const QString &name)
{
    JDItem *i = new JDItem(JDItem::Dir, findDirItem(curPath));
    i->setData(name);
    addItem(i);
}

void JDModel::addFile(const QString &curPath, const QString &name, const QString &size, const QString &descr, int number)
{
    JDItem *i = new JDItem(JDItem::File, name, size, descr, number, findDirItem(curPath));
    addItem(i);
}
