/*
 * toolbar.cpp - plugin
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

#include <QAction>
#include <QLabel>
#include <QSpinBox>

#include "screenshoticonset.h"
#include "toolbar.h"

class Button : public QAction {
    Q_OBJECT
public:
    Button(const QString &tip, const QIcon &ico, ToolBar::ButtonType type, bool checkable, QWidget *parent) :
        QAction(parent), type_(type)
    {
        setToolTip(tip);
        setIcon(ico);
        setCheckable(checkable);
    }

    ToolBar::ButtonType type() const { return type_; }

private:
    ToolBar::ButtonType type_;
};

//----------------ToolBar--------------------------------

ToolBar::ToolBar(QWidget *parent) : QToolBar(parent) { init(); }

ToolBar::~ToolBar()
{
    for (Button *b : std::as_const(buttons_)) {
        delete (b);
    }
    buttons_.clear();
}

void ToolBar::init()
{
    ScreenshotIconset *icoHost = ScreenshotIconset::instance();

    addWidget(new QLabel(tr("Line Width:")));
    sb = new QSpinBox(this);
    sb->setMinimum(1);
    setLineWidth(2);
    sb->setToolTip(tr("Line width"));
    addWidget(sb);
    // TODO: update after stopping support of Ubuntu Xenial:
    connect(sb, SIGNAL(valueChanged(int)), this, SIGNAL(newWidth(int)));

    QPixmap pix(16, 16);
    pix.fill(QColor(Qt::black));
    QIcon ico(pix);
    buttons_.append(new Button(tr("Select Color"), ico, ToolBar::ButtonColor, false, this));
    buttons_.append(new Button(tr("Pen"), icoHost->getIcon("psi/draw"), ToolBar::ButtonPen, true, this));
    buttons_.append(new Button(tr("Select"), icoHost->getIcon("psi/frame"), ToolBar::ButtonSelect, true, this));
    buttons_.append(new Button(tr("Cut"), icoHost->getIcon("psi/crop"), ToolBar::ButtonCut, false, this));
    buttons_.last()->setShortcut(QKeySequence("Ctrl+x"));

    buttons_.append(new Button(tr("Copy"), icoHost->getIcon("psi/copy"), ToolBar::ButtonCopy, false, this));
    buttons_.last()->setShortcut(QKeySequence("Ctrl+c"));

    buttons_.append(new Button(tr("Paste"), icoHost->getIcon("psi/paste"), ToolBar::ButtonInsert, false, this));
    buttons_.last()->setShortcut(QKeySequence("Ctrl+v"));

    buttons_.append(new Button(tr("Rotate"), icoHost->getIcon("psi/rotate"), ToolBar::ButtonRotate, false, this));
    buttons_.append(new Button(tr("Insert Text"), icoHost->getIcon("psi/text"), ToolBar::ButtonText, true, this));
    buttons_.append(new Button(tr("Undo"), icoHost->getIcon("psi/undo"), ToolBar::ButtonUndo, false, this));
    buttons_.last()->setShortcut(QKeySequence("Ctrl+z"));

    for (Button *b : std::as_const(buttons_)) {
        addAction(b);
        // TODO: update after stopping support of Ubuntu Xenial:
        connect(b, SIGNAL(triggered(bool)), SLOT(buttonChecked(bool)));
        connect(b, SIGNAL(triggered()), SLOT(buttonClicked()));
    }

    enableButton(false, ToolBar::ButtonUndo);
}

void ToolBar::enableButton(bool enable, ToolBar::ButtonType type)
{
    for (Button *b : std::as_const(buttons_)) {
        if (b->type() == type) {
            b->setEnabled(enable);
            break;
        }
    }
}

void ToolBar::checkButton(ToolBar::ButtonType type)
{
    for (Button *b : std::as_const(buttons_)) {
        if (b->type() == type && b->isCheckable()) {
            b->setChecked(true);
            break;
        }
    }
    emit checkedButtonChanged(type);
}

void ToolBar::buttonChecked(bool check)
{
    Button *s = (Button *)sender();

    if (!s->isCheckable()) {
        return;
    }

    if (s->type() == ButtonSelect && check) {
        enableButton(true, ButtonCut);
    } else {
        enableButton(false, ButtonCut);
    }

    if (check) {
        for (Button *b : std::as_const(buttons_)) {
            if (b != s)
                b->setChecked(false);
        }
        emit checkedButtonChanged(s->type());
    } else
        emit checkedButtonChanged(ToolBar::ButtonNoButton);
}

void ToolBar::setColorForColorButton(const QColor &color)
{
    for (Button *b : std::as_const(buttons_)) {
        if (b->type() == ButtonColor) {
            QPixmap pix(16, 16);
            pix.fill(color);
            b->setIcon(QIcon(pix));
            break;
        }
    }
}

void ToolBar::buttonClicked()
{
    Button *s = (Button *)sender();
    if (s)
        emit buttonClicked(s->type());
}

ToolBar::ButtonType ToolBar::currentButton() const
{
    for (Button *b : buttons_) {
        if (b->isChecked())
            return b->type();
    }
    return ToolBar::ButtonNoButton;
}

void ToolBar::setLineWidth(int width) { sb->setValue(width); }

#include "toolbar.moc"
