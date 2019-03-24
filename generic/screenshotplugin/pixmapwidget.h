/*
 * pixmapwidget.h - plugin
 * Copyright (C) 2009-2011  Evgeny Khryukin
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

#ifndef PIXMAPWIDGET_H
#define PIXMAPWIDGET_H

#include <QPen>
#include <QPointer>
#include <QVariant>

#include "toolbar.h"

#define constPenWidth "penwidth"
#define constColor "color"
#define constFont "font"

class SelectionRect : public QRect
{
public:
    SelectionRect();
    SelectionRect(int left, int top, int w, int h);
    void clear();
    enum CornerType { NoCorner, TopLeft, BottomLeft, TopRight, BottomRight };
    CornerType cornerUnderMouse(const QPoint& pos) const;
};


class PixmapWidget : public QWidget
{
    Q_OBJECT
public:
    PixmapWidget(QWidget *parent);
    ~PixmapWidget();
    void setToolBar(ToolBar *bar);
    void setPixmap(const QPixmap& pix);
    QPixmap getPixmap() const { return mainPixmap; }

private slots:    
    void checkedButtonChanged(ToolBar::ButtonType type);
    void paintToPixmap(QString text = "");
    void newWidth(int w);
    void buttonClicked(ToolBar::ButtonType);
    void cut();
    void copy();
    void selectFont();
    void blur();
    void insert();

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *);

private:
    void saveUndoPixmap();
    void selectColor();
    void undo();
    void rotate();
    void init(int lineWidth, const QString& color, const QString& font);

signals:
    void adjusted();
    void settingsChanged(const QString&, const QVariant&);
    void modified(bool);

private:
    ToolBar *bar_;
    QColor color_;
    QList<QPixmap> undoList_;
    QPixmap mainPixmap;
    ToolBar::ButtonType type_;
    QPoint p1;
    QPoint p2;
    QPen pen;
    QPen draftPen;
    QFont font_;
    SelectionRect* selectionRect;
    QCursor currentCursor;
    SelectionRect::CornerType cornerType;

    enum SmoothLineType { None, Horizontal, Vertical };
    SmoothLineType smoothLineType_;
};

#endif // PIXMAPWIDGET_H
