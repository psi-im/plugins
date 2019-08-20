/*
 * contentitem.h - item model for contents tree
 * Copyright (C) 2010  Ivan Romanov <drizt@land.ru>
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

#ifndef CONTENTITEM_H
#define CONTENTITEM_H

#include <QList>
#include <QVariant>
#include <QString>

class ContentItem {
public:
    ContentItem(const QString &name, ContentItem *parent = nullptr);
    ~ContentItem();

    void appendChild(ContentItem *child);

    ContentItem* child(int row);
    int childCount() const;
    int row() const;
    ContentItem *parent();

    QString group() const;
    void setGroup(const QString &name);

    QString name() const;
    void setName(const QString &name);
    
    QString url() const;
    void setUrl(const QString &url);

    QString html() const;
    void setHtml(const QString &html);

    bool toInstall() const;
    void setToInstall(bool b);
    
    bool isInstalled() const;
    void setIsInstalled(bool b);

private:
    ContentItem *parentItem_;
    QList<ContentItem*> childItems_;

    QString group_;
    QString name_;
    QString url_;
    QString html_;
    bool toInstall_;
    bool isInstalled_;
};

#endif // CONTENTITEM_H
