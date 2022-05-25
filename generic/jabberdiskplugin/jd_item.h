/*
 * jd_item.h - plugin
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

#ifndef JD_ITEM_H
#define JD_ITEM_H

#include <QModelIndex>

class JDItem {
public:
    enum Type { None, Dir, File };

    JDItem(Type t, const QString &name, const QString &size = QString(), const QString &descr = QString(),
           int number = -1, JDItem *parent = nullptr);
    JDItem(Type t, JDItem *parent = nullptr);
    virtual ~JDItem();

    void       setData(const QString &name, const QString &size = QString(), const QString &descr = QString(),
                       int number = -1);
    JDItem    *parent() const;
    Type       type() const;
    QString    name() const;
    QString    size() const;
    QString    description() const;
    int        number() const;
    QString    fullPath() const;
    QString    parentPath() const;
    QMimeData *mimeData() const;
    void       fromDataStream(QDataStream *const in);

    static const QString mimeType();

    //    bool operator<(const JDItem& i);
    //    bool operator>(const JDItem& i);
    bool operator==(const JDItem &i);

private:
    JDItem *parent_;
    QString name_;
    QString size_;
    QString descr_;
    int     number_;
    Type    type_;
};

struct ProxyItem {
    ProxyItem() : item(nullptr), index(QModelIndex()), parent(QModelIndex()) { }

    JDItem     *item;
    QModelIndex index;
    QModelIndex parent;

    bool isNull() { return !index.isValid() && !parent.isValid() && !item; }

    bool operator==(const ProxyItem &i) { return item == i.item && index == i.index && parent == i.parent; }
};

class ItemsList : public QList<ProxyItem> {
public:
    ItemsList();
    ~ItemsList();
    bool contains(const JDItem *const item) const;
    void clear();
};

#endif // JD_ITEM_H
