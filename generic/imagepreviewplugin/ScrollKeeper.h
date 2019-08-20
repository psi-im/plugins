/*
 * ScrollKeeper.h
 *
 *  Created on: 30 Oct 2016
 *      Author: rkfg
 */

#ifndef SCROLLKEEPER_H
#define SCROLLKEEPER_H

#include <QTextEdit>
#include <QWidget>

class QWebFrame;

class ScrollKeeper {
private:
    QWidget* chatView_;
    int scrollPos_;
    bool scrollToEnd_;
    QTextEdit* ted_;
public:
    ScrollKeeper(QWidget* chatView);
    virtual ~ScrollKeeper();
};

#endif // SCROLLKEEPER_H
