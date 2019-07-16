/*
 * cditemmodel.cpp - model for contents tree
 * Copyright (C) 2010  Ivan Romanov <drizt@land.ru>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <QDebug>
#include <QDir>
#include "cditemmodel.h"

CDItemModel::CDItemModel(QObject *parent)
    : QAbstractItemModel(parent)
    , rootItem_(new ContentItem(""))
{
}
CDItemModel::~CDItemModel()
{
    delete rootItem_;
}

int CDItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    // in our model always only 1 column
    return 1;
}

QVariant CDItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if(role == Qt::DisplayRole) {
        ContentItem *item = static_cast<ContentItem*>(index.internalPointer());
        return item->name();
    }

    ContentItem *item = static_cast<ContentItem*>(index.internalPointer());
    if(role == Qt::CheckStateRole) {
        if(item->isInstalled()) {
            return 1;
        } else if(item->toInstall()) {
            return 2;
        } else    {
            return 0;
        }
    }
    
    return QVariant();
}

bool CDItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid()) {
        return false;
    }
    
    if(role == Qt::CheckStateRole) {
        ContentItem *item = static_cast<ContentItem*>(index.internalPointer());
        item->setToInstall(value.toBool());

        // set the same for the descends 
        int i = 0;
        while(QAbstractItemModel::index(i, 0, index).isValid()) {
            setData(QAbstractItemModel::index(i++, 0, index), value, role);
        }
        
        if(index.parent().isValid()) {
            if(value.toBool() == false) {
                static_cast<ContentItem*>(index.internalPointer())->setToInstall(false);
            } else {
                int i = 0;
                bool b = true;
                while(index.sibling(i, 0).isValid()) {
                    b = data(index.sibling(i++, 0), role).toBool();
                    if(!b) {
                        break;
                    }
                }
                
                static_cast<ContentItem*>(index.internalPointer())->setToInstall(b);
            }
        }

        emit dataChanged(index, index);
        emit dataChanged(index.parent(), index.parent());
        return true;
    }
    return false;
}


Qt::ItemFlags CDItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return nullptr;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
}

QVariant CDItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)

    return QVariant();
}

QModelIndex CDItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    ContentItem *parentItem;

    if (!parent.isValid()) {
        parentItem = rootItem_;
    } else {
        parentItem = static_cast<ContentItem*>(parent.internalPointer());
    }

    ContentItem *childItem = parentItem->child(row);
    if (childItem) {
        return createIndex(row, column, childItem);
    } else {
        return QModelIndex();
    }
}

QModelIndex CDItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    ContentItem *childItem = static_cast<ContentItem*>(index.internalPointer());
    ContentItem *parentItem = childItem->parent();

    if (parentItem == rootItem_) {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int CDItemModel::rowCount(const QModelIndex &parent) const
{
    ContentItem *parentItem;
    if (parent.column() > 0) {
        return 0;
    }

    if (!parent.isValid()) {
        parentItem = rootItem_;
    } else {
        parentItem = static_cast<ContentItem*>(parent.internalPointer());
    }
    
    return parentItem->childCount();
}

void CDItemModel::addRecord(QString group, QString name, QString url, QString html)
{
    ContentItem *parent = rootItem_;
    QStringList subGroups = group.split("/");
    
    while(!subGroups.isEmpty()) {
        ContentItem *newParent = nullptr;
        for(int i = parent->childCount() - 1; i >= 0; i--) {
            if(parent->child(i)->name() == subGroups.first()) {
                newParent = parent->child(i);
                break;
            }
        }
        
        if(newParent == nullptr) {
            newParent = new ContentItem(subGroups.first(), parent);
            parent->appendChild(newParent);
        }
        
        parent = newParent;
        subGroups.removeFirst();
    }
    
    ContentItem *item = new ContentItem(name, parent);
    item->setGroup(group);
    item->setUrl(url);
    item->setHtml(html);
    parent->appendChild(item);
}

QList<ContentItem*> CDItemModel::getToInstall() const
{
    QList<ContentItem*> listItems;
    QModelIndex index = this->index(0, 0);
    while(index.isValid()) {
        if(QAbstractItemModel::index(0,0,index).isValid()) {
            // Descent
            index = QAbstractItemModel::index(0, 0, index);
        } else {
            while(index.isValid()) {
                ContentItem *item = static_cast<ContentItem*>(index.internalPointer());

                if(item->toInstall()) {
                    listItems << item;
                }
                
                if(!index.sibling(index.row() + 1, 0).isValid()) {
                    index = index.parent();
                    break;
                }
                index = index.sibling(index.row() + 1, 0);
            }
  
            while(index.parent().isValid() && !index.sibling(index.row() + 1, 0).isValid()) {
                index = index.parent();
            }

            index = index.sibling(index.row() + 1, 0);
        }
    }
    return listItems;
}

void CDItemModel::setDataDir(const QString &dataDir)
{
    dataDir_ = dataDir;
}

void CDItemModel::setResourcesDir(const QString &resourcesDir)
{
    resourcesDir_ = resourcesDir;
}

void CDItemModel::update()
{
    QModelIndex index = this->index(0, 0);
    while(index.isValid()) {
        if(QAbstractItemModel::index(0, 0, index).isValid()) {
            // Descent
            index = QAbstractItemModel::index(0, 0, index);
        } else {
            bool allInstalled = true;
            while(true) {
                ContentItem *item = static_cast<ContentItem*>(index.internalPointer());
                QString filename = item->url().section("/", -1);
                QString fullFileNameH = QDir::toNativeSeparators(QString("%1/%2/%3").
                                                                 arg(dataDir_).
                                                                 arg(item->group()).
                                                                 arg(filename));

                QString fullFileNameR = QDir::toNativeSeparators(QString("%1/%2/%3").
                                                                 arg(resourcesDir_).
                                                                 arg(item->group()).
                                                                 arg(filename));

                if(QFile::exists(fullFileNameH) || QFile::exists(fullFileNameR)) {
                    item->setToInstall(false);
                    item->setIsInstalled(true);
                    emit dataChanged(index, index);
                } else {
                    allInstalled = false;
                }
                
                if(!index.sibling(index.row() + 1, 0).isValid()) {
                    index = index.parent();
                    if(allInstalled) {
                        item->parent()->setIsInstalled(true);
                        emit dataChanged(index, index);
                    }

                    break;
                }
                index = index.sibling(index.row() + 1, 0);
            }
  
            while(index.parent().isValid() && !index.sibling(index.row() + 1, 0).isValid()) {
                index = index.parent();
            }

            index = index.sibling(index.row() + 1, 0);
        }
    } 
}
