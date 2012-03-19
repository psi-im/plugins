/*
 * toolbar.h - plugin
 * Copyright (C) 2009-2011  Khryukin Evgeny
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

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QToolBar>

class QToolBar;
class Button;
class QSpinBox;

class ToolBar : public QToolBar
{
	Q_OBJECT
public:
	enum ButtonType {
		ButtonSelect,
		ButtonPen,
		ButtonCut,
		ButtonText,
		ButtonColor,
		ButtonUndo,
		ButtonRotate,
		ButtonCopy,
		ButtonInsert,
		ButtonNoButton
	};

	ToolBar(QWidget *parent);
	~ToolBar();
	void init();
	ToolBar::ButtonType currentButton() const;
	void checkButton(ToolBar::ButtonType);
	void enableButton(bool enable, ToolBar::ButtonType type);
	void setColorForColorButton(const QColor &color);
	void setLineWidth(int width);

private slots:
	void buttonChecked(bool);
	void buttonClicked();

signals:
	void buttonClicked(ToolBar::ButtonType);
	void checkedButtonChanged(ToolBar::ButtonType);
	void newWidth(int);

private:
	QList<Button*> buttons_;
	QSpinBox *sb;

};


#endif // TOOLBAR_H
