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

#pragma once

#include "lineeditwidget.h"
#include <QDate>

class QToolButton;
class QCalendarWidget;

class DateWidget : public LineEditWidget {
    Q_OBJECT

    Q_PROPERTY(QDate date READ date WRITE setDate)

public:
    explicit DateWidget(QWidget *parent = nullptr);

    // get/set date
    void  setDate(const QDate &date);
    QDate date() const;

protected slots:
    void closeCalendar(const QDate &text);
    void calendarSetDate();
    void disableExpiration();
    void keyPressEvent(QKeyEvent *event);

private:
    // Inner widgets
    QToolButton     *m_tbCalendar;
    QToolButton     *m_tbClean;
    QCalendarWidget *m_calendar;
};
