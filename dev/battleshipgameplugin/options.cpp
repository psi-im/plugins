/*
 * options.cpp - Battleship Game plugin
 * Copyright (C) 2014  Aleksey Andreev
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

#include <QVariant>

#include "options.h"
//#include "common.h"
#include "optionaccessinghost.h"

Options::Options(QObject *parent) :
    QObject(parent), dndDisable(false), confDisable(false), saveWndPosition(false), saveWndWidthHeight(false),
    windowTop(-1), windowLeft(-1), windowWidth(-1), windowHeight(-1), defSoundSettings(false),
    soundStart("sound/chess_start.wav"), soundFinish("sound/chess_finish.wav"), soundMove("sound/chess_move.wav"),
    soundError("sound/chess_error.wav")
{
    if (psiOptions) {
        dndDisable      = psiOptions->getPluginOption(constDndDisable, QVariant(dndDisable)).toBool();
        confDisable     = psiOptions->getPluginOption(constConfDisable, QVariant(confDisable)).toBool();
        saveWndPosition = psiOptions->getPluginOption(constSaveWndPosition, QVariant(saveWndPosition)).toBool();
        saveWndWidthHeight
            = psiOptions->getPluginOption(constSaveWndWidthHeight, QVariant(saveWndWidthHeight)).toBool();
        windowTop        = psiOptions->getPluginOption(constWindowTop, QVariant(windowTop)).toInt();
        windowLeft       = psiOptions->getPluginOption(constWindowLeft, QVariant(windowLeft)).toInt();
        windowWidth      = psiOptions->getPluginOption(constWindowWidth, QVariant(windowWidth)).toInt();
        windowHeight     = psiOptions->getPluginOption(constWindowHeight, QVariant(windowHeight)).toInt();
        defSoundSettings = psiOptions->getPluginOption(constDefSoundSettings, QVariant(defSoundSettings)).toBool();
        soundStart       = psiOptions->getPluginOption(constSoundStart, QVariant(soundStart)).toString();
        soundFinish      = psiOptions->getPluginOption(constSoundFinish, QVariant(soundFinish)).toString();
        soundMove        = psiOptions->getPluginOption(constSoundMove, QVariant(soundMove)).toString();
        soundError       = psiOptions->getPluginOption(constSoundError, QVariant(soundError)).toString();
    }
}

OptionAccessingHost *Options::psiOptions = nullptr;

Options *Options::instance_ = nullptr;

Options *Options::instance()
{
    if (instance_ == nullptr)
        Options::instance_ = new Options();
    return Options::instance_;
}

void Options::reset()
{
    if (instance_ != nullptr) {
        delete Options::instance_;
        Options::instance_ = nullptr;
    }
}

QVariant Options::getOption(const QString &option_name) const
{
    if (option_name == constDndDisable)
        return dndDisable;
    if (option_name == constConfDisable)
        return confDisable;
    if (option_name == constSaveWndPosition)
        return saveWndPosition;
    if (option_name == constSaveWndWidthHeight)
        return saveWndWidthHeight;
    if (option_name == constWindowTop)
        return windowTop;
    if (option_name == constWindowLeft)
        return windowLeft;
    if (option_name == constWindowWidth)
        return windowWidth;
    if (option_name == constWindowHeight)
        return windowHeight;
    if (option_name == constDefSoundSettings)
        return defSoundSettings;
    if (option_name == constSoundStart)
        return soundStart;
    if (option_name == constSoundFinish)
        return soundFinish;
    if (option_name == constSoundMove)
        return soundMove;
    if (option_name == constSoundError)
        return soundError;
    return QVariant();
}

void Options::setOption(const QString &option_name, const QVariant &option_value)
{
    if ((saveWndPosition || (option_name != constWindowTop && option_name != constWindowLeft))
        && (saveWndWidthHeight || (option_name != constWindowWidth && option_name != constWindowHeight))) {
        Options::psiOptions->setPluginOption(option_name, option_value);
    }
}
