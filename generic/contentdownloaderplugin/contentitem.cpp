/*
 * contentitem.cpp - item model for contents tree
 * Copyright (C) 2010  Ivan Romanov <drizt@land.ru>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <QStringList>
#include <QDebug>

#include "contentitem.h"

ContentItem::ContentItem(const QString &name, ContentItem *parent)
	: parentItem_(parent)
	, name_(name)
	, url_("")
	, html_("")
	, toInstall_(false)
	, isInstalled_(false)
{
}

ContentItem::~ContentItem()
{
	qDeleteAll(childItems_);
}

void ContentItem::appendChild(ContentItem *item)
{
	childItems_.append(item);
}

ContentItem *ContentItem::child(int row)
{
	return childItems_.value(row);
}

int ContentItem::childCount() const
{
	return childItems_.count();
}

ContentItem *ContentItem::parent()
{
	return parentItem_;
}

int ContentItem::row() const
{
	if (parentItem_) {
		return parentItem_->childItems_.indexOf(const_cast<ContentItem*>(this));
	}

	return 0;
}

QString ContentItem::group() const
{
	return group_;
}

void ContentItem::setGroup(const QString &group)
{
	group_ = group;
}

QString ContentItem::name() const
{
	return name_;
}

void ContentItem::setName(const QString &name)
{
	name_ = name;
}
	
QString ContentItem::url() const
{
	return url_;
}

void ContentItem::setUrl(const QString &url)
{
	url_ = url;
}

QString ContentItem::html() const
{
	return html_;
}

void ContentItem::setHtml(const QString &html)
{
	html_ = html;
}

bool ContentItem::toInstall() const
{
	return toInstall_;
}

void ContentItem::setToInstall(bool b)
{
	if(!isInstalled_) {
		toInstall_ = b;
	}
}
	
bool ContentItem::isInstalled() const
{
	return isInstalled_;
}

void ContentItem::setIsInstalled(bool b)
{
	isInstalled_ = b;
	toInstall_ = b ? false : toInstall_;
}

