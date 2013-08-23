/*
 * jd_item.cpp - plugin
 * Copyright (C) 2011  Khryukin Evgeny
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

#include <QMimeData>
#include <QIODevice>
#include <QDataStream>
#include "jd_item.h"
//#include <QDebug>

JDItem::JDItem(Type t, const QString& name, const QString& size, const QString& descr, int number, JDItem* parent)
	: parent_(parent)
	, name_(name)
	, size_(size)
	, descr_(descr)
	, number_(number)
	, type_(t)
{
}

JDItem::JDItem(Type t, JDItem* parent)
	: parent_(parent)
	, type_(t)
{
}

JDItem::~JDItem()
{
}

void JDItem::setData(const QString& name, const QString& size, const QString& descr, int number)
{
	name_ = name;
	size_ = size;
	descr_ = descr;
	number_ = number;
}

JDItem* JDItem::parent() const
{
	return parent_;
}

JDItem::Type JDItem::type() const
{
	return type_;
}

QString JDItem::name() const
{
	return name_;
}

QString JDItem::size() const
{
	return size_;
}

QString JDItem::description() const
{
	return descr_;
}

QString JDItem::fullPath() const
{
	QString path;
	if(type_ == File) {
		path = QString("#%1").arg(QString::number(number_));
	}
	if(type_ == Dir) {
		path = name_;
	}
	path = parentPath() + path;

	return path;
}

QString JDItem::parentPath() const
{
	QString path;
	JDItem* it = parent_;
	while(it) {
		path = it->name() + path;
		it = it->parent();
	}
	return path;
}

int JDItem::number() const
{
	return number_;
}

const QString JDItem::mimeType()
{
	return "jditem/file";
}

bool JDItem::operator==(const JDItem& i)
{
	return name_ == i.name() && parent_ == i.parent() && number_ == i.number()
		&& size_ == i.size() && descr_ == i.description();
}

QMimeData* JDItem::mimeData() const
{
	QMimeData* data = new QMimeData();
	QByteArray ba;
	QDataStream out(&ba, QIODevice::WriteOnly);
	out << name_ << size_ << descr_ << number_ << type_;
	out << fullPath(); //Эта информация нам потребуется в моделе
	data->setData(mimeType(), ba);
	return data;
}

void JDItem::fromDataStream(QDataStream *const in)
{
	int t;
	*in >> name_ >> size_ >> descr_ >> number_ >> t;
	type_ = (Type)t;
}


//-----------------------------------
//------ItemsList--------------------
//-----------------------------------

ItemsList::ItemsList() : QList<ProxyItem>()
{
}

//ВНИМАНИЕ! Перед удалением листа желательно явно вызвать метод clear();
ItemsList::~ItemsList()
{
}

bool ItemsList::contains(const JDItem *const item) const
{
	for(int i = 0; i < size(); i++) {
		if(*at(i).item == *item)
			return true;
	}
	return false;
}

void ItemsList::clear()
{
	while(!isEmpty())
		delete this->takeFirst().item;
	QList<ProxyItem>::clear();
}
