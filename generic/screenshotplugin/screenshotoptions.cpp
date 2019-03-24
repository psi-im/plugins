/*
 * screenshotoptions.cpp - plugin
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

#include "screenshotoptions.h"
#include "options.h"
#include "defines.h"
#include <QTimer>

ScreenshotOptions::ScreenshotOptions(int delay, QWidget *parent)
    : QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui_.setupUi(this);
    ui_.sb_delay->setValue(delay);

    connect(ui_.buttonBox, SIGNAL(accepted()), SLOT(okPressed()));
    connect(ui_.buttonBox,SIGNAL(rejected()), SLOT(cancelPressed()));

    adjustSize();
    setFixedSize(size());
}

void ScreenshotOptions::okPressed()
{
    hide();
    QTimer::singleShot(500, this, SLOT(hideTimeout())); // чтобы при задержке 0сек это окно успело скрыться
}

void ScreenshotOptions::hideTimeout()
{
    int delay = ui_.sb_delay->value();
    Options::instance()->setOption(constDelay, delay);

    void(ScreenshotOptions::*signal)(int) = 0;
    if(ui_.rb_capture_desktop->isChecked())
        signal =  &ScreenshotOptions::captureDesktop;
    else if(ui_.rb_capture_window->isChecked())
        signal =  &ScreenshotOptions::captureWindow;
    else if(ui_.rb_capture_area->isChecked())
        signal = &ScreenshotOptions::captureArea;

    if(signal)
        emit (this->*signal)(delay);
    deleteLater();
}

void ScreenshotOptions::cancelPressed()
{
    emit screenshotCanceled();
    deleteLater();
}
