/*
 * options.h - plugin
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <QVariant>

//for Psi plugin only
class OptionAccessingHost;

class Options : public QObject {
    Q_OBJECT
public:
    static Options* instance();
    static void reset();
    ~Options();

    QVariant getOption(const QString& name, const QVariant& defValue = QVariant::Invalid);
    void setOption(const QString& name, const QVariant& value);

    //for Psi plugin only
    void setPsiOptions(OptionAccessingHost* psiOptions);

private:
    Options();
    static Options* instance_;

    //for Psi plugin only
    OptionAccessingHost* psiOptions;
};

#endif // OPTIONS_H
