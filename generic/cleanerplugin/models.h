/*
 * models.h - plugin
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

#ifndef MODELS_H
#define MODELS_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QSet>

#include "optionsparser.h"


class ClearingModel : public QAbstractTableModel
{
    Q_OBJECT

public:
        ClearingModel(QString dir, QObject *parent = 0);
        ~ClearingModel() {};
        virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;
        virtual QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
        virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
        virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
        virtual bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
        QString fileName(const QModelIndex & index) const;
        int fileSize(const QModelIndex & index) const;
        QString filePass(const QModelIndex & index) const;
        QString fileDate(const QModelIndex & index) const;
        void deleteSelected();
        void reset();
        void setDir(QString dir);
        int selectedCount(const QModelIndex & parent = QModelIndex()) const;
        void selectAll();
        void unselectAll();

private:
        QStringList headers, files;
        QString dir_;
        QSet<QString> selected;

signals:
        void updateLabel(int);
};



class ClearingVcardModel : public ClearingModel
{
    Q_OBJECT

public:
    ClearingVcardModel(QString dir, QObject *parent = 0) : ClearingModel(dir, parent) {};
    ~ClearingVcardModel() {};
    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
};



class ClearingHistoryModel : public ClearingModel
{
    Q_OBJECT

public:
    ClearingHistoryModel(QString dir, QObject *parent = 0) : ClearingModel(dir, parent) {};
    ~ClearingHistoryModel() {};
    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
};



class ClearingProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    ClearingProxyModel(QObject *parent = 0);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &parent) const;
};



class ClearingAvatarModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ClearingAvatarModel(QStringList dir, QObject *parent = 0);
    ~ClearingAvatarModel() {};
    virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;
    virtual QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
    int fileSize(const QModelIndex & index) const;
    QString filePass(const QModelIndex & index) const;
    QString fileDate(const QModelIndex & index) const;
    void deleteSelected();
    void reset();
    void setDir(QStringList dir);
    int selectedCount(const QModelIndex & parent = QModelIndex()) const;
    void selectAll();
    void unselectAll();



private:
    QStringList headers, files, dir_;
    QSet<QString> selected;

signals:
    void updateLabel(int);
};



class ClearingOptionsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ClearingOptionsModel(QString fileName, QObject *parent = 0);
    ~ClearingOptionsModel() {};
    virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;
    virtual QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
    void deleteSelected();
    void reset();
    void setFile(QString fileName);
    int selectedCount(const QModelIndex & parent = QModelIndex()) const;
    void selectAll();
    void unselectAll();



private:
    QStringList headers, options;
    QString fileName_;
    QSet<QString> selected;

    OptionsParser *parser_;

signals:
    void updateLabel(int);
};

#endif // MODELS_H
