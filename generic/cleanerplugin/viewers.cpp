/*
 * viewers.cpp - plugin
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

#include "viewers.h"

#include <QHeaderView>
#include <QMenu>
#include <QPainter>
#include <QStyleOptionViewItem>



//------------------------------------------
//-----------ClearingViewer-----------------
//------------------------------------------
void ClearingViewer::init(IconFactoryAccessingHost *iconHost)
{
        iconHost_ = iconHost;
        resizeColumnsToContents();
        horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
        horizontalHeader()->setStretchLastSection(true);
        horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
        verticalHeader()->setDefaultAlignment( Qt::AlignHCenter );

        connect(horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(sortByColumn(int)));
        connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(itemClicked(QModelIndex)));
}

void ClearingViewer::keyPressEvent(QKeyEvent * e)
{
        if (e->key() == Qt::Key_Space) {
                foreach(const QModelIndex &check, selectionModel()->selectedRows(0)) {
                        model()->setData(check, 3); //invert
                }
                e->accept();
        } else {
                QTableView::keyPressEvent(e);
                e->ignore();
        }
}

void ClearingViewer::contextMenuEvent( QContextMenuEvent * e )
{
        QMenu *popup = new QMenu(this);
        QList<QAction *> actions;
        actions <<new QAction(iconHost_->getIcon("psi/cm_check"), tr("Check"), popup)
                        <<new QAction(iconHost_->getIcon("psi/cm_uncheck"), tr("Uncheck"), popup)
                        <<new QAction(iconHost_->getIcon("psi/cm_invertcheck"), tr("Invert"), popup);
        popup->addActions(actions);
        QAction *result = popup->exec(e->globalPos());
        int iresult;
        if (result) {
                iresult = actions.indexOf(result);
                foreach(const QModelIndex &check, selectionModel()->selectedRows(0)) {
                        switch (iresult) {
                                case 0: //check
                                        model()->setData(check, QVariant(2));
                                        break;
                                case 1: //uncheck
                                        model()->setData(check, QVariant(0));
                                        break;
                                case 2: //invert
                                        model()->setData(check, QVariant(3));
                                        break;
                        }
                }
        }
        delete popup;
}

void ClearingViewer::itemClicked(QModelIndex index)
{
    if(index.column() == 0) {
        model()->setData(currentIndex(), 3); //invert
    }
}




//------------------------------------------
//-----------AvatarDelegate-----------------
//------------------------------------------
QSize AvatarDelegate::sizeHint(const QStyleOptionViewItem & /*option*/, const QModelIndex & index) const
{
    if(!index.isValid())
        return QSize(0,0);

    return QSize(300, 120);
}

void AvatarDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QPalette palette = option.palette;
    QRect r = option.rect;
    QColor c = (option.state & QStyle::State_Selected) ? palette.color(QPalette::Highlight) : palette.color(QPalette::Base);
    painter->fillRect(r, c);

    QPixmap pix = qVariantValue<QPixmap>(index.data(Qt::DisplayRole));
    pix = pix.scaled(100,100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    painter->save();
    painter->setClipRect(r);
    if(!pix.isNull()) {
        r.translate(10,10);
        r.setSize(pix.size());
        painter->drawPixmap(r, pix);
    } else {
        QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                                  ? QPalette::Normal : QPalette::Disabled;
        if (option.state & QStyle::State_Selected) {
                painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
        }
        else {
                painter->setPen(option.palette.color(cg, QPalette::Text));
        }
        r.translate(20,50);
        painter->drawText(r, tr("Empty file"));
    }
    painter->restore();
}
