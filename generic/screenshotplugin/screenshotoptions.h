/*
 * screenshotoptions.h - plugin
 * Copyright (C) 2009-2010  Evgeny Khryukin
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

#ifndef SCREENSHOTOPTIONS_H
#define SCREENSHOTOPTIONS_H

#include "ui_screenshotoptions.h"

class ScreenshotOptions : public QDialog
{
    Q_OBJECT
public:
    ScreenshotOptions(int delay, QWidget *parent = 0);

signals:
    void captureDesktop(int);
    void captureWindow(int);
    void captureArea(int);
    void screenshotCanceled();

private slots:
    void okPressed();
    void cancelPressed();
    void hideTimeout();

private:
    Ui::ScreenshotOptions ui_;
};

#endif // SCREENSHOTOPTIONS_H
