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

#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QX11Info>
#else
#include <QGuiApplication>
#endif

#include "x11info.h"

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

bool X11Info::isPlatformX11()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return QX11Info::isPlatformX11();
#else
    auto x11app = qApp->nativeInterface<QNativeInterface::QX11Application>();
    return !!x11app;
#endif
}

WindowList X11Info::getWindows(Atom prop)
{
    WindowList res;
    Atom       type   = 0;
    int        format = 0;
    uchar     *data   = nullptr;
    ulong      count, after;
    Display   *display = X11Info::display();
    Window     window  = X11Info::appRootWindow();
    if (XGetWindowProperty(display, window, prop, 0, 1024 * sizeof(Window) / 4, False, AnyPropertyType, &type, &format,
                           &count, &after, &data)
        == Success) {
        Window *list = reinterpret_cast<Window *>(data);
        for (uint i = 0; i < count; ++i)
            res += list[i];
        if (data)
            XFree(data);
    }
    return res;
}

Window X11Info::activeWindow()
{
    static Atom net_active = 0;
    if (!net_active)
        net_active = XInternAtom(X11Info::display(), "_NET_ACTIVE_WINDOW", True);

    return X11Info::getWindows(net_active).value(0);
}

bool X11Info::isWindowFullscreen()
{
    Window         w          = activeWindow();
    Display       *display    = X11Info::display();
    bool           full       = false;
    static Atom    state      = XInternAtom(display, "_NET_WM_STATE", False);
    static Atom    fullScreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    Atom           actual_type;
    int            actual_format;
    unsigned long  nitems;
    unsigned long  bytes;
    unsigned char *data = nullptr;

    if (XGetWindowProperty(display, w, state, 0, (~0L), False, AnyPropertyType, &actual_type, &actual_format, &nitems,
                           &bytes, &data)
        == Success) {
        if (nitems != 0) {
            Atom *atom = reinterpret_cast<Atom *>(data);
            for (ulong i = 0; i < nitems; i++) {
                if (atom[i] == fullScreen) {
                    full = true;
                    break;
                }
            }
        }
    }
    if (data)
        XFree(data);
    return full;
}

xcb_connection_t *X11Info::_xcb = nullptr;

Display *X11Info::_display            = nullptr;
int      X11Info::_xcbPreferredScreen = 0;
