/*
 * viewer.h - plugin
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

#ifndef VIEWER_H
#define VIEWER_H

#include "iconfactoryaccessinghost.h"
#include "typeaheadfind.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QDialog>
#include <QTextEdit>

class Viewer : public QDialog {
    Q_OBJECT
public:
    Viewer(QString filename, IconFactoryAccessingHost *IcoHost, QWidget *parent = nullptr);
    bool init();

private:
    IconFactoryAccessingHost *icoHost_;
    QString fileName_;
    QDateTime lastModified_;
    QTextEdit *textWid;
    ConfLogger::TypeAheadFindBar *findBar;
    QMap<int, QString> pages_;
    int currentPage_;
    void setPage();

private slots:
    void saveLog();
    void updateLog();
    void deleteLog();
    void nextPage();
    void prevPage();
    void firstPage();
    void lastPage();

protected:
    void closeEvent(QCloseEvent *e);

signals:
    void onClose(int,int);
};

#endif // VIEWER_H
