/*
 * ScrollKeeper.cpp
 *
 *  Created on: 30 Oct 2016
 *      Author: rkfg
 */

#include "ScrollKeeper.h"
#include <QScrollBar>

//#define SCROLL_DEBUG

ScrollKeeper::ScrollKeeper(QWidget* chatView) :
        chatView_(chatView),
        scrollPos_(0),
        scrollToEnd_(false),
        ted_(0)
{
    Q_UNUSED(chatView_)

    ted_ = qobject_cast<QTextEdit*>(chatView);
    if (ted_) {
        scrollPos_ = ted_->verticalScrollBar()->value();
        if (scrollPos_ == ted_->verticalScrollBar()->maximum()) {
            scrollToEnd_ = true;
        }
#ifdef SCROLL_DEBUG
        qDebug() << "QTED Scroll pos:" << scrollPos_ << "to end:" << scrollToEnd_ << "max:"
                << ted_->verticalScrollBar()->maximum();
#endif
    }
}

ScrollKeeper::~ScrollKeeper() {
    if (ted_) {
#ifdef SCROLL_DEBUG
        qDebug() << "QTED restoring scroll pos:" << scrollPos_ << "to end:" << scrollToEnd_ << "max:"
                << ted_->verticalScrollBar()->maximum();
#endif
        ted_->verticalScrollBar()->setValue(scrollToEnd_ ? ted_->verticalScrollBar()->maximum() : scrollPos_);
    }
}

