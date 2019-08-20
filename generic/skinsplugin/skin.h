/*
 * skin.h - plugin
 * Copyright (C) 2010  Evgeny Khryukin
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

#ifndef SKIN_H
#define SKIN_H

#include "ui_getskinname.h"
#include "ui_previewer.h"

#include <QListWidgetItem>

class Skin : public QListWidgetItem {
    public:
         Skin(QListWidget* parent) : QListWidgetItem(parent) {}
         ~Skin() {}
         void setFile(const QString &file);
         QString filePass();
         QString name();
         QString skinFolder();
         QPixmap previewPixmap();

    private:
         QString filePass_;

};

class Previewer : public QDialog {
    Q_OBJECT
    public:
        Previewer(Skin *skin, QWidget *parent = nullptr);
        bool loadSkinInformation();

    private:        
        Ui::Previewer ui_;
        Skin *skin_;

    signals:
        void applySkin();

};

class GetSkinName : public QDialog {
    Q_OBJECT
    public:
        GetSkinName(QString name, QString author, QString version, QWidget *parent = nullptr);

    private slots:
        void okPressed();

    signals:
        void ok(QString, QString, QString);

    private:
        Ui::GetSkinName ui_;
    };

#endif // SKIN_H
