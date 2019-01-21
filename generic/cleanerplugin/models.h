/*
 * models.h - plugin
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

class OptionsParser;



//---------------------------------
//------BaseModel------------------
//---------------------------------
class BaseModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    BaseModel(QObject* p = 0) : QAbstractTableModel(p) {};
    virtual bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
    virtual QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;
    virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual void reset();
    int selectedCount(const QModelIndex & parent = QModelIndex()) const;
    void selectAll(const QModelIndexList& list);
    void unselectAll();
    virtual void deleteSelected() = 0;

protected:
    bool isSelected(const QModelIndex& index) const;

protected:
    QStringList headers;
    QSet<QModelIndex> selected_;

signals:
    void updateLabel(int);
};



//---------------------------------
//------BaseFileModel--------------
//---------------------------------
class BaseFileModel : public BaseModel
{
    Q_OBJECT
public:
    BaseFileModel(QObject* p = 0) : BaseModel(p) {};
    virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    QString filePass(const QModelIndex & index) const;
    virtual void reset();
    virtual void deleteSelected();
    void setDirs(const QStringList& dirs);

protected:
    QString fileName(const QModelIndex & index) const;
    int fileSize(const QModelIndex & index) const;
    QString fileDate(const QModelIndex & index) const;


private:
    QStringList files_, dirs_;
};


//---------------------------------
//------ClearingModel--------------
//---------------------------------
class ClearingModel : public BaseFileModel
{
    Q_OBJECT
public:
    ClearingModel(const QString& dir, QObject *parent = 0);
        virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;    
};


//---------------------------------
//------ClearingVcardModel---------
//---------------------------------
class ClearingVcardModel : public ClearingModel
{
    Q_OBJECT
public:
    ClearingVcardModel(const QString& dir, QObject *parent = 0) : ClearingModel(dir, parent) {};
    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
};


//---------------------------------
//------ClearingHistoryModel-------
//---------------------------------
class ClearingHistoryModel : public ClearingModel
{
    Q_OBJECT
public:
    ClearingHistoryModel(const QString& dir, QObject *parent = 0) : ClearingModel(dir, parent) {};
    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
};



//---------------------------------
//------ClearingAvatarModel--------
//---------------------------------
class ClearingAvatarModel : public BaseFileModel
{
    Q_OBJECT
public:
    ClearingAvatarModel(const QStringList& dir, QObject *parent = 0);
    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
};


//---------------------------------
//------ClearingOptionsModel-------
//---------------------------------
class ClearingOptionsModel : public BaseModel
{
    Q_OBJECT
public:
    ClearingOptionsModel(const QString& fileName, QObject *parent = 0);
    virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    virtual void deleteSelected();
    virtual void reset();
    void setFile(const QString& fileName);

private:
    QStringList options;
    QString fileName_;

    OptionsParser *parser_;
};


//---------------------------------
//------ClearingProxyModel---------
//---------------------------------
class ClearingProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    ClearingProxyModel(QObject *parent = 0);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &parent) const;
};

#endif // MODELS_H
