/*
 * options.h - plugin widget
 *
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <QWidget>

class GPGProc;
class OptionAccessingHost;

namespace Ui {
class Options;
}

class Options : public QWidget {
    Q_OBJECT

public:
    explicit Options(QWidget *parent = nullptr);
    ~Options();

    void update();
    void setOptionAccessingHost(OptionAccessingHost *host) { _optionHost = host; }

    void loadSettings();
    void saveSettings();

public slots:
    void addKey();
    void removeKey();

    void importKeyFromFile();
    void importKeyFromClipboard();

    void exportKeyToFile();
    void exportKeyToClipboard();

    void showInfo();

    void updateKeys();

private:
    Ui::Options *        ui;
    GPGProc *            _gpgProc;
    OptionAccessingHost *_optionHost;
};

#endif // OPTIONS_H
