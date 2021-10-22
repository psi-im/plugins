/*
 * view.cpp - plugin
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

#include "view.h"
#include "model.h"

#include <QHeaderView>
#include <QMenu>

#include "delegate.h"
#include "iconfactoryaccessinghost.h"

Viewer::Viewer(QWidget *parent) : QTableView(parent) { setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); }

void Viewer::init(IconFactoryAccessingHost *iconHost)
{
    iconHost_ = iconHost;
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setItemDelegateForColumn(3, new IconDelegate(iconHost_, this));
    setItemDelegateForColumn(4, new IconDelegate(iconHost_, this));
    setItemDelegateForColumn(1, new LineEditDelegate(this));
    setItemDelegateForColumn(2, new LineEditDelegate(this));

    QHeaderView *header = horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    verticalHeader()->setDefaultAlignment(Qt::AlignHCenter);

    resizeColumnsToContents();

    connect(this, &Viewer::clicked, this, &Viewer::itemClicked);
}

void Viewer::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Space) {
        const auto checks = selectionModel()->selectedRows(0);
        for (const QModelIndex &check : checks) {
            model()->setData(check, 3); // invert
        }
    } else {
        QTableView::keyPressEvent(e);
    }
    e->accept();
}

void Viewer::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *          popup = new QMenu(this);
    QList<QAction *> actions;
    actions << new QAction(iconHost_->getIcon("psi/cm_check"), tr("Check"), popup)
            << new QAction(iconHost_->getIcon("psi/cm_uncheck"), tr("Uncheck"), popup)
            << new QAction(iconHost_->getIcon("psi/cm_invertcheck"), tr("Invert"), popup);
    popup->addActions(actions);
    QAction *result = popup->exec(e->globalPos());
    int      iresult;
    if (result) {
        iresult           = actions.indexOf(result);
        const auto checks = selectionModel()->selectedRows(0);
        for (const QModelIndex &check : checks) {
            switch (iresult) {
            case 0: // check
                model()->setData(check, QVariant(2));
                break;
            case 1: // uncheck
                model()->setData(check, QVariant(0));
                break;
            case 2: // invert
                model()->setData(check, QVariant(3));
                break;
            }
        }
    }
    delete popup;
}

void Viewer::itemClicked(const QModelIndex &index)
{
    if (index.column() == 0) {
        model()->setData(index, 3); // invert
    } else if (index.column() == 4) {
        emit checkSound(index);
    } else if (index.column() == 3) {
        emit getSound(index);
    }
}

void Viewer::deleteSelected()
{
    QItemSelectionModel *selection = selectionModel();
    qobject_cast<Model *>(model())->deleteRows(selection->selectedRows());
}
