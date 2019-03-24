/*
 * iconset.cpp - plugin
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

#include "screenshoticonset.h"

#include "iconfactoryaccessinghost.h"

ScreenshotIconset* ScreenshotIconset::instance_ = 0;

ScreenshotIconset* ScreenshotIconset::instance()
{
    if(!instance_) {
        instance_ = new ScreenshotIconset();
    }

    return instance_;
}

ScreenshotIconset::ScreenshotIconset()
    : QObject(0)
    , icoHost(0)
{
}

ScreenshotIconset::~ScreenshotIconset()
{
}

void ScreenshotIconset::reset()
{
    delete instance_;
    instance_ = 0;
}

QIcon ScreenshotIconset::getIcon(const QString& name)
{
    QIcon ico;
    if(icoHost) {
        ico = icoHost->getIcon(name);
    }

    if(ico.isNull())
        ico = QIcon(":/screenshotplugin/" + name);

    return ico;
}


//for Psi plugin only
void ScreenshotIconset::setIconHost(IconFactoryAccessingHost *_icoHost)
{
    icoHost = _icoHost;
}
