/*
 * iconset.h - plugin
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

#ifndef SCREENSHOTICONSET_H
#define SCREENSHOTICONSET_H

#include <QIcon>

// for Psi plugin only
class IconFactoryAccessingHost;

class ScreenshotIconset : public QObject {
    Q_OBJECT
public:
    static ScreenshotIconset *instance();
    static void               reset();
    ~ScreenshotIconset();

    QIcon getIcon(const QString &name);

    // for Psi plugin only
    void setIconHost(IconFactoryAccessingHost *_icoHost);

private:
    ScreenshotIconset();
    static ScreenshotIconset *instance_;

    // for Psi plugin only
    IconFactoryAccessingHost *icoHost;
};

#endif // SCREENSHOTICONSET_H
