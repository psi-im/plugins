/*
 * defines.h - plugin
 * Copyright (C) 2011  Khryukin Evgeny
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

#ifndef DEFINES_H
#define DEFINES_H

#define cVersion "0.6.5"

#define constName "Screenshot Plugin"
#define constVersionOption "version"
#define constLastFolder "lastfolder"
#define constHistory "history"
#define constShortCut "shortCut"
#define constFormat "format"
#define constFileName "fileName"
#define constServerList "serverlist"
#define constDelay "delay"
#define constWindowState "geometry.state"
#define constWindowX "geometry.x"
#define constWindowY "geometry.y"
#define constWindowWidth "geometry.width"
#define constWindowHeight "geometry.height"
#define constDefaultAction "default-action"
#define constRadius "radius"

enum DefaultAction { Desktop, Area, Window };

#endif // DEFINES_H
