/*
 * controller.h - plugin
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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QPointer>

class ApplicationInfoAccessingHost;
class Screenshot;

class Controller : public QObject {
    Q_OBJECT
public:
    Controller(ApplicationInfoAccessingHost* appInfo);
    ~Controller();

public slots:
    void onShortCutActivated();
    void openImage();

private:
    void doUpdate();

    QPointer<Screenshot> screenshot;
    ApplicationInfoAccessingHost* appInfo_;

};

#endif // CONTROLLER_H
