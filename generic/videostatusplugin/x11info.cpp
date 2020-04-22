/*
 * Copyright (C) 2013  Sergey Ilinykh (rion)
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

#include "x11info.h"

#include <QtGlobal>
#include <X11/Xlib.h>
#include <xcb/xcb.h>

Display *X11Info::display()
{
    if (!_display) {
        _display = XOpenDisplay(nullptr);
    }
    return _display;
}

unsigned long X11Info::appRootWindow(int screen)
{
    return screen == -1 ? XDefaultRootWindow(display()) : XRootWindowOfScreen(XScreenOfDisplay(display(), screen));
}

xcb_connection_t *X11Info::xcbConnection()
{
    if (!_xcb) {
        _xcb = xcb_connect(nullptr, &_xcbPreferredScreen);
        Q_ASSERT(_xcb);
    }
    return _xcb;
}

xcb_connection_t *X11Info::_xcb = nullptr;

Display *X11Info::_display            = nullptr;
int      X11Info::_xcbPreferredScreen = 0;
