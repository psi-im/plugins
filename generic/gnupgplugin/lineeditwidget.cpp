#include <QBoxLayout>
#include <QApplication>
#include <QDesktopWidget>
#include <QEvent>
#include <QRegExpValidator>

#include "lineeditwidget.h"

LineEditWidget::LineEditWidget(QWidget *parent)
    : QLineEdit(parent)
    , _layout(new QHBoxLayout())
    , _popup(nullptr)
    , _optimalLength(0)
{
    _layout->setSpacing(0);
    _layout->setContentsMargins(1, 3, 2, 3);
    _layout->addWidget(new QWidget());

    setLayout(_layout);
    setContentsMargins(0, 0, 0, 0);
    installEventFilter(this);
}

LineEditWidget::~LineEditWidget()
{
    _toolbuttons.clear();
}


QSize LineEditWidget::sizeHint() const
{
    QSize size;
    size = QLineEdit::sizeHint();

    int width = 0;

    if(_optimalLength) {
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
        width += fontMetrics().width("0") * _optimalLength;
#else
        width += fontMetrics().horizontalAdvance("0") * _optimalLength;
#endif
    }
    else {
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
    for(int i = _toolbuttons.size() - 1; i >= 0; i--) {
        width += _toolbuttons[i]->width();
    }

    setTextMargins(0, 0, width, 0);
    QLineEdit::showEvent(e);
}

bool LineEditWidget::eventFilter(QObject *o, QEvent *e)
{
    return QLineEdit::eventFilter(o, e);
}

void LineEditWidget::setRxValidator(const QString &str)
{
    _rxValidator = str;
    if (str.isEmpty()) {
        return;
    }

    QRegExp rx(str);
    QRegExpValidator *validator = new QRegExpValidator(rx, this);
    setValidator(validator);
}

void LineEditWidget::addWidget(QWidget *w)
{
    _toolbuttons << w;
    _layout->addWidget(w);
}

void LineEditWidget::setPopup(QWidget *w)
{
    if(_popup) {
        delete _popup;
        _popup = nullptr;
    }

    _popup = new QFrame(this);
    _popup->setWindowFlags(Qt::Popup);
    _popup->setFrameStyle(QFrame::StyledPanel);
    _popup->setAttribute(Qt::WA_WindowPropagation);
    _popup->setAttribute(Qt::WA_X11NetWmWindowTypeCombo);

    QBoxLayout *layout = new QBoxLayout(QBoxLayout::TopToBottom);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(w);
    _popup->setLayout(layout);
}

void LineEditWidget::showPopup()
{
    _popup->adjustSize();
    _popup->move(mapToGlobal(QPoint(width() - _popup->geometry().width(), height())));
    QSize size = qApp->desktop()->size();
    QRect rect = _popup->geometry();

    // if widget is beyond edge of display
    if(rect.right() > size.width()) {
        rect.moveRight(size.width());
    }

    if(rect.bottom() > size.height()) {
        rect.moveBottom(size.height());
    }

    _popup->move(rect.topLeft());
    _popup->show();
}

void LineEditWidget::hidePopup()
{
    if (_popup->isVisible()) {
        _popup->hide();
    }
}
