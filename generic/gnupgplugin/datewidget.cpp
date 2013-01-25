#include <QKeyEvent>
#include <QLineEdit>
#include <QCalendarWidget>
#include <QIcon>
#include <QHBoxLayout>
#include <QRegExp>
#include <QRegExpValidator>
#include <QApplication>
#include <QDesktopWidget>
#include <QLocale>
#include <QDebug>

#include "datewidget.h"

DateWidget::DateWidget(QWidget *parent)
	: LineEditWidget(parent)
	, _tbCalendar(new QToolButton(this))
	, _tbClean(new QToolButton(this))
	, _calendar(new QCalendarWidget(this))
{
	setReadOnly(true);

	_tbClean->setObjectName("brClear");
	_tbClean->setIcon(QIcon(":/icons/clean.png"));
	_tbClean->setContentsMargins(0, 0, 0, 0);
	_tbClean->setFocusPolicy(Qt::NoFocus);
	_tbClean->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	_tbClean->setIconSize(QSize(16, 16));
	_tbClean->setAutoRaise(true);
	_tbClean->setAutoFillBackground(true);
	_tbClean->setCursor(QCursor(Qt::ArrowCursor));
	_tbClean->resize(0, 0);
	addWidget(_tbClean);

	_tbCalendar->setObjectName("tbCalendar");
	_tbCalendar->setIcon(QIcon(":/icons/calendar.png"));
	_tbCalendar->setContentsMargins(0, 0, 0, 0);
	_tbCalendar->setFocusPolicy(Qt::NoFocus);
	_tbCalendar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	_tbCalendar->setIconSize(QSize(16, 16));
	_tbCalendar->setAutoRaise(true);
	_tbCalendar->setAutoFillBackground(true);
	_tbCalendar->setCursor(QCursor(Qt::ArrowCursor));
	_tbCalendar->resize(0, 0);
	addWidget(_tbCalendar);

	setPopup(_calendar);

	connect(_calendar, SIGNAL(clicked(const QDate&)), SLOT(closeCalendar(const QDate&)));
	connect(_tbCalendar, SIGNAL(clicked()), SLOT(showPopup()));
	connect(_tbCalendar, SIGNAL(clicked()), SLOT(calendarSetDate()));

	connect(_tbClean, SIGNAL(clicked()), SLOT(disableExpiration()));
}

// Always use format of current locale
inline QString dateFormat()
{
	return QLocale().dateFormat(QLocale::LongFormat);
}

void DateWidget::setDate(const QDate &date)
{
	setText(date.toString(dateFormat()));
}

QDate DateWidget::date() const
{
	return QDate::fromString(text(), dateFormat());
}

void DateWidget::closeCalendar(const QDate &date)
{
	setDate(date);
	hidePopup();
}

void DateWidget::calendarSetDate()
{
	if(date().isValid()) {
		_calendar->setSelectedDate(date());
	}
}

void DateWidget::disableExpiration()
{
	setText(trUtf8("never"));
}
