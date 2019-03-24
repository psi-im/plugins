/*
 * model.h - plugin
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

#ifndef MODEL_H
#define MODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <QVariantList>
#include <QSet>

class Model : public QAbstractTableModel
{
    Q_OBJECT

public:
        Model(const QStringList &Jids_, QVariantList selected_, QObject *parent = nullptr);
    ~Model() {}
        virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;
        virtual QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
        virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
        virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
        virtual bool setData ( const QModelIndex & index, const QVariant & value, const int role = Qt::EditRole );
        QString jid(const QModelIndex & index) const;
        void reset();
        void apply();
        void addRow();
    void deleteRow(const int row);
    int indexByJid(const QString &jid) const;
    QVariantList enableFor() const;
    QStringList getJids() const;


private:
        QStringList headers, Jids, tmpJids_;
        QSet<QString> selected;
};


#endif // MODEL_H
