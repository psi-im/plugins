#ifndef DATEWIDGET_H
#define DATEWIDGET_H

#include "lineeditwidget.h"

#include <QDate>

class QCalendarWidget;
class QToolButton;

class DateWidget : public LineEditWidget {
    Q_OBJECT

    Q_PROPERTY(QDate date
               READ date
               WRITE setDate)

public:
    explicit DateWidget(QWidget *parent = nullptr);

    // get/set date
    void setDate(const QDate &date);
    QDate date() const;

protected slots:
    void closeCalendar(const QDate &text);
    void calendarSetDate();
    void disableExpiration();
    void keyPressEvent(QKeyEvent *event);

private:
    // Inner widgets
    QToolButton *_tbCalendar;
    QToolButton *_tbClean;
    QCalendarWidget *_calendar;
};

#endif // DATEWIDGET_H
