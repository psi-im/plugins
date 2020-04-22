/*
 * model.h - plugin
 * Copyright (C) 2010  Evgeny Khryukin
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
#include <QSet>
#include <QStringList>

class Model : public QAbstractTableModel {
    Q_OBJECT

public:
    Model(const QStringList &watchedJids_, const QStringList &Sounds_, const QStringList &enabledJids_,
          QObject *parent = nullptr);
    ~Model() {};
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QVariant      headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QVariant      data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual int           rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int           columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual bool          setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    QString               jid(const QModelIndex &index) const;
    QString               soundFile(const QModelIndex &index) const;
    QString               tmpSoundFile(const QModelIndex &index) const;
    void                  apply();
    void                  deleteRows(const QModelIndexList &indexList);
    bool                  removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    void                  reset();
    void                  addRow(const QString &jid = "");
    void                  deleteRow(const QString &jid);
    void                  setStatusForJid(const QString &jid, const QString &status);
    QString               statusByJid(const QString &jid) const;
    QString               soundByJid(const QString &jid) const;
    int                   indexByJid(const QString &jid) const;
    QStringList           getWatchedJids() const;
    QStringList           getSounds() const;
    QStringList           getEnabledJids() const;
    void                  setJidEnabled(const QString &jid, bool enabled);
    bool                  jidEnabled(const QString &jid);

private:
    QStringList            headers, watchedJids, tmpWatchedJids_, sounds, tmpSounds_, enabledJids;
    QMap<QString, QString> statuses;
    QList<bool>            tmpEnabledJids_;
};

#endif // MODEL_H
