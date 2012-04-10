/*
 * model.h - plugin
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
        Model(QStringList Jids_, QVariantList selected_, QObject *parent = 0);
	~Model() {}
        virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;
        virtual QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
        virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
        virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
        virtual bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
        QString jid(const QModelIndex & index) const;
        void reset();
        void apply();
        void addRow();
	void deleteRow(int row);
	int indexByJid(const QString& jid) const;
	QVariantList enableFor() const;
	QStringList getJids() const;


private:
        QStringList headers, Jids, tmpJids_;
        QSet<QString> selected;
};


#endif // MODEL_H
