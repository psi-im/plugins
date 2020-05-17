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
#include <QCalendarWidget>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLocale>
#include <QRegExp>
#include <QRegExpValidator>

#include "datewidget.h"

DateWidget::DateWidget(QWidget *parent) :
    LineEditWidget(parent), m_tbCalendar(new QToolButton(this)), m_tbClean(new QToolButton(this)),
    m_calendar(new QCalendarWidget(this))
{
    setReadOnly(true);

    m_tbClean->setObjectName("brClear");
    m_tbClean->setIcon(QIcon(":/icons/clean.png"));
    m_tbClean->setContentsMargins(0, 0, 0, 0);
    m_tbClean->setFocusPolicy(Qt::NoFocus);
    m_tbClean->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_tbClean->setIconSize(QSize(16, 16));
    m_tbClean->setAutoRaise(true);
    m_tbClean->setAutoFillBackground(true);
    m_tbClean->setCursor(QCursor(Qt::ArrowCursor));
    m_tbClean->resize(0, 0);
    addWidget(m_tbClean);

    m_tbCalendar->setObjectName("tbCalendar");
    m_tbCalendar->setIcon(QIcon(":/icons/calendar.png"));
    m_tbCalendar->setContentsMargins(0, 0, 0, 0);
    m_tbCalendar->setFocusPolicy(Qt::NoFocus);
    m_tbCalendar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_tbCalendar->setIconSize(QSize(16, 16));
    m_tbCalendar->setAutoRaise(true);
    m_tbCalendar->setAutoFillBackground(true);
    m_tbCalendar->setCursor(QCursor(Qt::ArrowCursor));
    m_tbCalendar->resize(0, 0);
    addWidget(m_tbCalendar);

    setPopup(m_calendar);

    connect(m_calendar, &QCalendarWidget::clicked, this, &DateWidget::closeCalendar);
    connect(m_tbCalendar, &QToolButton::clicked, this, &DateWidget::showPopup);
    connect(m_tbCalendar, &QToolButton::clicked, this, &DateWidget::calendarSetDate);
    connect(m_tbClean, &QToolButton::clicked, this, &DateWidget::disableExpiration);
}

// Always use format of current locale
inline QString dateFormat()
{
    QString format = QLocale().dateFormat(QLocale::LongFormat);
#ifdef Q_OS_MAC
    // The LongFormat has changed between OS X 10.6 and 10.7.
    // https://qt.gitorious.org/qt/qtbase/commit/8e722eb/diffs
    // https://bugreports.qt-project.org/browse/QTBUG-27790
    if (format.count('y') == 1) {
        format.replace('y', "yyyy");
    }
#endif
    return format;
}

void DateWidget::setDate(const QDate &date) { setText(date.toString(dateFormat())); }

QDate DateWidget::date() const { return QDate::fromString(text(), dateFormat()); }

void DateWidget::closeCalendar(const QDate &date)
{
    setDate(date);
    hidePopup();
}

void DateWidget::calendarSetDate()
{
    if (date().isValid()) {
        m_calendar->setSelectedDate(date());
    }
}

void DateWidget::disableExpiration() { setText(tr("never")); }

void DateWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
        disableExpiration();
    } else if (event->key() == Qt::Key_Space) {
        showPopup();
    } else {
        LineEditWidget::keyPressEvent(event);
    }
}
