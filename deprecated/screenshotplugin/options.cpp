/*
 * options.cpp - plugin
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

#include "options.h"
#include "optionaccessinghost.h"

Options *Options::instance_ = nullptr;

Options *Options::instance()
{
    if (!instance_) {
        instance_ = new Options();
    }

    return instance_;
}

Options::Options() : QObject(nullptr), psiOptions(nullptr) { }

Options::~Options() { }

void Options::reset()
{
    delete instance_;
    instance_ = nullptr;
}

QVariant Options::getOption(const QString &name, const QVariant &defValue)
{
    QVariant val = defValue;
    if (psiOptions) {
        val = psiOptions->getPluginOption(name, val);
    }

    return val;
}

void Options::setOption(const QString &name, const QVariant &value)
{
    if (psiOptions) {
        psiOptions->setPluginOption(name, value);
    }
}

// for Psi plugin only
void Options::setPsiOptions(OptionAccessingHost *_psiOptions) { psiOptions = _psiOptions; }
