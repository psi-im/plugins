/*
 * model.h - plugin
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

#ifndef MODEL_H
#define MODEL_H

#include "jd_item.h"

class JDModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum {
        RoleType = Qt::UserRole + 1,
        RoleName = Qt::UserRole + 2,
        RoleSize = Qt::UserRole + 3,
        RoleNumber = Qt::UserRole + 4,
        RoleFullPath = Qt::UserRole + 5,
        RoleParentPath = Qt::UserRole + 6
    };

    JDModel(const QString& diskName, QObject *parent = 0);
    ~JDModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &) const { return 1; };
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index) const;
    QModelIndex indexForItem(JDItem* item) const;
    QVariant data(const QModelIndex &index, int role) const;    

    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    Qt::DropActions supportedDropActions() const;
    QStringList mimeTypes() const;
    //bool removeRow(int row, const QModelIndex &parent = QModelIndex());

    QStringList dirs(const QString& path) const;
    void addFile(const QString& curPath, const QString& name, const QString& size, const QString& descr, int number);
    void addDir(const QString& curPath, const QString& name);
    const QModelIndex rootIndex() const;
    const QString disk() const;

    static const QString rootPath() { return "/"; };

    void clear();

signals:
    void moveItem(const QString& oldPat, const QString& newPath);

private:
    JDItem* findDirItem(const QString& name) const;
    bool addItem(JDItem *i);
    void removeAll();

private:
    ItemsList items_;
    QString diskName_;
    const QModelIndex rootIndex_;
};

#endif
