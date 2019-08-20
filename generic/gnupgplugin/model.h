/*
 * model.h - key view model
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

#ifndef MODEL_H
#define MODEL_H

#include <QStandardItemModel>

class Model : public QStandardItemModel {
    Q_OBJECT

public:
    enum Columns {
        Type,
        Name,
        Email,
        Created,
        Expiration,
        Length,
        Comment,
        Algorithm,
        ShortId,
        Fingerprint,
        Count, // Trick to count items in enum
        First = 0,
        Last = Count - 1
    };

    Model(QObject *parent = nullptr);

public slots:
    void listKeys();

private:
    void showKeys(const QString &keysRaw);

};

#endif // MODEL_H
