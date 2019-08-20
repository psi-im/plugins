/*
 * Copyright (C) 2013  Sergey Ilinykh
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

#ifndef X11INFO_H
#define X11INFO_H

typedef struct _XDisplay Display;
typedef struct xcb_connection_t xcb_connection_t;

class X11Info {
    static Display *_display;
    static xcb_connection_t *_xcb;
    static int _xcbPreferredScreen;

public:
    static Display* display();
    static unsigned long appRootWindow(int screen = -1);
    static int appScreen();
    static xcb_connection_t* xcbConnection();
    static inline int xcbPreferredScreen() { return _xcbPreferredScreen; }
};

#endif // X11INFO_H
