#ifndef DATEWIDGET_H
#define DATEWIDGET_H

#include <QDate>
#include "lineeditwidget.h"

class QToolButton;
class QCalendarWidget;

class DateWidget : public LineEditWidget
{
	Q_OBJECT

	Q_PROPERTY(QDate date
			   READ date
			   WRITE setDate)

public:
	explicit DateWidget(QWidget *parent = 0);

	// get/set date
	void setDate(const QDate &date);
	QDate date() const;

protected slots:
	void closeCalendar(const QDate &text);
	void calendarSetDate();
	void disableExpiration();

private:
	// Inner widgets
	QToolButton *_tbCalendar;
	QToolButton *_tbClean;
	QCalendarWidget *_calendar;
};

#endif // DATEWIDGETs_H
