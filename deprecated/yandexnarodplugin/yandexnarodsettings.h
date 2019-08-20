/*
 * yandexnarodsettings.h - plugin
 * Copyright (C) 2009  Alexander Kazarin <boiler@co.ru>
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

#ifndef YANDEXNARODSETTINGS_H
#define YANDEXNARODSETTINGS_H

#include "ui_yandexnarodsettings.h"

class yandexnarodSettings  : public QWidget {
    Q_OBJECT;

public:
    yandexnarodSettings(QWidget *p = 0);
    ~yandexnarodSettings();
    QString getLogin() const { return ui.editLogin->text(); }
    QString getPasswd() const { return ui.editPasswd->text(); }
    void btnTest_enabled(bool b) { ui.btnTest->setEnabled(b); }
    void restoreSettings();

public slots:
    void setStatus(const QString& str);
    void saveSettings();

private slots:
    void on_btnClearCookies_clicked();

private:
    Ui::yandexnarodSettingsClass ui;

signals:
    void testclick();
    void startManager();

};

#endif // YANDEXNARODSETTINGS_H
