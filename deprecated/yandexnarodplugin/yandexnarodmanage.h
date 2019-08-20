/*
 * yandexnarodmanage.h - plugin
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

#ifndef YANDEXNARODMANAGE_H
#define YANDEXNARODMANAGE_H

#include "yandexnarodnetman.h"

#include <QDialog>
#include <QListWidget>

namespace Ui {
    class yandexnarodManageClass;
}

class ListWidget : public QListWidget {
    Q_OBJECT
public:
    ListWidget(QWidget* p = 0);

protected:
    virtual QStringList mimeTypes() const;
    virtual QMimeData *mimeData(const QList<QListWidgetItem *> &items) const;
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);

signals:
    void menu(const yandexnarodNetMan::FileItem&);
    void uploadFile(const QString&);
};

class yandexnarodManage : public QDialog {
    Q_OBJECT

public:
    yandexnarodManage(QWidget* p = 0);
    ~yandexnarodManage();

private:
    void newNetMan();
    void netmanPrepare();
    void copyToClipboard(const QString& text);

private slots:
    void newFileItem(yandexnarodNetMan::FileItem);
    void on_btnDelete_clicked();
    void on_btnClipboard_clicked();
    void on_listWidget_pressed(QModelIndex index);
    void on_btnReload_clicked();
    void on_btnUpload_clicked();
    void on_btnProlong_clicked();
    void on_btnClearCookies_clicked();
    void on_btnOpenBrowser_clicked();
    void netmanFinished();
    void doMenu(const yandexnarodNetMan::FileItem& item);
    void uploadFile(const QString& path);

private:
    Ui::yandexnarodManageClass* ui_;
    yandexnarodNetMan *netman;
    QList<QIcon> fileicons;
    QHash<QString, int> fileiconstyles;
};

#endif // YANDEXNARODMANAGE_H
