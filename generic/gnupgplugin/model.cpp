/*
 * model.cpp - key view model
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
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

#include "model.h"
#include "gpgprocess.h"
#include <QDateTime>

inline QString epochToHuman(const QString &seconds)
{
    qint64 ms = seconds.toLongLong() * 1000;
    if (ms)
        return QDateTime::fromMSecsSinceEpoch(ms).date().toString();
    else
        return QString();
}

inline QString uidToName(const QString &uid)
{
    if (uid.contains('(')) {
        return uid.section('(', 0, 0).trimmed();
    }
    else if (uid.contains('<')) {
        return uid.section('<', 0, 0).trimmed();
    }
    else {
        return uid.trimmed();
    }
}

inline QString uidToEMail(const QString &uid)
{
    if (uid.contains('<') && uid.contains('>')) {
        return uid.section('<', 1).section('>', 0, 0).trimmed();
    }
    else
        return "";
}

inline QString uidToComment(const QString &uid)
{
    if (uid.contains('(') && uid.contains(')')) {
        return uid.section('(', 1).section(')', 0, 0).trimmed();
    }
    else {
        return "";
    }
}

QList<QStandardItem*> parseLine(const QString &line)
{
    QList<QStandardItem*> rows;

    // Used ID
    QString uid = line.section(':', 9, 9);

    // Type
    rows << new QStandardItem(line.section(':', 0, 0));

    // Name
    rows << new QStandardItem(uidToName(uid));

    // E-mail
    rows << new QStandardItem(uidToEMail(uid));

    // Creation key time in human readable format
    rows << new QStandardItem(epochToHuman(line.section(':', 5, 5)));

    // Expiration time
    rows << new QStandardItem(epochToHuman(line.section(':', 6, 6)));

    // Length of key
    rows << new QStandardItem(line.section(':', 2, 2));

    // Comment
    rows << new QStandardItem(uidToComment(uid));

    // Algorithm
    int alg = line.section(':', 3, 3).toInt();
    switch(alg) {
    case 1: rows << new QStandardItem("RSA"); break;
    case 16: rows << new QStandardItem("ELG-E"); break;
    case 17: rows << new QStandardItem("DSA"); break;
    case 18: rows << new QStandardItem("ECC"); break;
    default: rows << new QStandardItem(""); break;
    }

    // Short ID
    rows << new QStandardItem(line.section(':', 4, 4).right(8));

    // Fingerprint
    rows << new QStandardItem("");

    return rows;
}

Model::Model(QObject *parent)
    : QStandardItemModel(parent)
{
}

void Model::listKeys()
{
    clear();

    static QStringList headerLabels;

    if (headerLabels.isEmpty()) {
        for (int i = 0; i < Model::Count; ++i) {
            headerLabels << QString();
        }

        headerLabels[Type] = tr("Type");
        headerLabels[Name] = tr("Name");
        headerLabels[Email] = tr("E-Mail");
        headerLabels[Created] = tr("Created");
        headerLabels[Expiration] = tr("Expiration");
        headerLabels[Length] = tr("Length");
        headerLabels[Comment] = tr("Comment");
        headerLabels[Algorithm] = tr("Algorithm");
        headerLabels[ShortId] = tr("Short ID");
        headerLabels[Fingerprint] = tr("Fingerprint");
    }

    setHorizontalHeaderLabels(headerLabels);

    QStringList arguments;
    arguments << "--with-fingerprint"
              << "--list-secret-keys"
              << "--with-colons"
              << "--fixed-list-mode";

    GpgProcess process;
    process.start(arguments);
    process.waitForFinished();
    QString keysRaw = QString::fromUtf8(process.readAll());

    arguments.clear();
    arguments << "--with-fingerprint"
              << "--list-public-keys"
              << "--with-colons"
              << "--fixed-list-mode";

    process.start(arguments);
    process.waitForFinished();
    keysRaw += QString::fromUtf8(process.readAll());


    showKeys(keysRaw);
}

void Model::showKeys(const QString &keysRaw)
{
    QStringList list = keysRaw.split("\n");
    QList<QStandardItem*> lastRow;
    QList<QStandardItem*> row;
    QStringList secretKeys;
    foreach (QString line, list) {
        QString type = line.section(':', 0, 0);
        if (type == "pub" || type == "sec") {
            row = parseLine(line);

            // Show only secret part for keys pair
            if (type == "sec") {
                secretKeys << row.at(Algorithm)->text();
            }
            else if (secretKeys.indexOf(row.at(ShortId)->text()) >= 0) {
                lastRow.clear();
                continue;
            }

            appendRow(row);
            lastRow = row;
        }
        else if ((type == "uid" || type == "ssb" || type == "sub") && !lastRow.isEmpty()) {
            row = parseLine(line);
            lastRow.first()->appendRow(row);
            if (lastRow.first()->rowCount() == 1) {
                lastRow.at(Name)->setText(row.at(Name)->text());
                lastRow.at(Email)->setText(row.at(Email)->text());
                lastRow.at(Comment)->setText(row.at(Comment)->text());
            }
        }
        else if (type == "fpr") {
            row.at(Fingerprint)->setText(line.section(':', Fingerprint, Fingerprint));
        }
    }
}
