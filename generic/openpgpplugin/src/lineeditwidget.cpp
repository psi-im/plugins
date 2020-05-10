/*
 * Copyright (C) 2013  Ivan Romanov <drizt@land.ru>
 * Copyright (C) 2020  Boris Pek <tehnick-8@yandex.ru>
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

#include <QApplication>
#include <QBoxLayout>
#include <QDesktopWidget>
#include <QEvent>
#include <QRegExpValidator>

#include "lineeditwidget.h"

LineEditWidget::LineEditWidget(QWidget *parent) :
    QLineEdit(parent), m_layout(new QHBoxLayout()), m_popup(nullptr), m_optimalLength(0)
{
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(1, 3, 2, 3);
    m_layout->addWidget(new QWidget());

    setLayout(m_layout);
    setContentsMargins(0, 0, 0, 0);
    installEventFilter(this);
}

LineEditWidget::~LineEditWidget() { m_toolbuttons.clear(); }

QSize LineEditWidget::sizeHint() const
{
    QSize size;
    size = QLineEdit::sizeHint();

    int width = 0;

    if (m_optimalLength) {
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
        width += fontMetrics().width("0") * _optimalLength;
#else
        width += fontMetrics().horizontalAdvance("0") * m_optimalLength;
#endif
    } else {
        width += size.width();
    }
    width += textMargins().right();
    size.setWidth(width);
    return size;
}

void LineEditWidget::showEvent(QShowEvent *e)
{
    // Width of standard QLineEdit plus extended tool buttons
    int width = 0;
    for (int i = m_toolbuttons.size() - 1; i >= 0; i--) {
        width += m_toolbuttons[i]->width();
    }

    setTextMargins(0, 0, width, 0);
    QLineEdit::showEvent(e);
}

bool LineEditWidget::eventFilter(QObject *o, QEvent *e) { return QLineEdit::eventFilter(o, e); }

void LineEditWidget::setRxValidator(const QString &str)
{
    m_rxValidator = str;
    if (str.isEmpty()) {
        return;
    }

    QRegExp           rx(str);
    QRegExpValidator *validator = new QRegExpValidator(rx, this);
    setValidator(validator);
}

void LineEditWidget::addWidget(QWidget *w)
{
    m_toolbuttons << w;
    m_layout->addWidget(w);
}

void LineEditWidget::setPopup(QWidget *w)
{
    if (m_popup) {
        delete m_popup;
        m_popup = nullptr;
    }

    m_popup = new QFrame(this);
    m_popup->setWindowFlags(Qt::Popup);
    m_popup->setFrameStyle(QFrame::StyledPanel);
    m_popup->setAttribute(Qt::WA_WindowPropagation);
    m_popup->setAttribute(Qt::WA_X11NetWmWindowTypeCombo);

    QBoxLayout *layout = new QBoxLayout(QBoxLayout::TopToBottom);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(w);
    m_popup->setLayout(layout);
}

void LineEditWidget::showPopup()
{
    m_popup->adjustSize();
    m_popup->move(mapToGlobal(QPoint(width() - m_popup->geometry().width(), height())));
    QSize size = qApp->desktop()->size();
    QRect rect = m_popup->geometry();

    // if widget is beyond edge of display
    if (rect.right() > size.width()) {
        rect.moveRight(size.width());
    }

    if (rect.bottom() > size.height()) {
        rect.moveBottom(size.height());
    }

    m_popup->move(rect.topLeft());
    m_popup->show();
}

void LineEditWidget::hidePopup()
{
    if (m_popup->isVisible()) {
        m_popup->hide();
    }
}
