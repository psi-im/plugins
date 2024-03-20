/*
 * model.cpp - plugin
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

#include "model.h"
#include <QRegularExpression>

Model::Model(const QStringList &watchedJids_, const QStringList &Sounds_, const QStringList &enabledJids_,
             QObject *parent) :
    QAbstractTableModel(parent),
    watchedJids(watchedJids_), sounds(Sounds_), enabledJids(enabledJids_)
{
    headers << tr("") << tr("Watch for JIDs") << tr("Sounds (if empty default sound will be used)") << tr("") << tr("");

    tmpWatchedJids_ = watchedJids;
    tmpSounds_      = sounds;

    for (const auto &enabledJid : enabledJids_) {
        tmpEnabledJids_ << (enabledJid == "true" ? true : false);
    }
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            return headers.at(section);
        } else {
            return section + 1;
        }
    } else
        return QVariant();
}

Qt::ItemFlags Model::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    int column = index.column();

    if (column == 0)
        flags |= Qt::ItemIsUserCheckable;
    else if (column == 1 || column == 2)
        flags |= Qt::ItemIsEditable;

    return flags;
}

int Model::columnCount(const QModelIndex & /*parent*/) const { return headers.size(); }

int Model::rowCount(const QModelIndex & /* parent*/) const { return tmpWatchedJids_.size(); }

QVariant Model::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int i = index.column();
    switch (i) {
    case 0:
        if (role == Qt::CheckStateRole) {
            return tmpEnabledJids_.at(index.row()) ? 2 : 0;
        } else if (role == Qt::TextAlignmentRole) {
            return (int)(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole)
            return QVariant();
        break;
    case 1:
        if (role == Qt::TextAlignmentRole) {
            return (int)(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole)
            return QVariant(tmpWatchedJids_.at(index.row()));
        break;
    case 2:
        if (role == Qt::TextAlignmentRole) {
            return (int)(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole)
            return QVariant(tmpSounds_.at(index.row()));
        break;
    case 3:
        if (role == Qt::TextAlignmentRole) {
            return (int)(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole)
            return QVariant();
        break;
    case 4:
        if (role == Qt::TextAlignmentRole) {
            return (int)(Qt::AlignRight | Qt::AlignVCenter);
        } else if (role == Qt::DisplayRole)
            return QVariant();
        break;
    }
    return QVariant();
}

QString Model::jid(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();

    return watchedJids.at(index.row());
}

QString Model::soundFile(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();

    return sounds.at(index.row());
}

QString Model::tmpSoundFile(const QModelIndex &index) const
{
    if (!index.isValid())
        return QString();

    return tmpSounds_.at(index.row());
}

bool Model::setData(const QModelIndex &index, const QVariant &value, const int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    int column = index.column();
    if (column == 0) {
        bool b = tmpEnabledJids_.at(index.row());
        switch (value.toInt()) {
        case 0:
            tmpEnabledJids_.replace(index.row(), false);
            break;
        case 2:
            tmpEnabledJids_.replace(index.row(), true);
            break;
        case 3:
            tmpEnabledJids_.replace(index.row(), !b);
            break;
        }
    } else if (column == 1)
        tmpWatchedJids_.replace(index.row(), value.toString());

    else if (column == 2)
        tmpSounds_.replace(index.row(), value.toString());

    emit dataChanged(index, index);

    return true;
}

void Model::setStatusForJid(const QString &jid, const QString &status) { statuses.insert(jid, status); }

void Model::deleteRows(const QModelIndexList &indexList)
{
    QList<bool> selected;
    for (int i = 0; i < tmpWatchedJids_.size(); i++) {
        selected << false;
    }

    for (auto index : indexList) {
        selected[index.row()] = true;
    }

    QStringList tmpJids, tmpSounds;
    // QList<bool> tmpEnabledJids;
    for (int i = tmpWatchedJids_.size() - 1; i >= 0; i--) {
        if (selected.at(i)) {
            removeRow(i);
        }
    }
}

bool Model::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; i++) {
        tmpWatchedJids_.removeAt(row);
        tmpSounds_.removeAt(row);
        tmpEnabledJids_.removeAt(row);
    }
    endRemoveRows();
    return true;
}

void Model::reset()
{
    tmpWatchedJids_ = watchedJids;
    tmpSounds_      = sounds;

    tmpEnabledJids_.clear();
    for (const auto &enabledJid : std::as_const(enabledJids)) {
        tmpEnabledJids_ << (enabledJid == "true" ? true : false);
    }
}

void Model::addRow(const QString &jid)
{
    beginInsertRows(QModelIndex(), tmpWatchedJids_.size(), tmpWatchedJids_.size());
    tmpWatchedJids_ << jid;
    tmpSounds_ << "";

    if (!jid.isEmpty()) { // вызов происходит из меню контакта
        watchedJids.append(jid);
        sounds.append("");
        enabledJids.append("true");
    }

    tmpEnabledJids_.append(true);
    endInsertRows();
}

void Model::deleteRow(const QString &jid)
{
    int index = watchedJids.indexOf(QRegularExpression(jid, QRegularExpression::CaseInsensitiveOption));
    if (index == -1)
        return;
    watchedJids.removeAt(index);
    sounds.removeAt(index);
    tmpWatchedJids_.removeAt(index);
    tmpSounds_.removeAt(index);
    tmpEnabledJids_.removeAt(index);

    emit layoutChanged();
}

void Model::apply()
{
    watchedJids = tmpWatchedJids_;
    sounds      = tmpSounds_;
    enabledJids.clear();
    for (auto enabledJid : std::as_const(tmpEnabledJids_)) {
        enabledJids << (enabledJid ? "true" : "false");
    }
}

QString Model::statusByJid(const QString &jid) const { return statuses.value(jid, "offline"); }

QString Model::soundByJid(const QString &jid) const
{
    QString sound;
    int     index = watchedJids.indexOf(QRegularExpression(jid, QRegularExpression::CaseInsensitiveOption));
    if (index < sounds.size() && index != -1)
        sound = sounds.at(index);

    return sound;
}

int Model::indexByJid(const QString &jid) const
{
    return watchedJids.indexOf(QRegularExpression(jid, QRegularExpression::CaseInsensitiveOption));
}

QStringList Model::getWatchedJids() const { return watchedJids; }

QStringList Model::getSounds() const { return sounds; }

QStringList Model::getEnabledJids() const { return enabledJids; }

void Model::setJidEnabled(const QString &jid, bool enabled)
{
    // Do nothing if need to disable jid which is not in watcher list
    if (!getWatchedJids().contains(jid, Qt::CaseInsensitive) && !enabled) {
        return;
    }

    // Before add Jid to watcher list
    if (!getWatchedJids().contains(jid, Qt::CaseInsensitive)) {
        addRow(jid);
    }

    QModelIndex ind = index(indexByJid(jid), 0);
    setData(ind, enabled ? 2 : 0);
}

bool Model::jidEnabled(const QString &jid)
{
    // watcher doesn't applied for this jid
    if (!getWatchedJids().contains(jid, Qt::CaseInsensitive)) {
        return false;
    }

    QModelIndex ind = index(indexByJid(jid), 0);
    return (data(ind, Qt::CheckStateRole) == 2) ? true : false;
}
