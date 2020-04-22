/*
 * ripperccoptions.cpp
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

#include "ripperccoptions.h"
#include "ui_ripperccoptions.h"

#include "optionaccessinghost.h"

RipperCCOptions::RipperCCOptions(QWidget *parent) : QWidget(parent), ui(new Ui::RipperCCOptions) { ui->setupUi(this); }

RipperCCOptions::~RipperCCOptions() { delete ui; }

void RipperCCOptions::loadSettings()
{
    ui->sbInterval->setValue(_optionHost->getPluginOption("attention-interval", 1).toInt());
}

void RipperCCOptions::saveSettings() { _optionHost->setPluginOption("attention-interval", ui->sbInterval->value()); }
