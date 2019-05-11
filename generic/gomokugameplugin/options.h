/*
 * options.h - Gomoku Game plugin
 * Copyright (C) 2011  Aleksey Andreev
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * You can also redistribute and/or modify this program under the
 * terms of the Psi License, specified in the accompanied COPYING
 * file, as published by the Psi Project; either dated January 1st,
 * 2005, or (at your option) any later version.
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <QObject>

#include "optionaccessor.h"

#define constDndDisable         "dnddsbl"
#define constConfDisable        "confdsbl"
#define constSaveWndPosition    "savewndpos"
#define constSaveWndWidthHeight "savewndwh"
#define constWindowTop          "wndtop"
#define constWindowLeft         "wndleft"
#define constWindowWidth        "wndwidth"
#define constWindowHeight       "wndheight"
#define constDefSoundSettings   "defsndstngs"
#define constSoundStart         "soundstart"
#define constSoundFinish        "soundfinish"
#define constSoundMove          "soundmove"
#define constSoundError         "sounderror"

class Options : public QObject
{
Q_OBJECT
public:
    static OptionAccessingHost *psiOptions;
    static Options *instance();
    static void reset();
    QVariant getOption(const QString &option_name) const;
    void     setOption(const QString &option_name, const QVariant &option_value);

private:
    static Options *instance_;
    bool dndDisable;
    bool confDisable;
    bool saveWndPosition;
    bool saveWndWidthHeight;
    int  windowTop;
    int  windowLeft;
    int  windowWidth;
    int  windowHeight;
    bool defSoundSettings;
    QString soundStart;
    QString soundFinish;
    QString soundMove;
    QString soundError;

private:
    Options(QObject *parent = nullptr);

signals:

public slots:

};

#endif // OPTIONS_H
