/*
 * cditemmodel.h - model for contents tree
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef CDITEMMODEL_H
#define CDITEMMODEL_H

#include <QAbstractTableModel>
#include "contentitem.h"

class CDItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    CDItemModel(QObject *parent = NULL);
    ~CDItemModel();

    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void addRecord(QString group, QString name, QString url, QString html);
    QList<ContentItem*> getToInstall() const;
    void setDataDir(const QString &dataDir);
    void setResourcesDir(const QString &resourcesDir);
    void update();

private:
    ContentItem *rootItem_;
    QString dataDir_;
    QString resourcesDir_;
};

#endif // CDITEMMODEL_H
