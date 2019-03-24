/*
 * ripperccoptions.h
 *
 * Copyright (C) 2016
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef RIPPERCCOPTIONS_H
#define RIPPERCCOPTIONS_H

#include <QWidget>

class OptionAccessingHost;

namespace Ui { class RipperCCOptions; }

class RipperCCOptions : public QWidget
{
    Q_OBJECT

public:
    explicit RipperCCOptions(QWidget *parent = 0);
    ~RipperCCOptions();

    void update();
    void setOptionAccessingHost(OptionAccessingHost* host) { _optionHost = host; }

    void loadSettings();
    void saveSettings();

private:
    Ui::RipperCCOptions *ui;
    OptionAccessingHost *_optionHost;
};

#endif // RIPPERCCOPTIONS_H
