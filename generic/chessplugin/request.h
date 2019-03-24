/*
 * request.h - plugin
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

#ifndef REQUEST_H
#define REQUEST_H

#include "figure.h"

struct Request {
    int account = 0;
    QString jid;
    QString yourJid;
    Figure::GameType type = Figure::GameType::NoGame;
    QString requestId;
    QString chessId;

    bool operator==(const Request& other)
    {
        return jid == other.jid && account == other.account;
    }
};


#endif // REQUEST_H
