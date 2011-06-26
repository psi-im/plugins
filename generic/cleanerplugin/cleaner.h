/*
 * cleaner.h - plugin
 * Copyright (C) 2009-2010  Khryukin Evgeny
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
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef CLEANER_H
#define CLEANER_H

#include <QtGui>
#include "cleanerplugin.h"
#include "ui_cleaner.h"
#include "models.h"


class CleanerMainWindow : public QMainWindow
{
    Q_OBJECT

public:
        CleanerMainWindow(CleanerPlugin *cleaner);
        virtual ~CleanerMainWindow() {};
        int height_;
        int width_;

private:
        QString vCardDir_, historyDir_, cacheDir_;
	QString profilesConfigDir_, profilesDataDir_, profilesCacheDir_;
        QAction *findHistory, *findVcards;
        void setContent();
        QLabel *sb1, *sb2, *sb3;
        bool clearDir(QString path);
        CleanerPlugin *cleaner_;
        Ui::CleanerMainWindow ui_;

        void createStatusBar();
        void updateStatusBar();
        void createMainMenu();
        QString picturesDir();
        QString avatarsDir();
        QString currentProfileDir();

        ClearingHistoryModel *historyModel_;
        ClearingVcardModel *vcardsModel_;
        ClearingAvatarModel *avatarModel_;
        ClearingOptionsModel *optionsModel_;
        ClearingProxyModel *proxyHistoryModel_, *proxyVcardsModel_;
        QSortFilterProxyModel *proxyAvatarModel_, *proxyOptionsModel_;

public slots:
        void resizeWindow(int, int);
        void showCleaner();

private slots:
        void deleteButtonPressed();
        void deleteVcards();
        void deleteHistory();
        void deleteAvatars();
        void deleteOptions();
        void filterEvent();
        void viewVcard(QModelIndex index);
        void viewHistory(QModelIndex index);
        void viewAvatar(QModelIndex index);
        void chooseProfileAct();
        void changeProfile(QString);
        void clearJuick();
        void clearBirhday();
        void currentTabChanged(int tab);
        void selectAll();
        void unselectAll();

protected:
        void closeEvent(QCloseEvent * event);
};


#endif // CLEANER_H
